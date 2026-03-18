#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

void relight_chunk(Chunk *chunk);

static pthread_t mesh_thread;
static pthread_mutex_t mesh_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mesh_queue_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mesh_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t mesh_ready_cond = PTHREAD_COND_INITIALIZER;
static _Atomic bool mesh_thread_running = false;
static _Atomic bool mesh_thread_should_exit = false;

typedef struct {
	uint8_t x, y, z;
	uint32_t timestamp;
} chunk_mesh_job_t;

#define MAX_MESH_QUEUE_SIZE 4096
static chunk_mesh_job_t mesh_queue[MAX_MESH_QUEUE_SIZE];
static int mesh_queue_head = 0;
static int mesh_queue_tail = 0;
static int mesh_queue_count = 0;
static uint32_t job_counter = 0;

static bool enqueue_mesh_job(uint8_t x, uint8_t y, uint8_t z) {
	int result = pthread_mutex_lock(&mesh_queue_mutex);
	if (result != 0) {
		fprintf(stderr, "Failed to lock mesh queue mutex: %s\n", strerror(result));
		return false;
	}

	for (int i = 0; i < mesh_queue_count; i++) {
		int idx = (mesh_queue_head + i) % MAX_MESH_QUEUE_SIZE;
		if (mesh_queue[idx].x == x && mesh_queue[idx].y == y && mesh_queue[idx].z == z) {
			mesh_queue[idx].timestamp = ++job_counter;
			pthread_mutex_unlock(&mesh_queue_mutex);
			return true;
		}
	}

	if (mesh_queue_count < MAX_MESH_QUEUE_SIZE) {
		mesh_queue[mesh_queue_tail].x = x;
		mesh_queue[mesh_queue_tail].y = y;
		mesh_queue[mesh_queue_tail].z = z;
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
			} else if (result != 0) {
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

			if (chunk->needs_update) {
				if (chunk->lighting_changed && are_all_neighbors_loaded(job.x, job.y, job.z)) {
					chunk->lighting_changed = false;
					// Release mutex before BFS — it touches block data only, not chunk
					// structure. get/set helpers check is_loaded so unloads are safe.
					pthread_mutex_unlock(&chunks_mutex);
#ifdef DEBUG
					profiler_start(PROFILER_ID_RELIGHT, false);
#endif
					relight_chunk(chunk);
#ifdef DEBUG
					profiler_stop(PROFILER_ID_RELIGHT, false);
#endif
					pthread_mutex_lock(&chunks_mutex);
					// Re-validate chunk is still loaded after reacquiring.
					if (!chunk->is_loaded) {
						pthread_mutex_unlock(&chunks_mutex);
						continue;
					}
					static const int8_t ndx[] = { 1,-1, 0, 0, 0, 0 };
					static const int8_t ndy[] = { 0, 0, 1,-1, 0, 0 };
					static const int8_t ndz[] = { 0, 0, 0, 0, 1,-1 };
					for (int d = 0; d < 6; d++) {
						int nx2 = (int)job.x + ndx[d];
						int ny2 = (int)job.y + ndy[d];
						int nz2 = (int)job.z + ndz[d];
						if (nx2 < 0 || nx2 >= settings.render_distance) continue;
						if (ny2 < 0 || ny2 >= WORLD_HEIGHT) continue;
						if (nz2 < 0 || nz2 >= settings.render_distance) continue;
						Chunk* nc = &chunks[nx2][ny2][nz2];
						if (nc->is_loaded && !nc->needs_update)
							nc->needs_update = true;
					}
				}
				// Release mutex during mesh build — only reads blocks[], writes faces[].
				chunk->needs_update = false;
				pthread_mutex_unlock(&chunks_mutex);
#ifdef DEBUG
				profiler_start(PROFILER_ID_MESH, false);
#endif
				generate_chunk_mesh(chunk);
#ifdef DEBUG
				profiler_stop(PROFILER_ID_MESH, false);
#endif
				chunk->mesh_dirty = true;
				atomic_store(&mesh_needs_rebuild, true);
				continue; // mutex already unlocked
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

void enqueue_chunk_update_priority(uint8_t x, uint8_t y, uint8_t z) {
	pthread_mutex_lock(&mesh_queue_mutex);
	// Remove any existing entry for this chunk.
	for (int i = 0; i < mesh_queue_count; i++) {
		int idx = (mesh_queue_head + i) % MAX_MESH_QUEUE_SIZE;
		if (mesh_queue[idx].x == x && mesh_queue[idx].y == y && mesh_queue[idx].z == z) {
			for (int j = i; j < mesh_queue_count - 1; j++) {
				int cur  = (mesh_queue_head + j)     % MAX_MESH_QUEUE_SIZE;
				int next = (mesh_queue_head + j + 1) % MAX_MESH_QUEUE_SIZE;
				mesh_queue[cur] = mesh_queue[next];
			}
			mesh_queue_count--;
			mesh_queue_tail = (mesh_queue_tail - 1 + MAX_MESH_QUEUE_SIZE) % MAX_MESH_QUEUE_SIZE;
			break;
		}
	}
	if (mesh_queue_count < MAX_MESH_QUEUE_SIZE) {
		mesh_queue_head = (mesh_queue_head - 1 + MAX_MESH_QUEUE_SIZE) % MAX_MESH_QUEUE_SIZE;
		mesh_queue[mesh_queue_head].x = x;
		mesh_queue[mesh_queue_head].y = y;
		mesh_queue[mesh_queue_head].z = z;
		mesh_queue[mesh_queue_head].timestamp = ++job_counter;
		mesh_queue_count++;
		pthread_cond_signal(&mesh_queue_cond);
	}
	pthread_mutex_unlock(&mesh_queue_mutex);
}

void process_chunks() {
	int result = pthread_mutex_lock(&chunks_mutex);
	if (result != 0) {
		fprintf(stderr, "Failed to lock chunks mutex in process_chunks: %s\n", strerror(result));
		return;
	}

	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->needs_update && chunk->is_loaded) {
					if (!enqueue_mesh_job(x, y, z)) {
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