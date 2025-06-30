#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

void process_chunks() {	
	pthread_mutex_lock(&chunks_mutex);
	// Process all chunks directly
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				Chunk* chunk = &chunks[x][y][z];
				
				if (chunk->needs_update) {
					// Only process if all neighbors are loaded
					if (are_all_neighbors_loaded(x, y, z)) {
						set_chunk_lighting(chunk);
						generate_chunk_mesh(chunk);
						chunk->needs_update = false;
						mesh_needs_rebuild = true;
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&chunks_mutex);
}