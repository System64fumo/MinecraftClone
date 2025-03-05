#include "main.h"
#include "world.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int last_cx = -1;
int last_cz = -1;

const int SEA_LEVEL = 64;

const float continent_scale = 0.005f;
const float mountain_scale = 0.03f;
const float flatness_scale = 0.02f;
const float cave_scale = 0.1f;
const float cave_simplex_scale = 0.1f;

uint32_t serialize(uint8_t a, uint8_t b, uint8_t c) {
	return ((uint32_t)a << 16) | ((uint32_t)b << 8) | (uint32_t)c;
}

void deserialize(uint32_t serialized, uint8_t *a, uint8_t *b, uint8_t *c) {
	*a = (serialized >> 16) & 0xFF;
	*b = (serialized >> 8) & 0xFF;
	*c = serialized & 0xFF;
}

chunk_loader_t chunk_loader = {
	.running = false,
	.should_load = false,
	.thread = 0
};

void* chunk_loader_thread(void* arg) {
	chunk_loader_t* loader = (chunk_loader_t*)arg;
	
	while (1) {
		Entity* entity = NULL;
		bool should_load = false;

		// Wait for load request
		pthread_mutex_lock(&loader->mutex);
		while (!loader->should_load) {
			pthread_cond_wait(&loader->cond, &loader->mutex);
		}

		// Get the entity and reset flag
		entity = loader->entity;
		should_load = loader->should_load;
		loader->should_load = false;
		pthread_mutex_unlock(&loader->mutex);

		if (entity != NULL && should_load) {
			// Perform actual chunk loading
			int center_cx = floorf(entity->x / CHUNK_SIZE) - (RENDER_DISTANCE / 2);
			int center_cz = floorf(entity->z / CHUNK_SIZE) - (RENDER_DISTANCE / 2);

			int dx, dz;
			int local_last_cx, local_last_cz;

			pthread_mutex_lock(&loader->mutex);
			local_last_cx = last_cx;
			local_last_cz = last_cz;
			pthread_mutex_unlock(&loader->mutex);

			dx = center_cx - local_last_cx;
			dz = center_cz - local_last_cz;

			pthread_mutex_lock(&loader->mutex);
			last_cx = center_cx;
			last_cz = center_cz;
			pthread_mutex_unlock(&loader->mutex);

			if (dx == 0 && dz == 0) continue;

			// Allocate and copy temp_chunks manually
			Chunk*** temp_chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);
			if (!temp_chunks) continue;

			pthread_mutex_lock(&mesh_mutex);
			for (int x = 0; x < RENDER_DISTANCE; x++) {
				for (int y = 0; y < WORLD_HEIGHT; y++) {
					memcpy(temp_chunks[x][y], chunks[x][y], RENDER_DISTANCE * sizeof(Chunk));
				}
			}
			pthread_mutex_unlock(&mesh_mutex);

			// Process old chunks
			if (dx > 0) { // Moving East
				for (int x = 0; x < dx && x < RENDER_DISTANCE; x++) {
					for (int y = 0; y < WORLD_HEIGHT; y++) {
						for (int z = 0; z < RENDER_DISTANCE; z++) {
							if (temp_chunks[x][y][z].vertices != NULL) {
								if (x == dx - 1 && x + 1 < RENDER_DISTANCE) {
									temp_chunks[x + 1][y][z].needs_update = true;
								}
								unload_chunk(&temp_chunks[x][y][z]);
							}
						}
					}
				}
			}
			else if (dx < 0) { // Moving West
				for (int x = RENDER_DISTANCE + dx; x < RENDER_DISTANCE; x++) {
					for (int y = 0; y < WORLD_HEIGHT; y++) {
						for (int z = 0; z < RENDER_DISTANCE; z++) {
							if (temp_chunks[x][y][z].vertices != NULL) {
								if (x == RENDER_DISTANCE + dx && x - 1 >= 0) {
									temp_chunks[x - 1][y][z].needs_update = true;
								}
								unload_chunk(&temp_chunks[x][y][z]);
							}
						}
					}
				}
			}

			if (dz > 0) { // Moving South
				for (int x = 0; x < RENDER_DISTANCE; x++) {
					for (int y = 0; y < WORLD_HEIGHT; y++) {
						for (int z = 0; z < dz && z < RENDER_DISTANCE; z++) {
							if (temp_chunks[x][y][z].vertices != NULL) {
								if (z == dz - 1 && z + 1 < RENDER_DISTANCE) {
									temp_chunks[x][y][z + 1].needs_update = true;
								}
								unload_chunk(&temp_chunks[x][y][z]);
							}
						}
					}
				}
			}
			else if (dz < 0) { // Moving North
				for (int x = 0; x < RENDER_DISTANCE; x++) {
					for (int y = 0; y < WORLD_HEIGHT; y++) {
						for (int z = RENDER_DISTANCE + dz; z < RENDER_DISTANCE; z++) {
							if (temp_chunks[x][y][z].vertices != NULL) {
								if (z == RENDER_DISTANCE + dz && z - 1 >= 0) {
									temp_chunks[x][y][z - 1].needs_update = true;
								}
								unload_chunk(&temp_chunks[x][y][z]);
							}
						}
					}
				}
			}

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

			// Load new chunks and mark edges for update
			for (int x = 0; x < RENDER_DISTANCE; x++) {
				for (int y = 0; y < WORLD_HEIGHT; y++) {
					for (int z = 0; z < RENDER_DISTANCE; z++) {
						bool should_skip = false;
						
						pthread_mutex_lock(&mesh_mutex);
						should_skip = chunks[x][y][z].vertices != NULL;
						pthread_mutex_unlock(&mesh_mutex);
						
						if (should_skip)
							continue;
						
						// Load chunk atomically
						Chunk temp_chunk = {0};
						load_chunk_data(&temp_chunk, x, y, z, x + center_cx, y, z + center_cz);
						
						pthread_mutex_lock(&mesh_mutex);
						chunks[x][y][z] = temp_chunk;
						
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
				}
			}

			// Free temp_chunks
			free_chunks(temp_chunks, RENDER_DISTANCE, WORLD_HEIGHT);
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
	set_chunk_lighting(chunk);
}

void load_chunk(unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz) {
	chunks[ci_x][ci_y][ci_z].ci_x = ci_x;
	chunks[ci_x][ci_y][ci_z].ci_y = ci_y;
	chunks[ci_x][ci_y][ci_z].ci_z = ci_z;
	chunks[ci_x][ci_y][ci_z].x = cx;
	chunks[ci_x][ci_y][ci_z].y = cy;
	chunks[ci_x][ci_y][ci_z].z = cz;
	chunks[ci_x][ci_y][ci_z].needs_update = true;

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


	// Abuse terrain thread for lighting (This is terrible and should ideally run on another thread)
	set_chunk_lighting(&chunks[ci_x][ci_y][ci_z]);
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