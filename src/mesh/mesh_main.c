#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static pthread_t mesh_thread;
static pthread_mutex_t mesh_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mesh_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mesh_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool mesh_thread_running = false;
static bool mesh_thread_should_exit = false;

typedef struct {
	uint8_t x, y, z;
	bool needs_lighting;
} chunk_mesh_job_t;

#define MAX_MESH_QUEUE_SIZE 512
static chunk_mesh_job_t mesh_queue[MAX_MESH_QUEUE_SIZE];
static int mesh_queue_head = 0;
static int mesh_queue_tail = 0;
static int mesh_queue_count = 0;
static bool new_mesh_ready = false;

static bool enqueue_mesh_job(uint8_t x, uint8_t y, uint8_t z, bool needs_lighting) {
	pthread_mutex_lock(&mesh_queue_mutex);

	for (int i = 0; i < mesh_queue_count; i++) {
		int idx = (mesh_queue_head + i) % MAX_MESH_QUEUE_SIZE;
		if (mesh_queue[idx].x == x && mesh_queue[idx].y == y && mesh_queue[idx].z == z) {
			if (needs_lighting && !mesh_queue[idx].needs_lighting) {
				mesh_queue[idx].needs_lighting = true;
			}
			pthread_mutex_unlock(&mesh_queue_mutex);
			return true;
		}
	}

	if (mesh_queue_count < MAX_MESH_QUEUE_SIZE) {
		mesh_queue[mesh_queue_tail].x = x;
		mesh_queue[mesh_queue_tail].y = y;
		mesh_queue[mesh_queue_tail].z = z;
		mesh_queue[mesh_queue_tail].needs_lighting = needs_lighting;
		mesh_queue_tail = (mesh_queue_tail + 1) % MAX_MESH_QUEUE_SIZE;
		mesh_queue_count++;
		
		pthread_cond_signal(&mesh_queue_cond);
		pthread_mutex_unlock(&mesh_queue_mutex);
		return true;
	}
	
	pthread_mutex_unlock(&mesh_queue_mutex);
	return false;
}

static bool dequeue_mesh_job(chunk_mesh_job_t* job) {
	pthread_mutex_lock(&mesh_queue_mutex);
	
	if (mesh_queue_count == 0) {
		pthread_mutex_unlock(&mesh_queue_mutex);
		return false;
	}
	
	*job = mesh_queue[mesh_queue_head];
	mesh_queue_head = (mesh_queue_head + 1) % MAX_MESH_QUEUE_SIZE;
	mesh_queue_count--;
	
	pthread_mutex_unlock(&mesh_queue_mutex);
	return true;
}

static void* mesh_thread_worker(void* arg) {
	chunk_mesh_job_t job;
	
	while (!mesh_thread_should_exit) {
		pthread_mutex_lock(&mesh_queue_mutex);
		while (mesh_queue_count == 0 && !mesh_thread_should_exit) {
			pthread_cond_wait(&mesh_queue_cond, &mesh_queue_mutex);
		}
		pthread_mutex_unlock(&mesh_queue_mutex);
		
		if (mesh_thread_should_exit) break;

		while (dequeue_mesh_job(&job)) {
			pthread_mutex_lock(&chunks_mutex);
			
			Chunk* chunk = &chunks[job.x][job.y][job.z];

			if (chunk->needs_update && are_all_neighbors_loaded(job.x, job.y, job.z)) {
				if (job.needs_lighting) {
					set_chunk_lighting(chunk);
				}
				generate_chunk_mesh(chunk);
				chunk->needs_update = false;

				pthread_mutex_lock(&mesh_ready_mutex);
				new_mesh_ready = true;
				pthread_mutex_unlock(&mesh_ready_mutex);
			}
			
			pthread_mutex_unlock(&chunks_mutex);
		}
	}
	
	return NULL;
}

bool init_mesh_thread() {
	if (mesh_thread_running) return true;
	
	mesh_thread_should_exit = false;
	
	if (pthread_create(&mesh_thread, NULL, mesh_thread_worker, NULL) != 0) {
		return false;
	}
	
	mesh_thread_running = true;
	return true;
}

void cleanup_mesh_thread() {
	if (!mesh_thread_running) return;

	pthread_mutex_lock(&mesh_queue_mutex);
	mesh_thread_should_exit = true;
	pthread_cond_signal(&mesh_queue_cond);
	pthread_mutex_unlock(&mesh_queue_mutex);

	pthread_join(mesh_thread, NULL);
	mesh_thread_running = false;
}

void rebuild_combined_visible_mesh() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE, false);
	#endif
	
	pthread_mutex_lock(&chunks_mutex);
	
	opaque_index_count = 0;
	transparent_index_count = 0;

	uint32_t total_opaque_vertices = 0;
	uint32_t total_opaque_indices = 0;
	uint32_t total_transparent_vertices = 0;
	uint32_t total_transparent_indices = 0;

	static Vertex* opaque_vertices = NULL;
	static uint32_t* opaque_indices = NULL;
	static Vertex* transparent_vertices = NULL;
	static uint32_t* transparent_indices = NULL;
	static size_t current_opaque_capacity = 0;
	static size_t current_transparent_capacity = 0;

	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t x = 0; x < settings.render_distance; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < settings.render_distance; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (!(visibility_map[x][y][z] & (1 << face)) || !chunk->is_loaded) continue;
					
					total_opaque_vertices += chunk->faces[face].vertex_count;
					total_opaque_indices += chunk->faces[face].index_count;
					total_transparent_vertices += chunk->transparent_faces[face].vertex_count;
					total_transparent_indices += chunk->transparent_faces[face].index_count;
				}
			}
		}
	}

	if (total_opaque_vertices == 0 && total_transparent_vertices == 0) {
		mesh_needs_rebuild = false;
		pthread_mutex_unlock(&chunks_mutex);
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_MERGE, false);
		#endif
		return;
	}

	if (total_opaque_vertices > current_opaque_capacity) {
		free(opaque_vertices);
		free(opaque_indices);
		opaque_vertices = malloc(total_opaque_vertices * sizeof(Vertex));
		opaque_indices = malloc(total_opaque_indices * sizeof(uint32_t));
		current_opaque_capacity = total_opaque_vertices;
	}

	if (total_transparent_vertices > current_transparent_capacity) {
		free(transparent_vertices);
		free(transparent_indices);
		transparent_vertices = malloc(total_transparent_vertices * sizeof(Vertex));
		transparent_indices = malloc(total_transparent_indices * sizeof(uint32_t));
		current_transparent_capacity = total_transparent_vertices;
	}

	uint32_t opaque_vertex_offset = 0;
	uint32_t opaque_index_offset = 0;
	uint32_t opaque_base_vertex = 0;
	uint32_t transparent_vertex_offset = 0;
	uint32_t transparent_index_offset = 0;
	uint32_t transparent_base_vertex = 0;

	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t x = 0; x < settings.render_distance; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < settings.render_distance; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (!(visibility_map[x][y][z] & (1 << face)) || !chunk->is_loaded) continue;

					if (chunk->faces[face].vertex_count > 0) {
						memcpy(opaque_vertices + opaque_vertex_offset, 
							chunk->faces[face].vertices,
							chunk->faces[face].vertex_count * sizeof(Vertex));

						for (uint32_t i = 0; i < chunk->faces[face].index_count; i++) {
							opaque_indices[opaque_index_offset + i] = 
								chunk->faces[face].indices[i] + opaque_base_vertex;
						}

						opaque_vertex_offset += chunk->faces[face].vertex_count;
						opaque_index_offset += chunk->faces[face].index_count;
						opaque_base_vertex += chunk->faces[face].vertex_count;
					}


					if (chunk->transparent_faces[face].vertex_count > 0) {
						memcpy(transparent_vertices + transparent_vertex_offset,
							chunk->transparent_faces[face].vertices,
							chunk->transparent_faces[face].vertex_count * sizeof(Vertex));

						for (uint32_t i = 0; i < chunk->transparent_faces[face].index_count; i++) {
							transparent_indices[transparent_index_offset + i] = 
								chunk->transparent_faces[face].indices[i] + transparent_base_vertex;
						}

						transparent_vertex_offset += chunk->transparent_faces[face].vertex_count;
						transparent_index_offset += chunk->transparent_faces[face].index_count;
						transparent_base_vertex += chunk->transparent_faces[face].vertex_count;
					}
				}
			}
		}
	}

	if (total_opaque_vertices > 0) {
		opaque_index_count = total_opaque_indices;
		glBindVertexArray(opaque_VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, opaque_VBO);
		glBufferData(GL_ARRAY_BUFFER, total_opaque_vertices * sizeof(Vertex), opaque_vertices, GL_STATIC_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_opaque_indices * sizeof(uint32_t), opaque_indices, GL_STATIC_DRAW);
	}

	if (total_transparent_vertices > 0) {
		transparent_index_count = total_transparent_indices;
		glBindVertexArray(transparent_VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBO);
		glBufferData(GL_ARRAY_BUFFER, total_transparent_vertices * sizeof(Vertex), transparent_vertices, GL_STATIC_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_transparent_indices * sizeof(uint32_t), transparent_indices, GL_STATIC_DRAW);
	}

	glBindVertexArray(0);
	mesh_needs_rebuild = false;
	pthread_mutex_unlock(&chunks_mutex);
	
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MERGE, false);
	#endif
}

void process_chunks() {	
	pthread_mutex_lock(&mesh_ready_mutex);
	if (new_mesh_ready || mesh_needs_rebuild) {
		new_mesh_ready = false;
		pthread_mutex_unlock(&mesh_ready_mutex);
		rebuild_combined_visible_mesh();
	} else {
		pthread_mutex_unlock(&mesh_ready_mutex);
	}
	
	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				Chunk* chunk = &chunks[x][y][z];
				
				if (chunk->needs_update && are_all_neighbors_loaded(x, y, z)) {
					enqueue_mesh_job(x, y, z, true);
				}
			}
		}
	}
	pthread_mutex_unlock(&chunks_mutex);
}