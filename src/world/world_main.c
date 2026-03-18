#include "main.h"
#include "entity.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void relight_chunk(Chunk *chunk);
void init_column_lighting(Chunk col[WORLD_HEIGHT]);

_Atomic int world_offset_x = 0;
_Atomic int world_offset_z = 0;
int last_cx = -1;
int last_cy = -1;
int last_cz = -1;


// Column jobs: one job per (ci_x, ci_z) generates all WORLD_HEIGHT chunks.
// This means the sky column pass always has full vertical context.
typedef struct {
	uint8_t ci_x, ci_z;
	int cx, cz;
	float priority;
} column_load_request_t;

typedef struct {
	column_load_request_t* requests;
	int capacity;
	int size;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
} column_load_queue_t;

typedef struct {
	bool tracking_active;
	uint32_t chunks_queued;
	uint32_t chunks_completed;
	pthread_mutex_t mutex;
} world_gen_tracker_t;

static column_load_queue_t column_load_queue;
#define NUM_GEN_THREADS 4
static pthread_t world_gen_threads[NUM_GEN_THREADS];
bool world_gen_thread_running = false;
pthread_mutex_t chunks_mutex = PTHREAD_MUTEX_INITIALIZER;
static world_gen_tracker_t world_gen_tracker = {0};

static bool* chunk_needs_load_cache = NULL;
static size_t cache_size = 0;

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
	if (world_gen_tracker.tracking_active)
		world_gen_tracker.chunks_queued++;
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

static void init_column_load_queue() {
	column_load_queue.capacity = 256;
	column_load_queue.size = 0;
	column_load_queue.requests = malloc(column_load_queue.capacity * sizeof(column_load_request_t));
	pthread_mutex_init(&column_load_queue.mutex, NULL);
	pthread_cond_init(&column_load_queue.cond, NULL);

	size_t total = settings.render_distance * settings.render_distance;
	if (cache_size < total) {
		free(chunk_needs_load_cache);
		chunk_needs_load_cache = malloc(total * sizeof(bool));
		cache_size = total;
	}
}

static void enqueue_column(int ci_x, int ci_z, int cx, int cz, float priority) {
	pthread_mutex_lock(&column_load_queue.mutex);
	if (column_load_queue.size >= column_load_queue.capacity) {
		column_load_queue.capacity *= 2;
		column_load_queue.requests = realloc(column_load_queue.requests,
			column_load_queue.capacity * sizeof(column_load_request_t));
	}
	column_load_request_t *req = &column_load_queue.requests[column_load_queue.size++];
	req->ci_x = ci_x;
	req->ci_z = ci_z;
	req->cx = cx;
	req->cz = cz;
	req->priority = priority;
	pthread_cond_signal(&column_load_queue.cond);
	pthread_mutex_unlock(&column_load_queue.mutex);
}

// Generate all WORLD_HEIGHT chunks for a column, light them, then install.
// Running terrain gen outside chunks_mutex allows true parallelism.
void* world_gen_thread_func(void* arg) {
	while (world_gen_thread_running) {
		pthread_mutex_lock(&column_load_queue.mutex);
		while (column_load_queue.size == 0 && world_gen_thread_running)
			pthread_cond_wait(&column_load_queue.cond, &column_load_queue.mutex);
		if (!world_gen_thread_running) {
			pthread_mutex_unlock(&column_load_queue.mutex);
			break;
		}

		// Dequeue highest-priority (lowest dist) column.
		int best = 0;
		for (int i = 1; i < column_load_queue.size; i++) {
			if (column_load_queue.requests[i].priority < column_load_queue.requests[best].priority)
				best = i;
		}
		column_load_request_t req = column_load_queue.requests[best];
		column_load_queue.requests[best] = column_load_queue.requests[--column_load_queue.size];
		pthread_mutex_unlock(&column_load_queue.mutex);

		// Validate slot still maps to the same world position (player may have scrolled).
		int current_offset_x = (int)atomic_load(&world_offset_x);
		int current_offset_z = (int)atomic_load(&world_offset_z);
		if (req.ci_x != req.cx - current_offset_x || req.ci_z != req.cz - current_offset_z) {
			track_chunk_completed();
			continue;
		}

		// --- Terrain generation (outside chunks_mutex) ---
		// Generate all WORLD_HEIGHT chunks for this column.
		Chunk temp_chunks[WORLD_HEIGHT];
		memset(temp_chunks, 0, sizeof(temp_chunks));
#ifdef DEBUG
		profiler_start(PROFILER_ID_TERRAIN, false);
#endif
		for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
			load_chunk_data(&temp_chunks[cy], req.ci_x, cy, req.ci_z, req.cx, cy, req.cz);
		}
#ifdef DEBUG
		profiler_stop(PROFILER_ID_TERRAIN, false);
#endif

		// --- Sky lighting pass (outside chunks_mutex) ---
#ifdef DEBUG
		profiler_start(PROFILER_ID_LIGHTING, false);
#endif
		init_column_lighting(temp_chunks);
#ifdef DEBUG
		profiler_stop(PROFILER_ID_LIGHTING, false);
#endif

		// --- Install (under chunks_mutex) ---
		pthread_mutex_lock(&chunks_mutex);

		// Re-validate after acquiring lock.
		current_offset_x = (int)atomic_load(&world_offset_x);
		current_offset_z = (int)atomic_load(&world_offset_z);
		if (req.ci_x != req.cx - current_offset_x || req.ci_z != req.cz - current_offset_z) {
			pthread_mutex_unlock(&chunks_mutex);
			track_chunk_completed();
			continue;
		}

		for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
			Chunk *slot = &chunks[req.ci_x][cy][req.ci_z];
			unload_chunk(slot);
			*slot = temp_chunks[cy];
			slot->is_loaded = true;
		}

		// Cross-boundary relight is handled by the mesh thread when it processes
		// each chunk and checks are_all_neighbors_loaded + lighting_changed.
		// Just mark horizontal neighbours for mesh rebuild since new faces appeared.
		for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
			static const int8_t ndx[] = { 1,-1, 0, 0 };
			static const int8_t ndz[] = { 0, 0, 1,-1 };
			for (int d = 0; d < 4; d++) {
				int nx = (int)req.ci_x + ndx[d];
				int nz = (int)req.ci_z + ndz[d];
				if (nx < 0 || nx >= settings.render_distance) continue;
				if (nz < 0 || nz >= settings.render_distance) continue;
				if (chunks[nx][cy][nz].is_loaded)
					chunks[nx][cy][nz].needs_update = true;
			}
		}

		pthread_mutex_unlock(&chunks_mutex);
		track_chunk_completed();
	}
	return NULL;
}

void start_world_gen_thread() {
	init_column_load_queue();
	init_world_gen_tracker();
	world_gen_thread_running = true;
	for (int i = 0; i < NUM_GEN_THREADS; i++)
		pthread_create(&world_gen_threads[i], NULL, world_gen_thread_func, NULL);
}

void stop_world_gen_thread() {
	world_gen_thread_running = false;
	pthread_mutex_lock(&column_load_queue.mutex);
	pthread_cond_broadcast(&column_load_queue.cond);
	pthread_mutex_unlock(&column_load_queue.mutex);
	for (int i = 0; i < NUM_GEN_THREADS; i++)
		pthread_join(world_gen_threads[i], NULL);
	pthread_mutex_destroy(&column_load_queue.mutex);
	pthread_cond_destroy(&column_load_queue.cond);
	free(column_load_queue.requests);
	column_load_queue.requests = NULL;
	free(chunk_needs_load_cache);
	chunk_needs_load_cache = NULL;
	cache_size = 0;
}

static bool column_needs_loading(uint8_t ci_x, uint8_t ci_z) {
	for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
		if (!chunks[ci_x][cy][ci_z].is_loaded)
			return true;
	}
	return false;
}

void load_around_entity(Entity* entity) {
	int center_cx = floorf(entity->pos.x / CHUNK_SIZE) - (settings.render_distance / 2);
	int center_cz = floorf(entity->pos.z / CHUNK_SIZE) - (settings.render_distance / 2);

	// On first call last_cx==-1 is a sentinel; treat as no prior position.
	if (last_cx == -1) {
		last_cx = center_cx;
		last_cz = center_cz;
		atomic_store(&world_offset_x, center_cx);
		atomic_store(&world_offset_z, center_cz);
		// fall through to enqueue below
	}

	int dx = center_cx - last_cx;
	int dz = center_cz - last_cz;

	bool position_changed = (dx != 0 || dz != 0);
	// Don't return early — always fall through to enqueue missing columns.
	// This re-enqueues any jobs that were discarded due to stale-check.

#ifdef DEBUG
	if (position_changed) {
		profiler_start(PROFILER_ID_WORLD_GEN, false);
		start_world_gen_tracking();
	}
#endif

	last_cx = center_cx;
	last_cz = center_cz;

	if (position_changed) {
	pthread_mutex_lock(&chunks_mutex);

	// Unload columns that scroll off the edge.
	for (int x = 0; x < settings.render_distance; x++) {
		int new_x = x - dx;
		int new_z_dummy = 0; // handled below
		(void)new_z_dummy;
		for (int z = 0; z < settings.render_distance; z++) {
			int new_z = z - dz;
			bool scrolls_off = (new_x < 0 || new_x >= settings.render_distance ||
								new_z < 0 || new_z >= settings.render_distance);
			if (scrolls_off) {
				for (int y = 0; y < WORLD_HEIGHT; y++)
					unload_chunk(&chunks[x][y][z]);
			}
		}
	}

	// Single-pass scratch-buffer shift — fixes diagonal move corruption in the
	// original two-pass approach where X-shift would overwrite slots the Z-shift
	// still needed to read.
	{
		int rd = settings.render_distance;
		size_t total = (size_t)rd * WORLD_HEIGHT * rd;
		Chunk *tmp = malloc(total * sizeof(Chunk));
		if (tmp) {
			memset(tmp, 0, total * sizeof(Chunk));
			for (int x = 0; x < rd; x++) {
				int nx = x - dx;
				if (nx < 0 || nx >= rd) continue;
				for (int z = 0; z < rd; z++) {
					int nz = z - dz;
					if (nz < 0 || nz >= rd) continue;
					for (int y = 0; y < WORLD_HEIGHT; y++) {
						tmp[(size_t)nx*WORLD_HEIGHT*rd + y*rd + nz] = chunks[x][y][z];
						tmp[(size_t)nx*WORLD_HEIGHT*rd + y*rd + nz].ci_x = nx;
						tmp[(size_t)nx*WORLD_HEIGHT*rd + y*rd + nz].ci_z = nz;
						tmp[(size_t)nx*WORLD_HEIGHT*rd + y*rd + nz].mesh_dirty = true;
					}
				}
			}
			memcpy(chunks[0][0], tmp, total * sizeof(Chunk));
			free(tmp);
		}
	}
	// Update world_offset AFTER data is in place so get_block_at always
	// sees a consistent (offset, data) pair.
	atomic_store(&world_offset_x, center_cx);
	atomic_store(&world_offset_z, center_cz);
	mesh_needs_rebuild = true;

	// Mark edge columns that now border empty slots for mesh rebuild.
	// Only mark columns that are loaded and border an unloaded neighbour —
	// do NOT mark surviving interior columns since their mesh is still valid.
	for (int x = 0; x < settings.render_distance; x++) {
		for (int z = 0; z < settings.render_distance; z++) {
			if (!chunks[x][0][z].is_loaded) continue;
			bool borders_empty = false;
			if (x > 0 && !chunks[x-1][0][z].is_loaded) borders_empty = true;
			if (x < settings.render_distance-1 && !chunks[x+1][0][z].is_loaded) borders_empty = true;
			if (z > 0 && !chunks[x][0][z-1].is_loaded) borders_empty = true;
			if (z < settings.render_distance-1 && !chunks[x][0][z+1].is_loaded) borders_empty = true;
			if (borders_empty) {
				for (int y = 0; y < WORLD_HEIGHT; y++)
					chunks[x][y][z].needs_update = true;
			}
		}
	}

	pthread_mutex_unlock(&chunks_mutex);
	} // end position_changed shift block

	float entity_chunk_x = entity->pos.x / CHUNK_SIZE;
	float entity_chunk_z = entity->pos.z / CHUNK_SIZE;

	// Count columns to load.
	int cols_to_load = 0;
	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t z = 0; z < settings.render_distance; z++) {
			size_t idx = (size_t)x * settings.render_distance + z;
			chunk_needs_load_cache[idx] = column_needs_loading(x, z);
			if (chunk_needs_load_cache[idx]) cols_to_load++;
		}
	}
	pthread_mutex_unlock(&chunks_mutex);

	if (cols_to_load == 0) {
#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
#endif
		return;
	}

	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t z = 0; z < settings.render_distance; z++) {
			size_t idx = (size_t)x * settings.render_distance + z;
			if (!chunk_needs_load_cache[idx]) continue;

			float wdx = (x + center_cx) - entity_chunk_x;
			float wdz = (z + center_cz) - entity_chunk_z;
			float dist_sq = wdx*wdx + wdz*wdz;

			enqueue_column(x, z, x + center_cx, z + center_cz, dist_sq);
			track_chunk_queued();
		}
	}

	pthread_mutex_lock(&world_gen_tracker.mutex);
	if (world_gen_tracker.chunks_queued == 0) {
#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN, false);
#endif
	}
	pthread_mutex_unlock(&world_gen_tracker.mutex);
}

void load_chunk_data(Chunk* chunk, unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz) {
	chunk->ci_x = ci_x;
	chunk->ci_y = ci_y;
	chunk->ci_z = ci_z;
	chunk->x = cx;
	chunk->y = cy;
	chunk->z = cz;
	chunk->needs_update = true;
	chunk->lighting_changed = true;

	generate_chunk_terrain(chunk, cx, cy, cz);
}

void unload_chunk(Chunk* chunk) {
	if (chunk == NULL) return;

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