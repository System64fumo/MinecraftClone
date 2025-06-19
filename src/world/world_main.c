#include "main.h"
#include "world.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

int last_cx = -1;
int last_cy = -1;
int last_cz = -1;

// Threading variables
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

void init_world_gen_tracker(void) {
	pthread_mutex_init(&world_gen_tracker.mutex, NULL);
	world_gen_tracker.tracking_active = false;
	world_gen_tracker.chunks_queued = 0;
	world_gen_tracker.chunks_completed = 0;
}

void cleanup_world_gen_tracker(void) {
	pthread_mutex_destroy(&world_gen_tracker.mutex);
}

void start_world_gen_tracking(void) {
	pthread_mutex_lock(&world_gen_tracker.mutex);
	world_gen_tracker.tracking_active = true;
	world_gen_tracker.chunks_queued = 0;
	world_gen_tracker.chunks_completed = 0;
	pthread_mutex_unlock(&world_gen_tracker.mutex);
}

void track_chunk_queued(void) {
	pthread_mutex_lock(&world_gen_tracker.mutex);
	if (world_gen_tracker.tracking_active) {
		world_gen_tracker.chunks_queued++;
	}
	pthread_mutex_unlock(&world_gen_tracker.mutex);
}

void track_chunk_completed(void) {
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

	for (int i = chunk_load_queue.size - 1; i > 0; i--) {
		if (chunk_load_queue.requests[i].priority < chunk_load_queue.requests[i-1].priority) {
			chunk_load_request_t tmp = chunk_load_queue.requests[i];
			chunk_load_queue.requests[i] = chunk_load_queue.requests[i-1];
			chunk_load_queue.requests[i-1] = tmp;
		}
		else {
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

		for (uint8_t face = 0; face < 6; face++) {
			if (req.ci_x > 0) {
				if (chunks[req.ci_x-1][req.ci_y][req.ci_z].faces[face].vertices)
					chunks[req.ci_x-1][req.ci_y][req.ci_z].needs_update = true;
			}
			if (req.ci_x < RENDER_DISTANCE-1) {
				if (chunks[req.ci_x+1][req.ci_y][req.ci_z].faces[face].vertices)
					chunks[req.ci_x+1][req.ci_y][req.ci_z].needs_update = true;
			}
			if (req.ci_z > 0) {
				if (chunks[req.ci_x][req.ci_y][req.ci_z-1].faces[face].vertices)
					chunks[req.ci_x][req.ci_y][req.ci_z-1].needs_update = true;
			}
			if (req.ci_z < RENDER_DISTANCE-1) {
				if (chunks[req.ci_x][req.ci_y][req.ci_z+1].faces[face].vertices)
					chunks[req.ci_x][req.ci_y][req.ci_z+1].needs_update = true;
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
	free(chunk_load_queue.requests);
}

void load_around_entity(Entity* entity) {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_WORLD_GEN, false);
	start_world_gen_tracking();
	#endif

	int center_cx = floorf(entity->pos.x / CHUNK_SIZE) - (RENDER_DISTANCE / 2);
	last_cy = floorf(entity->pos.y / CHUNK_SIZE) - (WORLD_HEIGHT / 2);
	int center_cz = floorf(entity->pos.z / CHUNK_SIZE) - (RENDER_DISTANCE / 2);

	int dx = center_cx - last_cx;
	int dz = center_cz - last_cz;

	last_cx = center_cx;
	last_cz = center_cz;

	if (dx == 0 && dz == 0) {
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
		#endif
		return;
	}

	Chunk*** temp_chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);
	if (!temp_chunks) {
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
		#endif
		return;
	}

	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			memcpy(temp_chunks[x][y], chunks[x][y], RENDER_DISTANCE * sizeof(Chunk));
		}
	}
	pthread_mutex_unlock(&chunks_mutex);

	// Process old chunks (east/west movement)
	if (dx != 0) {
		int start_x = (dx > 0) ? 0 : RENDER_DISTANCE + dx;
		int end_x = (dx > 0) ? dx : RENDER_DISTANCE;
		
		for (uint8_t x = start_x; x < end_x; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					bool has_data = false;
					for (uint8_t face = 0; face < 6; face++) {
						if (temp_chunks[x][y][z].faces[face].vertices) {
							has_data = true;
							break;
						}
					}
					
					if (has_data) {
						if ((dx > 0 && x == dx - 1 && x + 1 < RENDER_DISTANCE) ||
							(dx < 0 && x == RENDER_DISTANCE + dx && x - 1 >= 0)) {
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
		int start_z = (dz > 0) ? 0 : RENDER_DISTANCE + dz;
		int end_z = (dz > 0) ? dz : RENDER_DISTANCE;
		
		for (int x = 0; x < RENDER_DISTANCE; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = start_z; z < end_z; z++) {
					bool has_data = false;
					for (int face = 0; face < 6; face++) {
						if (temp_chunks[x][y][z].faces[face].vertices) {
							has_data = true;
							break;
						}
					}
					
					if (has_data) {
						if ((dz > 0 && z == dz - 1 && z + 1 < RENDER_DISTANCE) ||
							(dz < 0 && z == RENDER_DISTANCE + dz && z - 1 >= 0)) {
							temp_chunks[x][y][z + (dz > 0 ? 1 : -1)].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}

	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			memset(chunks[x][y], 0, RENDER_DISTANCE * sizeof(Chunk));
		}
	}

	// Move surviving chunks
	for (int x = 0; x < RENDER_DISTANCE; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < RENDER_DISTANCE; z++) {
				int old_x = x + dx;
				int old_z = z + dz;
				
				if (old_x >= 0 && old_x < RENDER_DISTANCE && 
					old_z >= 0 && old_z < RENDER_DISTANCE) {
					bool has_data = false;
					for (int face = 0; face < 6; face++) {
						if (temp_chunks[old_x][y][old_z].faces[face].vertices) {
							has_data = true;
							break;
						}
					}
					
					if (has_data) {
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
	
	// Enqueue chunks that need to be loaded
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				pthread_mutex_lock(&chunks_mutex);
				bool needs_load = true;
				for (uint8_t face = 0; face < 6; face++) {
					if (chunks[x][y][z].faces[face].vertices) {
						needs_load = false;
						break;
					}
				}
				pthread_mutex_unlock(&chunks_mutex);
				
				if (needs_load) {
					float world_chunk_x = (x + center_cx);
					float world_chunk_y = (y + last_cy);
					float world_chunk_z = (z + center_cz);

					float dx = world_chunk_x - entity_chunk_x;
					float dy = world_chunk_y - entity_chunk_y;
					float dz = world_chunk_z - entity_chunk_z;
					float dist_sq = dx*dx + dy*dy + dz*dz;

					enqueue_chunk_load(x, y, z, x + center_cx, y, z + center_cz, dist_sq);
					track_chunk_queued();
				}
			}
		}
	}

	pthread_mutex_lock(&world_gen_tracker.mutex);
	if (world_gen_tracker.chunks_queued == 0) {
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
		#endif
	}
	pthread_mutex_unlock(&world_gen_tracker.mutex);
	
	free_chunks(temp_chunks, RENDER_DISTANCE, WORLD_HEIGHT);
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
	if (!chunk) return;

	for (uint8_t face = 0; face < 6; face++) {
		if (chunk->faces[face].vertices) {
			free(chunk->faces[face].vertices);
			chunk->faces[face].vertices = NULL;
		}
		if (chunk->faces[face].indices) {
			free(chunk->faces[face].indices);
			chunk->faces[face].indices = NULL;
		}
		if (chunk->transparent_faces[face].vertices) {
			free(chunk->transparent_faces[face].vertices);
			chunk->transparent_faces[face].vertices = NULL;
		}
		if (chunk->transparent_faces[face].indices) {
			free(chunk->transparent_faces[face].indices);
			chunk->transparent_faces[face].indices = NULL;
		}
	}
}