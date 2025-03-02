#include "main.h"

#define MAX_LIGHT_LEVEL 15
float sky_brightness = 1.0f;

void set_chunk_lighting(Chunk* chunk) {
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				if (chunk->blocks[x][y][z].id == 0)
					chunk->blocks[x][y][z].light_data = sky_brightness * MAX_LIGHT_LEVEL;
			}
		}
	}
}