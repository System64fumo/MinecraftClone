#include "main.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int last_cx = -1;
static int last_cz = -1;

void load_around_entity(Entity* entity) {
    static Chunk temp_chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];
    
    int center_cx = (int)fmaxf(0, fminf(WORLD_SIZE, floorf(entity->x / CHUNK_SIZE) - (RENDERR_DISTANCE / 2)));
    int center_cz = (int)fmaxf(0, fminf(WORLD_SIZE, floorf(entity->z / CHUNK_SIZE) - (RENDERR_DISTANCE / 2)));

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
    } else if (dx < 0) { // Moving West
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
    } else if (dz < 0) { // Moving North
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
                    // Mark adjacent existing chunks for update
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

	chunk->needs_update = true;
}