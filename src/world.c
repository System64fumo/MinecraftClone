#include "main.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z) {
	float scale = 0.05f;  // Adjust this to change the "roughness" of the terrain
	float height_scale = 16.0f;  // Maximum height variation

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