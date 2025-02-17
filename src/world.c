#include "main.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int last_cx = -1;
int last_cz = -1;

uint32_t serialize(uint8_t a, uint8_t b, uint8_t c) {
	return ((uint32_t)a << 16) | ((uint32_t)b << 8) | (uint32_t)c;
}

void deserialize(uint32_t serialized, uint8_t *a, uint8_t *b, uint8_t *c) {
	*a = (serialized >> 16) & 0xFF;
	*b = (serialized >> 8) & 0xFF;
	*c = serialized & 0xFF;
}

void load_around_entity(Entity* entity) {
	static Chunk temp_chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];

	int center_cx = floorf(entity->x / CHUNK_SIZE) - (RENDERR_DISTANCE / 2);
	int center_cz = floorf(entity->z / CHUNK_SIZE) - (RENDERR_DISTANCE / 2);

	int dx = center_cx - last_cx;
	int dz = center_cz - last_cz;

	if (dx == 0 && dz == 0) return;

	#ifdef DEBUG
	profiler_start(PROFILER_ID_WORLD_GEN);
	#endif

	memcpy(temp_chunks, chunks, sizeof(chunks));

	// Clear old chunks and mark boundaries for update
	if (dx > 0) { // Moving East
		for (int x = 0; x < dx && x < RENDERR_DISTANCE; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = 0; z < RENDERR_DISTANCE; z++) {
					if (temp_chunks[x][y][z].VBO) {
						if (x == dx - 1 && x + 1 < RENDERR_DISTANCE) {
							temp_chunks[x + 1][y][z].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}
	else if (dx < 0) { // Moving West
		for (int x = RENDERR_DISTANCE + dx; x < RENDERR_DISTANCE; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = 0; z < RENDERR_DISTANCE; z++) {
					if (temp_chunks[x][y][z].VBO) {
						if (x == RENDERR_DISTANCE + dx && x - 1 >= 0) {
							temp_chunks[x - 1][y][z].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}

	if (dz > 0) { // Moving South
		for (int x = 0; x < RENDERR_DISTANCE; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = 0; z < dz && z < RENDERR_DISTANCE; z++) {
					if (temp_chunks[x][y][z].VBO) {
						if (z == dz - 1 && z + 1 < RENDERR_DISTANCE) {
							temp_chunks[x][y][z + 1].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}
	else if (dz < 0) { // Moving North
		for (int x = 0; x < RENDERR_DISTANCE; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = RENDERR_DISTANCE + dz; z < RENDERR_DISTANCE; z++) {
					if (temp_chunks[x][y][z].VBO) {
						if (z == RENDERR_DISTANCE + dz && z - 1 >= 0) {
							temp_chunks[x][y][z - 1].needs_update = true;
						}
						unload_chunk(&temp_chunks[x][y][z]);
					}
				}
			}
		}
	}

	memset(chunks, 0, sizeof(chunks));

	// Move surviving chunks
	for (int x = 0; x < RENDERR_DISTANCE; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < RENDERR_DISTANCE; z++) {
				int old_x = x + dx;
				int old_z = z + dz;
					
				if (old_x >= 0 && old_x < RENDERR_DISTANCE && 
					old_z >= 0 && old_z < RENDERR_DISTANCE &&
					temp_chunks[old_x][y][old_z].VBO) {
					chunks[x][y][z] = temp_chunks[old_x][y][old_z];
					chunks[x][y][z].ci_x = x;
					chunks[x][y][z].ci_z = z;
				}
			}
		}
	}

	// Load new chunks and mark edges for update
	for (int x = 0; x < RENDERR_DISTANCE; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < RENDERR_DISTANCE; z++) {
				if (!chunks[x][y][z].VBO) {
					load_chunk(x, y, z, x + center_cx, y, z + center_cz);
					if (x > 0 && chunks[x-1][y][z].VBO) chunks[x-1][y][z].needs_update = true;
					if (x < RENDERR_DISTANCE-1 && chunks[x+1][y][z].VBO) chunks[x+1][y][z].needs_update = true;
					if (z > 0 && chunks[x][y][z-1].VBO) chunks[x][y][z-1].needs_update = true;
					if (z < RENDERR_DISTANCE-1 && chunks[x][y][z+1].VBO) chunks[x][y][z+1].needs_update = true;
				}
			}
		}
	}

	last_cx = center_cx;
	last_cz = center_cz;

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_WORLD_GEN);
	#endif
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
	if (chunk->VBO) {
		glDeleteBuffers(1, &chunk->VBO);
		chunk->VBO = 0;
	}
	if (chunk->EBO) {
		glDeleteBuffers(1, &chunk->EBO);
		chunk->EBO = 0;
	}
	if (chunk->VAO) {
		glDeleteVertexArrays(1, &chunk->VAO);
		chunk->VAO = 0;
	}

	memset(chunk, 0, sizeof(Chunk));
}



void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z) {
	float scale = 0.01f;
	static float height_scale = WORLD_HEIGHT * CHUNK_SIZE;

	for(uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for(uint8_t z = 0; z < CHUNK_SIZE; z++) {
			float world_x = (chunk_x * CHUNK_SIZE + x) * scale;
			float world_z = (chunk_z * CHUNK_SIZE + z) * scale;

			float height = stb_perlin_noise3(world_x, 0.0f, world_z, 0, 0, 0);
			height = (height + 1.0f) * 0.5f;  // Normalize to 0-1 range
			int terrain_height = (int)(height * height_scale) + 8;  // Add base height

			int chunk_base_y = chunk_y * CHUNK_SIZE;

			for(uint8_t y = 0; y < CHUNK_SIZE; y++) {
				int absolute_y = chunk_base_y + y;

				if (absolute_y == 0) {
					chunk->blocks[x][y][z] = (Block){.id = 7};  // Bedrock
				}
				else if(absolute_y > terrain_height) {
					chunk->blocks[x][y][z] = (Block){.id = 0};  // Air
				}
				else if(absolute_y == terrain_height) {
					chunk->blocks[x][y][z] = (Block){.id = 2};  // Grass
				}
				else if(absolute_y >= terrain_height - 3) {
					chunk->blocks[x][y][z] = (Block){.id = 1};  // Dirt
				}
				else {
					chunk->blocks[x][y][z] = (Block){.id = 3};  // Stone
				}
			}
		}
	}
}
