#include "main.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_LIGHT_LEVEL 15

void set_chunk_lighting(Chunk* chunk) {
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				if (chunk->blocks[x][y][z].id == 6)
					chunk->blocks[x][y][z].light_level = 7;
				else if (block_data[chunk->blocks[x][y][z].id][1] == 1)
					chunk->blocks[x][y][z].light_level = settings.sky_brightness * MAX_LIGHT_LEVEL;
			}
		}
	}
}

#define WORLD_WIDTH (settings.render_distance * CHUNK_SIZE)
#define WORLD_DEPTH (settings.render_distance * CHUNK_SIZE)
#define WORLD_HEIGHT_BLOCKS (WORLD_HEIGHT * CHUNK_SIZE)

unsigned char* generate_light_texture() {
	size_t texture_size = WORLD_WIDTH * WORLD_HEIGHT_BLOCKS * WORLD_DEPTH * 4;
	unsigned char* texture_data = (unsigned char*)malloc(texture_size);
	if (!texture_data) {
		fprintf(stderr, "Failed to allocate memory for 3D texture\n");
		return NULL;
	}
	
	memset(texture_data, 0, texture_size);
	
	for (int cx = 0; cx < settings.render_distance; cx++) {
		for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
			for (int cz = 0; cz < settings.render_distance; cz++) {
				Chunk* chunk = &chunks[cx][cy][cz];
				if (!chunk->is_loaded) continue;
				
				int chunk_world_x = cx * CHUNK_SIZE;
				int chunk_world_y = cy * CHUNK_SIZE;
				int chunk_world_z = cz * CHUNK_SIZE;
				
				for (int bx = 0; bx < CHUNK_SIZE; bx++) {
					for (int by = 0; by < CHUNK_SIZE; by++) {
						for (int bz = 0; bz < CHUNK_SIZE; bz++) {
							uint8_t light = chunk->blocks[bx][by][bz].light_level;
							
							int tex_x = chunk_world_x + bx;
							int tex_y = chunk_world_y + by;
							int tex_z = chunk_world_z + bz;
						
							size_t idx = ((size_t)tex_z * WORLD_HEIGHT_BLOCKS * WORLD_WIDTH + 
										 (size_t)tex_y * WORLD_WIDTH + 
										 (size_t)tex_x) * 4;
							
							if (idx >= texture_size) {
								fprintf(stderr, "Index out of bounds: %zu (max: %zu)\n", idx, texture_size-1);
								continue;
							}
							
							uint8_t light_value = light * 17; // 17 = 255/15
							
							texture_data[idx]	 = light_value;  // R
							texture_data[idx + 1] = light_value;  // G
							texture_data[idx + 2] = light_value;  // B
							texture_data[idx + 3] = 255;		  // A
						}
					}
				}
			}
		}
	}
	
	return texture_data;
}