#include "main.h"
#include "world.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

int last_cx = -1;
int last_cz = -1;

const int SEA_LEVEL = 64;
bool world_loading = false;

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

typedef struct {
	Entity* entity;
} ThreadArgs;

pthread_t terrain_thread = 0;
pthread_mutex_t terrain_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
bool terrain_thread_busy = false;

void load_around_entity_func(Entity* entity) {
	int center_cx = floorf(entity->x / CHUNK_SIZE) - (RENDER_DISTANCE / 2);
	int center_cz = floorf(entity->z / CHUNK_SIZE) - (RENDER_DISTANCE / 2);

	int dx = center_cx - last_cx;
	int dz = center_cz - last_cz;

	if (dx == 0 && dz == 0) return;

	// Allocate and copy temp_chunks manually
	Chunk*** temp_chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);
	for (int x = 0; x < RENDER_DISTANCE; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			memcpy(temp_chunks[x][y], chunks[x][y], RENDER_DISTANCE * sizeof(Chunk));
		}
	}

	// Clear old chunks and mark boundaries for update
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

	// Load new chunks and mark edges for update
	world_loading = true;
	for (int x = 0; x < RENDER_DISTANCE; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < RENDER_DISTANCE; z++) {
				if (chunks[x][y][z].vertices != NULL)
					continue;

				load_chunk(x, y, z, x + center_cx, y, z + center_cz);
				if (x > 0 && chunks[x-1][y][z].vertices != NULL) chunks[x-1][y][z].needs_update = true;
				if (x < RENDER_DISTANCE-1 && chunks[x+1][y][z].vertices != NULL) chunks[x+1][y][z].needs_update = true;
				if (z > 0 && chunks[x][y][z-1].vertices != NULL) chunks[x][y][z-1].needs_update = true;
				if (z < RENDER_DISTANCE-1 && chunks[x][y][z+1].vertices != NULL) chunks[x][y][z+1].needs_update = true;
			}
		}
	}
	world_loading = false;

	// Free temp_chunks
	free_chunks(temp_chunks, RENDER_DISTANCE, WORLD_HEIGHT);

	last_cx = center_cx;
	last_cz = center_cz;
}

void* load_around_entity_thread(void* args) {
	// Profiler causes threads to hang, Should probably look into it..
	// #ifdef DEBUG
	// profiler_start(PROFILER_ID_WORLD_GEN);
	// #endif
	pthread_mutex_lock(&terrain_thread_mutex);
	terrain_thread_busy = true;
	ThreadArgs* thread_args = (ThreadArgs*)args;
	load_around_entity_func(thread_args->entity);

	free(args);
	// #ifdef DEBUG
	// profiler_stop(PROFILER_ID_WORLD_GEN);
	// #endif
	terrain_thread_busy = false;
	pthread_mutex_unlock(&terrain_thread_mutex);

	return NULL;
}

void load_around_entity(Entity* entity) {
	if (terrain_thread_busy)
		return;

	ThreadArgs* args = (ThreadArgs*)malloc(sizeof(ThreadArgs));

	args->entity = entity;

	int result = pthread_create(&terrain_thread, NULL, load_around_entity_thread, args);
	if (result != 0) {
		free(args);
		return;
	}

	pthread_detach(terrain_thread);
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