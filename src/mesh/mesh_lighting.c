#include "main.h"
#include "config.h"

#define MAX_LIGHT_LEVEL 15

void set_chunk_lighting(Chunk* chunk) {
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				if (chunk->blocks[x][y][z].id == 0)
					chunk->blocks[x][y][z].light_data = settings.sky_brightness * MAX_LIGHT_LEVEL;
			}
		}
	}
}