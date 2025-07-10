#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

static pthread_t mesh_thread;
static pthread_mutex_t mesh_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mesh_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mesh_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mesh_ready_cond = PTHREAD_COND_INITIALIZER;
static _Atomic bool mesh_thread_running = false;
static _Atomic bool mesh_thread_should_exit = false;

typedef struct {
	uint8_t x, y, z;
	bool needs_lighting;
	uint32_t timestamp;
} chunk_mesh_job_t;

#define MAX_MESH_QUEUE_SIZE 512
static chunk_mesh_job_t mesh_queue[MAX_MESH_QUEUE_SIZE];
static int mesh_queue_head = 0;
static int mesh_queue_tail = 0;
static int mesh_queue_count = 0;
static _Atomic bool new_mesh_ready = false;
static uint32_t job_counter = 0;

static bool enqueue_mesh_job(uint8_t x, uint8_t y, uint8_t z, bool needs_lighting) {
	int result = pthread_mutex_lock(&mesh_queue_mutex);
	if (result != 0) {
		fprintf(stderr, "Failed to lock mesh queue mutex: %s\n", strerror(result));
		return false;
	}

	for (int i = 0; i < mesh_queue_count; i++) {
		int idx = (mesh_queue_head + i) % MAX_MESH_QUEUE_SIZE;
		if (mesh_queue[idx].x == x && mesh_queue[idx].y == y && mesh_queue[idx].z == z) {
			if (needs_lighting && !mesh_queue[idx].needs_lighting) {
				mesh_queue[idx].needs_lighting = true;
				mesh_queue[idx].timestamp = ++job_counter;
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
		mesh_queue[mesh_queue_tail].timestamp = ++job_counter;
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
	int result = pthread_mutex_lock(&mesh_queue_mutex);
	if (result != 0) {
		fprintf(stderr, "Failed to lock mesh queue mutex: %s\n", strerror(result));
		return false;
	}
	
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
	
	while (!atomic_load(&mesh_thread_should_exit)) {
		struct timespec timeout;
		clock_gettime(CLOCK_REALTIME, &timeout);
		timeout.tv_sec += 1;
		
		int result = pthread_mutex_lock(&mesh_queue_mutex);
		if (result != 0) {
			fprintf(stderr, "Mesh thread mutex lock failed: %s\n", strerror(result));
			break;
		}
		
		while (mesh_queue_count == 0 && !atomic_load(&mesh_thread_should_exit)) {
			result = pthread_cond_timedwait(&mesh_queue_cond, &mesh_queue_mutex, &timeout);
			if (result == ETIMEDOUT) {
				pthread_mutex_unlock(&mesh_queue_mutex);
				continue;
			}
			else if (result != 0) {
				fprintf(stderr, "Mesh thread cond wait failed: %s\n", strerror(result));
				pthread_mutex_unlock(&mesh_queue_mutex);
				goto exit_thread;
			}
		}
		pthread_mutex_unlock(&mesh_queue_mutex);
		
		if (atomic_load(&mesh_thread_should_exit)) break;

		while (dequeue_mesh_job(&job)) {
			result = pthread_mutex_lock(&chunks_mutex);
			if (result != 0) {
				fprintf(stderr, "Failed to lock chunks mutex: %s\n", strerror(result));
				continue;
			}

			if (job.x >= settings.render_distance || job.y >= WORLD_HEIGHT || job.z >= settings.render_distance) {
				pthread_mutex_unlock(&chunks_mutex);
				continue;
			}
			
			Chunk* chunk = &chunks[job.x][job.y][job.z];

			if (chunk->needs_update && are_all_neighbors_loaded(job.x, job.y, job.z)) {
				if (job.needs_lighting)
					set_chunk_lighting(chunk);

				generate_chunk_mesh(chunk);
				chunk->needs_update = false;

				pthread_mutex_lock(&mesh_ready_mutex);
				atomic_store(&new_mesh_ready, true);
				pthread_cond_signal(&mesh_ready_cond);
				pthread_mutex_unlock(&mesh_ready_mutex);
			}
			
			pthread_mutex_unlock(&chunks_mutex);
		}
	}
	
exit_thread:
	atomic_store(&mesh_thread_running, false);
	return NULL;
}

bool init_mesh_thread() {
	if (atomic_load(&mesh_thread_running)) return true;
	
	atomic_store(&mesh_thread_should_exit, false);
	
	int result = pthread_create(&mesh_thread, NULL, mesh_thread_worker, NULL);
	if (result != 0) {
		fprintf(stderr, "Failed to create mesh thread: %s\n", strerror(result));
		return false;
	}
	
	atomic_store(&mesh_thread_running, true);
	return true;
}

void cleanup_mesh_thread() {
	if (!atomic_load(&mesh_thread_running)) return;

	atomic_store(&mesh_thread_should_exit, true);

	pthread_mutex_lock(&mesh_queue_mutex);
	pthread_cond_broadcast(&mesh_queue_cond);
	pthread_mutex_unlock(&mesh_queue_mutex);
	
	pthread_mutex_lock(&mesh_ready_mutex);
	pthread_cond_broadcast(&mesh_ready_cond);
	pthread_mutex_unlock(&mesh_ready_mutex);

	int result = pthread_join(mesh_thread, NULL);
	if (result != 0) {
		fprintf(stderr, "Failed to join mesh thread: %s\n", strerror(result));
	}
	
	atomic_store(&mesh_thread_running, false);
}

void rebuild_combined_visible_mesh() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE, false);
	#endif
	
	int result = pthread_mutex_lock(&chunks_mutex);
	if (result != 0) {
		fprintf(stderr, "Failed to lock chunks mutex in rebuild: %s\n", strerror(result));
		return;
	}
	
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

	// Calculate total vertex/index counts
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

	// Reallocate buffers if needed with safety checks
	if (total_opaque_vertices > current_opaque_capacity) {
		Vertex* new_opaque_vertices = realloc(opaque_vertices, total_opaque_vertices * sizeof(Vertex));
		uint32_t* new_opaque_indices = realloc(opaque_indices, total_opaque_indices * sizeof(uint32_t));
		
		if (!new_opaque_vertices || !new_opaque_indices) {
			fprintf(stderr, "Failed to allocate memory for opaque mesh\n");
			free(new_opaque_vertices);
			free(new_opaque_indices);
			pthread_mutex_unlock(&chunks_mutex);
			return;
		}
		
		opaque_vertices = new_opaque_vertices;
		opaque_indices = new_opaque_indices;
		current_opaque_capacity = total_opaque_vertices;
	}

	if (total_transparent_vertices > current_transparent_capacity) {
		Vertex* new_transparent_vertices = realloc(transparent_vertices, total_transparent_vertices * sizeof(Vertex));
		uint32_t* new_transparent_indices = realloc(transparent_indices, total_transparent_indices * sizeof(uint32_t));
		
		if (!new_transparent_vertices || !new_transparent_indices) {
			fprintf(stderr, "Failed to allocate memory for transparent mesh\n");
			free(new_transparent_vertices);
			free(new_transparent_indices);
			pthread_mutex_unlock(&chunks_mutex);
			return;
		}
		
		transparent_vertices = new_transparent_vertices;
		transparent_indices = new_transparent_indices;
		current_transparent_capacity = total_transparent_vertices;
	}

	uint32_t opaque_vertex_offset = 0;
	uint32_t opaque_index_offset = 0;
	uint32_t opaque_base_vertex = 0;
	uint32_t transparent_vertex_offset = 0;
	uint32_t transparent_index_offset = 0;
	uint32_t transparent_base_vertex = 0;

	// Copy vertex and index data
	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t x = 0; x < settings.render_distance; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < settings.render_distance; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (!(visibility_map[x][y][z] & (1 << face)) || !chunk->is_loaded) continue;

					// Copy opaque mesh data
					if (chunk->faces[face].vertex_count > 0) {
						if (opaque_vertex_offset + chunk->faces[face].vertex_count > total_opaque_vertices ||
							opaque_index_offset + chunk->faces[face].index_count > total_opaque_indices) {
							fprintf(stderr, "Buffer overflow detected in opaque mesh\n");
							continue;
						}

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

					// Copy transparent mesh data
					if (chunk->transparent_faces[face].vertex_count > 0) {
						if (transparent_vertex_offset + chunk->transparent_faces[face].vertex_count > total_transparent_vertices ||
							transparent_index_offset + chunk->transparent_faces[face].index_count > total_transparent_indices) {
							fprintf(stderr, "Buffer overflow detected in transparent mesh\n");
							continue;
						}

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

	// Upload to GPU
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
	// Check if mesh rebuild is needed
	pthread_mutex_lock(&mesh_ready_mutex);
	bool should_rebuild = atomic_load(&new_mesh_ready) || mesh_needs_rebuild;
	if (should_rebuild) {
		atomic_store(&new_mesh_ready, false);
		pthread_mutex_unlock(&mesh_ready_mutex);
		rebuild_combined_visible_mesh();
	} else {
		pthread_mutex_unlock(&mesh_ready_mutex);
	}
	
	// Queue mesh generation jobs
	int result = pthread_mutex_lock(&chunks_mutex);
	if (result != 0) {
		fprintf(stderr, "Failed to lock chunks mutex in process_chunks: %s\n", strerror(result));
		return;
	}
	
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				Chunk* chunk = &chunks[x][y][z];
				
				if (chunk->needs_update && are_all_neighbors_loaded(x, y, z)) {
					if (!enqueue_mesh_job(x, y, z, true)) {
						#ifdef DEBUG
						printf("Warning: Mesh queue full, dropping job for chunk (%d,%d,%d)\n", x, y, z);
						#endif
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&chunks_mutex);
}