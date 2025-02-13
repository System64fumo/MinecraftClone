#include "main.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include <stdio.h>
#include <string.h>

static int last_cx = -1;
static int last_cz = -1;

void load_around_entity(Entity* entity) {
	int center_cx = (int)fmaxf(0, fminf(WORLD_SIZE, floorf(entity->x / CHUNK_SIZE) - (RENDERR_DISTANCE / 2)));
	//int center_cy = (int)fmaxf(0, fminf(WORLD_HEIGHT, -(floorf(entity->y / CHUNK_SIZE) - WORLD_HEIGHT)));
	int center_cz = (int)fmaxf(0, fminf(WORLD_SIZE, floorf(entity->z / CHUNK_SIZE) - (RENDERR_DISTANCE / 2)));

	int dx = center_cx - last_cx;
	int dz = center_cz - last_cz;
	
	// Only update chunks if position has changed
	if (dx != 0 || dz != 0) {
		if (dx > 0) printf("Moving East\n");
		if (dx < 0) printf("Moving West\n");
		if (dz > 0) printf("Moving South\n");
		if (dz < 0) printf("Moving North\n");

		// Unload existing chunks
		for(int cx = 0; cx < RENDERR_DISTANCE; cx++) {
			for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
				for(int cz = 0; cz < RENDERR_DISTANCE; cz++) {
					Chunk* chunk = &chunks[cx][cy][cz];
					if (chunk->VBO) {
						unload_chunk(chunk);
					}
				}
			}
		}

		// Load new chunks
		for(int x = 0; x < RENDERR_DISTANCE; x++) {
			for(int y = 0; y < WORLD_HEIGHT; y++) {
				for(int z = 0; z < RENDERR_DISTANCE; z++) {
					load_chunk(x, y, z, x + center_cx, y, z + center_cz);
				}
			}
		}

		last_cx = center_cx;
		last_cz = center_cz;
	}
}

void load_chunk(unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, unsigned char cx, unsigned char cy, unsigned char cz) {
	chunks[ci_x][ci_y][ci_z].ci_x = ci_x;
	chunks[ci_x][ci_y][ci_z].ci_y = ci_y;
	chunks[ci_x][ci_y][ci_z].ci_z = ci_z;
	chunks[ci_x][ci_y][ci_z].x = cx;
	chunks[ci_x][ci_y][ci_z].y = cy;
	chunks[ci_x][ci_y][ci_z].z = cz;

	generate_chunk_terrain(&chunks[ci_x][ci_y][ci_z], cx, cy, cz);
}

void unload_chunk(Chunk* chunk) {
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

void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z) {
	float scale = 0.01f;  // Adjust this to change the "roughness" of the terrain
	float height_scale = WORLD_HEIGHT * CHUNK_SIZE;  // Maximum height variation

	for(int x = 0; x < CHUNK_SIZE; x++) {
		for(int z = 0; z < CHUNK_SIZE; z++) {
			// Get world coordinates
			float world_x = (chunk_x * CHUNK_SIZE + x) * scale;
			float world_z = (chunk_z * CHUNK_SIZE + z) * scale;

			// Generate height using Perlin noise
			float height = stb_perlin_noise3(world_x, 0.0f, world_z, 0, 0, 0);
			height = (height + 1.0f) * 0.5f;  // Normalize to 0-1 range
			int terrain_height = (int)(height * height_scale) + 8;  // Add base height

			// Calculate absolute y position for this chunk
			int chunk_base_y = chunk_y * CHUNK_SIZE;

			// Fill the chunk with blocks
			for(int y = 0; y < CHUNK_SIZE; y++) {
				int absolute_y = chunk_base_y + y;
				if(absolute_y > terrain_height) {
					chunk->blocks[x][y][z] = (Block){.id = 0, .metadata = 0};  // Air
				}
				else if(absolute_y == terrain_height) {
					chunk->blocks[x][y][z] = (Block){.id = 2, .metadata = 0};  // Grass
				}
				else if(absolute_y >= terrain_height - 3) {
					chunk->blocks[x][y][z] = (Block){.id = 1, .metadata = 0};  // Dirt
				}
				else {
					chunk->blocks[x][y][z] = (Block){.id = 3, .metadata = 0};  // Stone
				}
			}
		}
	}

	chunk->needs_update = true;
}
