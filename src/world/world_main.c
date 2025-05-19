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

const int SEA_LEVEL = 64;

const float continent_scale = 0.005f;
const float mountain_scale = 0.03f;
const float flatness_scale = 0.02f;
const float cave_scale = 0.1f;
const float cave_simplex_scale = 0.1f;

chunk_loader_t chunk_loader = {
	.running = false,
	.should_load = false,
	.thread = 0
};

uint32_t serialize(uint8_t a, uint8_t b, uint8_t c) {
	return ((uint32_t)a << 16) | ((uint32_t)b << 8) | (uint32_t)c;
}

void deserialize(uint32_t serialized, uint8_t *a, uint8_t *b, uint8_t *c) {
	*a = (serialized >> 16) & 0xFF;
	*b = (serialized >> 8) & 0xFF;
	*c = serialized & 0xFF;
}
			
// Perform counting sort based on digit at exp
void counting_sort(chunk_load_item_t* arr, int size, int exp) {
	chunk_load_item_t* output = malloc(size * sizeof(chunk_load_item_t));
	int count[10] = {0};
	int i;
	
	// Store count of occurrences in count[]
	for (i = 0; i < size; i++) {
		count[GET_DIGIT(arr[i].distance, exp)]++;
	}
	
	// Change count[i] so it contains actual position
	for (i = 1; i < 10; i++) {
		count[i] += count[i - 1];
	}
	
	// Build the output array
	for (i = size - 1; i >= 0; i--) {
		int digit = GET_DIGIT(arr[i].distance, exp);
		output[count[digit] - 1] = arr[i];
		count[digit]--;
	}
	
	// Copy the output array back
	for (i = 0; i < size; i++) {
		arr[i] = output[i];
	}
	
	free(output);
}

// Radix sort implementation
void radix_sort(chunk_load_item_t* arr, int size) {
	if (size <= 1) return;
	
	// Find the maximum distance to know number of digits
	float max_dist = arr[0].distance;
	for (int i = 1; i < size; i++) {
		if (arr[i].distance > max_dist) {
			max_dist = arr[i].distance;
		}
	}
	
	// Scale distances to integers for sorting
	const float scale = 1000.0f; // Scale factor to preserve decimal precision
	for (int i = 0; i < size; i++) {
		arr[i].distance *= scale;
	}
	
	// Do counting sort for every digit
	for (int exp = 1; max_dist*scale/exp > 0; exp *= 10) {
		counting_sort(arr, size, exp);
	}
	
	// Scale back to original values
	for (int i = 0; i < size; i++) {
		arr[i].distance /= scale;
	}
}

void* chunk_loader_thread(void* arg) {
	chunk_loader_t* loader = (chunk_loader_t*)arg;
	struct timespec ts;
	
	while (1) {
		Entity* entity = NULL;
		bool should_load = false;

		// Wait for load request with timeout
		pthread_mutex_lock(&loader->mutex);
		if (!loader->should_load) {
			clock_gettime(CLOCK_REALTIME, &ts);
			ts.tv_nsec += 10000000; // 10ms
			pthread_cond_timedwait(&loader->cond, &loader->mutex, &ts);
		}

		// Get the entity and reset flag
		entity = loader->entity;
		should_load = loader->should_load;
		loader->should_load = false;
		pthread_mutex_unlock(&loader->mutex);

		if (entity != NULL && should_load) {
			#ifdef DEBUG
			profiler_start(PROFILER_ID_WORLD_GEN, false);
			#endif
			
			int center_cx = floorf(entity->pos.x / CHUNK_SIZE) - (RENDER_DISTANCE / 2);
			last_cy = floorf(entity->pos.y / CHUNK_SIZE) - (WORLD_HEIGHT / 2);
			int center_cz = floorf(entity->pos.z / CHUNK_SIZE) - (RENDER_DISTANCE / 2);

			int dx, dz;
			int local_last_cx, local_last_cz;

			// Get current position with minimal lock time
			pthread_mutex_lock(&loader->mutex);
			local_last_cx = last_cx;
			local_last_cz = last_cz;
			pthread_mutex_unlock(&loader->mutex);

			dx = center_cx - local_last_cx;
			dz = center_cz - local_last_cz;

			// Update position with minimal lock time
			pthread_mutex_lock(&loader->mutex);
			last_cx = center_cx;
			last_cz = center_cz;
			pthread_mutex_unlock(&loader->mutex);

			if (dx == 0 && dz == 0) continue;

			// Allocate temporary chunks
			Chunk*** temp_chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);
			if (!temp_chunks) continue;

			// Copy chunks with a single lock
			pthread_mutex_lock(&mesh_mutex);
			for (int x = 0; x < RENDER_DISTANCE; x++) {
				for (int y = 0; y < WORLD_HEIGHT; y++) {
					memcpy(temp_chunks[x][y], chunks[x][y], RENDER_DISTANCE * sizeof(Chunk));
				}
			}
			pthread_mutex_unlock(&mesh_mutex);

			// Process old chunks (east/west movement)
			if (dx != 0) {
				int start_x = (dx > 0) ? 0 : RENDER_DISTANCE + dx;
				int end_x = (dx > 0) ? dx : RENDER_DISTANCE;
				
				for (int x = start_x; x < end_x; x++) {
					for (int y = 0; y < WORLD_HEIGHT; y++) {
						for (int z = 0; z < RENDER_DISTANCE; z++) {
							if (temp_chunks[x][y][z].vertices != NULL) {
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
							if (temp_chunks[x][y][z].vertices != NULL) {
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

			// Update chunks array
			pthread_mutex_lock(&mesh_mutex);
			// Clear chunks array
			for (int x = 0; x < RENDER_DISTANCE; x++) {
				for (int y = 0; y < WORLD_HEIGHT; y++) {
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
							old_z >= 0 && old_z < RENDER_DISTANCE &&
							temp_chunks[old_x][y][old_z].vertices != NULL) {
							chunks[x][y][z] = temp_chunks[old_x][y][old_z];
							chunks[x][y][z].ci_x = x;
							chunks[x][y][z].ci_z = z;
						}
					}
				}
			}
			pthread_mutex_unlock(&mesh_mutex);

			// Create a prioritized list of chunks to load based on distance to entity		
			chunk_load_item_t* load_queue = malloc(RENDER_DISTANCE * WORLD_HEIGHT * RENDER_DISTANCE * sizeof(chunk_load_item_t));
			int queue_size = 0;
			
			// Entity position within chunk coordinates
			float entity_chunk_x = entity->pos.x / CHUNK_SIZE;
			float entity_chunk_y = entity->pos.y / CHUNK_SIZE;
			float entity_chunk_z = entity->pos.z / CHUNK_SIZE;
			
			// Calculate which chunks need loading and their distances
			for (int x = 0; x < RENDER_DISTANCE; x++) {
				for (int y = 0; y < WORLD_HEIGHT; y++) {
					for (int z = 0; z < RENDER_DISTANCE; z++) {
						bool should_load = false;
						
						pthread_mutex_lock(&mesh_mutex);
						should_load = chunks[x][y][z].vertices == NULL;
						pthread_mutex_unlock(&mesh_mutex);
						
						if (should_load) {
							// Calculate world position of this chunk
							float world_chunk_x = (x + center_cx);
							float world_chunk_y = (y + last_cy);
							float world_chunk_z = (z + center_cz);
							
							// Calculate distance to entity (squared)
							float dx = world_chunk_x - entity_chunk_x;
							float dy = world_chunk_y - entity_chunk_y;
							float dz = world_chunk_z - entity_chunk_z;
							float dist_sq = dx*dx + dy*dy + dz*dz;
							
							// Add to queue
							load_queue[queue_size].x = x;
							load_queue[queue_size].y = y;
							load_queue[queue_size].z = z;
							load_queue[queue_size].distance = dist_sq;
							queue_size++;
						}
					}
				}
			}
			
			// Sort chunks by distance (simple bubble sort - could be optimized)
			radix_sort(load_queue, queue_size);
			
			// Load chunks in priority order
			for (int i = 0; i < queue_size; i++) {
				int x = load_queue[i].x;
				int y = load_queue[i].y;
				int z = load_queue[i].z;
				
				// Load chunk atomically
				Chunk temp_chunk = {0};
				load_chunk_data(&temp_chunk, x, y, z, x + center_cx, y, z + center_cz);
				temp_chunk.is_loaded = true;
				
				pthread_mutex_lock(&mesh_mutex);
				chunks[x][y][z] = temp_chunk;
				
				// Mark adjacent chunks for update
				if (x > 0 && chunks[x-1][y][z].vertices != NULL) 
					chunks[x-1][y][z].needs_update = true;
				if (x < RENDER_DISTANCE-1 && chunks[x+1][y][z].vertices != NULL) 
					chunks[x+1][y][z].needs_update = true;
				if (z > 0 && chunks[x][y][z-1].vertices != NULL) 
					chunks[x][y][z-1].needs_update = true;
				if (z < RENDER_DISTANCE-1 && chunks[x][y][z+1].vertices != NULL) 
					chunks[x][y][z+1].needs_update = true;
				pthread_mutex_unlock(&mesh_mutex);
			}
			
			free(load_queue);
			free_chunks(temp_chunks, RENDER_DISTANCE, WORLD_HEIGHT);
			#ifdef DEBUG
			profiler_stop(PROFILER_ID_WORLD_GEN, false);
			#endif
		}
	}

	return NULL;
}

void init_chunk_loader() {
	pthread_mutex_init(&chunk_loader.mutex, NULL);
	pthread_cond_init(&chunk_loader.cond, NULL);
}

void destroy_chunk_loader() {
	if (chunk_loader.running) {
		pthread_cancel(chunk_loader.thread);
		chunk_loader.running = false;
	}
	
	pthread_mutex_destroy(&chunk_loader.mutex);
	pthread_cond_destroy(&chunk_loader.cond);
}

void load_around_entity(Entity* entity) {
	pthread_mutex_lock(&chunk_loader.mutex);
	
	// Start thread if not already running
	if (!chunk_loader.running) {
		chunk_loader.running = true;
		pthread_create(&chunk_loader.thread, NULL, chunk_loader_thread, &chunk_loader);
	}
	
	// Update entity and signal thread to load
	chunk_loader.entity = entity;
	chunk_loader.should_load = true;
	pthread_cond_signal(&chunk_loader.cond);
	
	pthread_mutex_unlock(&chunk_loader.mutex);
}

int save_chunk_to_file(const char *filename, const Chunk *chunk) {
	size_t data_size = sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE + // blocks
					   sizeof(int) * 3; // x, y, z

	uint8_t *buffer = malloc(data_size);
	if (!buffer) {
		return -1;
	}

	memcpy(buffer, chunk->blocks, sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

	memcpy(buffer + sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, &chunk->x, sizeof(int) * 3);

	int result = write_binary_file(filename, buffer, data_size);

	free(buffer);

	return result;
}

int load_chunk_from_file(const char *filename, Chunk *chunk) {
	size_t data_size = sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE + // blocks
					   sizeof(int) * 3; // x, y, z

	size_t loaded_size;
	uint8_t *data = read_binary_file(filename, &loaded_size);

	if (!data) {
		return -1;
	}

	if (loaded_size != data_size) {
		free(data);
		return -1;
	}

	memcpy(chunk->blocks, data, sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE);

	memcpy(&chunk->x, data + sizeof(Block) * CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, sizeof(int) * 3);

	free(data);

	return 0; // Success
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

void load_chunk(unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz) {
	chunks[ci_x][ci_y][ci_z].ci_x = ci_x;
	chunks[ci_x][ci_y][ci_z].ci_y = ci_y;
	chunks[ci_x][ci_y][ci_z].ci_z = ci_z;
	chunks[ci_x][ci_y][ci_z].x = cx;
	chunks[ci_x][ci_y][ci_z].y = cy;
	chunks[ci_x][ci_y][ci_z].z = cz;
	chunks[ci_x][ci_y][ci_z].needs_update = true;
	chunks[ci_x][ci_y][ci_z].is_loaded = false;

	// char filename[255];
	// snprintf(filename, sizeof(filename), "%s/saves/chunks/%u.bin", game_dir, serialize(cx, cy, cz));

	// size_t size;
	// int *read_data = read_binary_file(filename, &size);
	// if (read_data) {
	// 	printf("Loading chunk: %s\n", filename);
	// 	load_chunk_from_file(filename, &chunks[ci_x][ci_y][ci_z]);
	// }
	// else {
	// 	printf("Generating chunk: %s\n", filename);
	 	generate_chunk_terrain(&chunks[ci_x][ci_y][ci_z], cx, cy, cz);
	// 	save_chunk_to_file(filename, &chunks[ci_x][ci_y][ci_z]);
	// }
}

void unload_chunk(Chunk* chunk) {
	// char filename[255];
	// snprintf(filename, sizeof(filename), "%s/saves/chunks/%u.bin", game_dir, serialize(chunk->x, chunk->y, chunk->z));
	// if (access(filename, F_OK) == 0)
	// 	save_chunk_to_file(filename, chunk);
	if (chunk->vertices != NULL) {
		free(chunk->vertices);
		chunk->vertices = NULL;
	}
	if (chunk->indices != NULL) {
		free(chunk->indices);
		chunk->indices = NULL;
	}

	memset(chunk, 0, sizeof(Chunk));
}