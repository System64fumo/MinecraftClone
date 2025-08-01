#include "main.h"
#include "entity.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

_Atomic int world_offset_x = 0;
_Atomic int world_offset_z = 0;
int last_cx = -1;
int last_cy = -1;
int last_cz = -1;

typedef struct {
	uint8_t ci_x, ci_y, ci_z;
	int cx, cy, cz;
	float priority;	
} chunk_load_request_t;

typedef struct {
	chunk_load_request_t* requests;
	int capacity;
	int size;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} chunk_load_queue_t;

typedef struct {
	bool tracking_active;
	uint32_t chunks_queued;
	uint32_t chunks_completed;
	pthread_mutex_t mutex;
} world_gen_tracker_t;

chunk_load_queue_t chunk_load_queue;
pthread_t world_gen_thread;
bool world_gen_thread_running = false;
pthread_mutex_t chunks_mutex = PTHREAD_MUTEX_INITIALIZER;
world_gen_tracker_t world_gen_tracker = {0};

bool* chunk_needs_load_cache = NULL;
size_t cache_size = 0;

void init_world_gen_tracker() {
	pthread_mutex_init(&world_gen_tracker.mutex, NULL);
	world_gen_tracker.tracking_active = false;
	world_gen_tracker.chunks_queued = 0;
	world_gen_tracker.chunks_completed = 0;
}

void cleanup_world_gen_tracker() {
	pthread_mutex_destroy(&world_gen_tracker.mutex);
}

void start_world_gen_tracking() {
	pthread_mutex_lock(&world_gen_tracker.mutex);
	world_gen_tracker.tracking_active = true;
	world_gen_tracker.chunks_queued = 0;
	world_gen_tracker.chunks_completed = 0;
	pthread_mutex_unlock(&world_gen_tracker.mutex);
}

void track_chunk_queued() {
	pthread_mutex_lock(&world_gen_tracker.mutex);
	if (world_gen_tracker.tracking_active) {
		world_gen_tracker.chunks_queued++;
	}
	pthread_mutex_unlock(&world_gen_tracker.mutex);
}

void track_chunk_completed() {
	pthread_mutex_lock(&world_gen_tracker.mutex);
	if (world_gen_tracker.tracking_active) {
		world_gen_tracker.chunks_completed++;

		if (world_gen_tracker.chunks_completed >= world_gen_tracker.chunks_queued && 
			world_gen_tracker.chunks_queued > 0) {
			world_gen_tracker.tracking_active = false;
			#ifdef DEBUG
			profiler_stop(PROFILER_ID_WORLD_GEN, false);
			#endif
		}
	}
	pthread_mutex_unlock(&world_gen_tracker.mutex);
}

void init_chunk_load_queue() {
	chunk_load_queue.capacity = 128;
	chunk_load_queue.size = 0;
	chunk_load_queue.requests = malloc(chunk_load_queue.capacity * sizeof(chunk_load_request_t));
	pthread_mutex_init(&chunk_load_queue.mutex, NULL);
	pthread_cond_init(&chunk_load_queue.cond, NULL);

	size_t total_chunks = settings.render_distance * WORLD_HEIGHT * settings.render_distance;
	if (cache_size < total_chunks) {
		chunk_needs_load_cache = realloc(chunk_needs_load_cache, total_chunks * sizeof(bool));
		cache_size = total_chunks;
	}
}

static inline bool chunk_needs_loading(uint8_t x, uint8_t y, uint8_t z) {
	for (uint8_t face = 0; face < 6; face++) {
		if (chunks[x][y][z].faces[face].vertices) {
			return false;
		}
	}
	return true;
}

void enqueue_chunk_load(int ci_x, int ci_y, int ci_z, int cx, int cy, int cz, float priority) {
	pthread_mutex_lock(&chunk_load_queue.mutex);

	if (chunk_load_queue.size >= chunk_load_queue.capacity) {
		chunk_load_queue.capacity *= 2;
		chunk_load_queue.requests = realloc(chunk_load_queue.requests, 
										 chunk_load_queue.capacity * sizeof(chunk_load_request_t));
	}

	chunk_load_request_t* req = &chunk_load_queue.requests[chunk_load_queue.size++];
	req->ci_x = ci_x;
	req->ci_y = ci_y;
	req->ci_z = ci_z;
	req->cx = cx;
	req->cy = cy;
	req->cz = cz;
	req->priority = priority;

	int insert_pos = chunk_load_queue.size - 1;
	int max_sort_depth = 16;
	
	for (int i = insert_pos; i > 0 && i > insert_pos - max_sort_depth; i--) {
		if (chunk_load_queue.requests[i].priority < chunk_load_queue.requests[i-1].priority) {
			chunk_load_request_t tmp = chunk_load_queue.requests[i];
			chunk_load_queue.requests[i] = chunk_load_queue.requests[i-1];
			chunk_load_queue.requests[i-1] = tmp;
		} else {
			break;
		}
	}
	
	pthread_cond_signal(&chunk_load_queue.cond);
	pthread_mutex_unlock(&chunk_load_queue.mutex);
}

void* world_gen_thread_func(void* arg) {
	while (world_gen_thread_running) {
		pthread_mutex_lock(&chunk_load_queue.mutex);

		while (chunk_load_queue.size == 0 && world_gen_thread_running) {
			pthread_cond_wait(&chunk_load_queue.cond, &chunk_load_queue.mutex);
		}
		
		if (!world_gen_thread_running) {
			pthread_mutex_unlock(&chunk_load_queue.mutex);
			break;
		}

		chunk_load_request_t req = chunk_load_queue.requests[0];

		for (int i = 1; i < chunk_load_queue.size; i++) {
			chunk_load_queue.requests[i-1] = chunk_load_queue.requests[i];
		}
		chunk_load_queue.size--;
		
		pthread_mutex_unlock(&chunk_load_queue.mutex);

		Chunk temp_chunk = {0};
		load_chunk_data(&temp_chunk, req.ci_x, req.ci_y, req.ci_z, req.cx, req.cy, req.cz);

		pthread_mutex_lock(&chunks_mutex);
		chunks[req.ci_x][req.ci_y][req.ci_z] = temp_chunk;
		chunks[req.ci_x][req.ci_y][req.ci_z].is_loaded = true;

		const int neighbors[][3] = {{-1,0,0}, {1,0,0}, {0,0,-1}, {0,0,1}};
		for (int n = 0; n < 4; n++) {
			int nx = req.ci_x + neighbors[n][0];
			int nz = req.ci_z + neighbors[n][2];
			
			if (nx >= 0 && nx < settings.render_distance && 
				nz >= 0 && nz < settings.render_distance) {
				
				bool has_vertices = false;
				for (uint8_t face = 0; face < 6 && !has_vertices; face++) {
					if (chunks[nx][req.ci_y][nz].faces[face].vertices) {
						has_vertices = true;
					}
				}
				if (has_vertices) {
					chunks[nx][req.ci_y][nz].needs_update = true;
				}
			}
		}
		
		pthread_mutex_unlock(&chunks_mutex);
		track_chunk_completed();
	}
	return NULL;
}

void start_world_gen_thread() {
	init_chunk_load_queue();
	init_world_gen_tracker();
	world_gen_thread_running = true;
	pthread_create(&world_gen_thread, NULL, world_gen_thread_func, NULL);
}

void stop_world_gen_thread() {
	world_gen_thread_running = false;
	pthread_cond_signal(&chunk_load_queue.cond);
	pthread_join(world_gen_thread, NULL);
	
	pthread_mutex_destroy(&chunk_load_queue.mutex);
	pthread_cond_destroy(&chunk_load_queue.cond);
	
	if (chunk_needs_load_cache) {
		free(chunk_needs_load_cache);
		chunk_needs_load_cache = NULL;
		cache_size = 0;
	}
}

static void swap_pending_chunks(chunk_load_request_t* a, chunk_load_request_t* b) {
	chunk_load_request_t temp = *a;
	*a = *b;
	*b = temp;
}

static int partition_chunks(chunk_load_request_t* chunks, int low, int high) {
	float pivot = chunks[high].priority;
	int i = low - 1;
	
	for (int j = low; j < high; j++) {
		if (chunks[j].priority < pivot) {
			i++;
			swap_pending_chunks(&chunks[i], &chunks[j]);
		}
	}
	swap_pending_chunks(&chunks[i + 1], &chunks[high]);
	return i + 1;
}

static void quicksort_chunks(chunk_load_request_t* chunks, int low, int high) {
	if (low < high) {
		int pi = partition_chunks(chunks, low, high);
		quicksort_chunks(chunks, low, pi - 1);
		quicksort_chunks(chunks, pi + 1, high);
	}
}

static bool chunk_has_data(Chunk* chunk) {
	if (!chunk) return false;
	for (uint8_t face = 0; face < 6; face++) {
		if (chunk->faces[face].vertices) {
			return true;
		}
	}
	return false;
}

void move_world(int x, int y, int z) {
	// Calculate the actual movement amounts in world units
	float move_x = x * CHUNK_SIZE;
	float move_y = y * CHUNK_SIZE;
	float move_z = z * CHUNK_SIZE;
	
	pthread_mutex_lock(&chunks_mutex);
	
	// Iterate through all chunks
	for (uint8_t ci_x = 0; ci_x < settings.render_distance; ci_x++) {
		for (uint8_t ci_y = 0; ci_y < WORLD_HEIGHT; ci_y++) {
			for (uint8_t ci_z = 0; ci_z < settings.render_distance; ci_z++) {
				Chunk* chunk = &chunks[ci_x][ci_y][ci_z];
				
				// Update the chunk's world position
				chunk->x += x;
				chunk->y += y;
				chunk->z += z;
				
				// Update all vertices in the chunk's mesh
				for (uint8_t face = 0; face < 6; face++) {
					// Update opaque faces
					if (chunk->faces[face].vertices) {
						for (uint32_t v = 0; v < chunk->faces[face].vertex_count; v++) {
							chunk->faces[face].vertices[v].x += move_x;
							chunk->faces[face].vertices[v].y += move_y;
							chunk->faces[face].vertices[v].z += move_z;
						}
					}
					
					// Update transparent faces
					if (chunk->transparent_faces[face].vertices) {
						for (uint32_t v = 0; v < chunk->transparent_faces[face].vertex_count; v++) {
							chunk->transparent_faces[face].vertices[v].x += move_x;
							chunk->transparent_faces[face].vertices[v].y += move_y;
							chunk->transparent_faces[face].vertices[v].z += move_z;
						}
					}
				}
			}
		}
	}
	mesh_needs_rebuild = true;
	pthread_mutex_unlock(&chunks_mutex);
}

void load_around_entity(Entity* entity) {
	int center_cx = floorf(entity->pos.x / CHUNK_SIZE) - (settings.render_distance / 2);
	int center_cz = floorf(entity->pos.z / CHUNK_SIZE) - (settings.render_distance / 2);

	int dx = center_cx - last_cx;
	int dz = center_cz - last_cz;

	if (last_cx == center_cx || last_cz == center_cz)
		return;

	#ifdef DEBUG
	profiler_start(PROFILER_ID_WORLD_GEN, false);
	start_world_gen_tracking();
	#endif

	last_cx = center_cx;
	last_cz = center_cz;

	atomic_store(&world_offset_x, center_cx);
	atomic_store(&world_offset_z, center_cz);

	if (dx == 0 && dz == 0) {
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
		#endif
		return;
	}

	Chunk*** temp_chunks = allocate_chunks();
	if (!temp_chunks) {
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
		#endif
		return;
	}

	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			memcpy(temp_chunks[x][y], chunks[x][y], settings.render_distance * sizeof(Chunk));
		}
	}
	pthread_mutex_unlock(&chunks_mutex);

	// Process old chunks (east/west movement)
	if (dx != 0) {
		int start_x = (dx > 0) ? 0 : settings.render_distance + dx;
		int end_x = (dx > 0) ? dx : settings.render_distance;
		
		for (uint8_t x = start_x; x < end_x; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < settings.render_distance; z++) {
					if (chunk_has_data(&temp_chunks[x][y][z])) {
						if ((dx > 0 && x == dx - 1 && x + 1 < settings.render_distance) ||
							(dx < 0 && x == settings.render_distance + dx && x - 1 >= 0)) {
							temp_chunks[x + (dx > 0 ? 1 : -1)][y][z].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}

	// Process old chunks (north/south movement)
	if (dz != 0) {
		int start_z = (dz > 0) ? 0 : settings.render_distance + dz;
		int end_z = (dz > 0) ? dz : settings.render_distance;
		
		for (int x = 0; x < settings.render_distance; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = start_z; z < end_z; z++) {
					if (chunk_has_data(&temp_chunks[x][y][z])) {
						if ((dz > 0 && z == dz - 1 && z + 1 < settings.render_distance) ||
							(dz < 0 && z == settings.render_distance + dz && z - 1 >= 0)) {
							temp_chunks[x][y][z + (dz > 0 ? 1 : -1)].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}

	// Clear and move surviving chunks
	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			memset(chunks[x][y], 0, settings.render_distance * sizeof(Chunk));
		}
	}

	for (int x = 0; x < settings.render_distance; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < settings.render_distance; z++) {
				int old_x = x + dx;
				int old_z = z + dz;
				
				if (old_x >= 0 && old_x < settings.render_distance && 
					old_z >= 0 && old_z < settings.render_distance) {
					if (chunk_has_data(&temp_chunks[old_x][y][old_z])) {
						chunks[x][y][z] = temp_chunks[old_x][y][old_z];
						chunks[x][y][z].ci_x = x;
						chunks[x][y][z].ci_z = z;
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&chunks_mutex);

	float entity_chunk_x = entity->pos.x / CHUNK_SIZE;
	float entity_chunk_y = entity->pos.y / CHUNK_SIZE;
	float entity_chunk_z = entity->pos.z / CHUNK_SIZE;
	
	int chunks_to_load = 0;
	
	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				size_t idx = x * WORLD_HEIGHT * settings.render_distance + y * settings.render_distance + z;
				chunk_needs_load_cache[idx] = chunk_needs_loading(x, y, z);
				if (chunk_needs_load_cache[idx]) {
					chunks_to_load++;
				}
			}
		}
	}
	pthread_mutex_unlock(&chunks_mutex);
	
	if (chunks_to_load > 0) {
		chunk_load_request_t* pending_chunks = malloc(chunks_to_load * sizeof(chunk_load_request_t));
		int pending_count = 0;
		
		for (uint8_t x = 0; x < settings.render_distance; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < settings.render_distance; z++) {
					size_t idx = x * WORLD_HEIGHT * settings.render_distance + y * settings.render_distance + z;
					
					if (chunk_needs_load_cache[idx]) {
						float world_chunk_x = (x + center_cx);
						float world_chunk_y = (y + last_cy);
						float world_chunk_z = (z + center_cz);

						float dx = world_chunk_x - entity_chunk_x;
						float dy = world_chunk_y - entity_chunk_y;
						float dz = world_chunk_z - entity_chunk_z;
						float dist_sq = dx*dx + dy*dy + dz*dz;

						pending_chunks[pending_count] = (chunk_load_request_t){
							.ci_x = x, .ci_y = y, .ci_z = z,
							.cx = x + center_cx, .cy = y, .cz = z + center_cz,
							.priority = dist_sq
						};
						pending_count++;
					}
				}
			}
		}

		if (pending_count > 1) {
			quicksort_chunks(pending_chunks, 0, pending_count - 1);
		}
		
		for (int i = 0; i < pending_count; i++) {
			enqueue_chunk_load(pending_chunks[i].ci_x, pending_chunks[i].ci_y, pending_chunks[i].ci_z,
							 pending_chunks[i].cx, pending_chunks[i].cy, pending_chunks[i].cz,
							 pending_chunks[i].priority);
			track_chunk_queued();
		}
		
		free(pending_chunks);
	}

	pthread_mutex_lock(&world_gen_tracker.mutex);
	if (world_gen_tracker.chunks_queued == 0) {
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
		#endif
	}
	pthread_mutex_unlock(&world_gen_tracker.mutex);
	free_chunks(temp_chunks);
}

void load_chunk_data(Chunk* chunk, unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz) {
	chunk->ci_x = ci_x;
	chunk->ci_y = ci_y;
	chunk->ci_z = ci_z;
	chunk->x = cx;
	chunk->y = cy;
	chunk->z = cz;
	chunk->needs_update = true;

	generate_chunk_terrain(chunk, cx, cy, cz);
}

void unload_chunk(Chunk* chunk) {
	if (chunk != NULL) return;

	for (uint8_t face = 0; face < 6; face++) {
		free(chunk->faces[face].vertices);
		free(chunk->faces[face].indices);
		free(chunk->transparent_faces[face].vertices);
		free(chunk->transparent_faces[face].indices);
		
		chunk->faces[face].vertices = NULL;
		chunk->faces[face].indices = NULL;
		chunk->transparent_faces[face].vertices = NULL;
		chunk->transparent_faces[face].indices = NULL;
	}
}