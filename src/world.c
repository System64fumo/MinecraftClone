#include "main.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

void load_chunk(unsigned char cx, unsigned char cy, unsigned char cz) {
	chunks[cx][cy][cz] = (Chunk){0};
	chunks[cx][cy][cz].x = cx;
	chunks[cx][cy][cz].y = cy;
	chunks[cx][cy][cz].z = cz;
	chunks[cx][cy][cz].needs_update = true;
	chunks[cx][cy][cz].vbo = 0;
	chunks[cx][cy][cz].color_vbo = 0;
	chunks[cx][cy][cz].vertices = NULL;
	chunks[cx][cy][cz].colors = NULL;

	generate_chunk_terrain(&chunks[cx][cy][cz], cx, cy, cz);
}

void unload_chunk(Chunk* chunk) {
	if (chunk->vbo) {
		gl_delete_buffers(1, &chunk->vbo);
		chunk->vbo = 0;
	}
	if (chunk->color_vbo) {
		gl_delete_buffers(1, &chunk->color_vbo);
		chunk->color_vbo = 0;
	}
	if (chunk->vertices) {
		free(chunk->vertices);
		chunk->vertices = NULL;
	}
	if (chunk->colors) {
		free(chunk->colors);
		chunk->colors = NULL;
	}
	chunk->vertex_count = 0;
	chunk->needs_update = false;

	// Clear block data
	memset(chunk->blocks, 0, sizeof(chunk->blocks));
}

void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z) {
	float scale = 0.01f;  // Adjust this to change the "roughness" of the terrain
	float height_scale = 64.0f;  // Maximum height variation

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