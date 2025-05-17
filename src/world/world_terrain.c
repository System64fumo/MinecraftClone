#include "main.h"
#include "world.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z) {
	int chunk_base_y = chunk_y * CHUNK_SIZE;
	int world_x_base = chunk_x * CHUNK_SIZE;
	int world_z_base = chunk_z * CHUNK_SIZE;
	bool empty_chunk = true;

	// Pre-calculate noise for each (x,z) position
	float continent_noise[CHUNK_SIZE][CHUNK_SIZE];
	float flatness[CHUNK_SIZE][CHUNK_SIZE];
	float mountain_noise[CHUNK_SIZE][CHUNK_SIZE];
	float height_map[CHUNK_SIZE][CHUNK_SIZE];
	bool is_beach_map[CHUNK_SIZE][CHUNK_SIZE];
	bool is_ocean_map[CHUNK_SIZE][CHUNK_SIZE];

	// First pass: calculate all 2D noise values
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		float world_x = world_x_base + x;
		for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
			float world_z = world_z_base + z;

			// Continent noise
			continent_noise[x][z] = (stb_perlin_noise3(world_x * continent_scale, 0.0f, world_z * continent_scale, 0, 0, 0) + 1.0f) * 0.5f;
			float continent_height = (continent_noise[x][z] - 0.5f) * 128;

			// Flatness
			flatness[x][z] = powf((stb_perlin_noise3(world_x * flatness_scale, 0.0f, world_z * flatness_scale, 0, 0, 0) + 1.0f) * 0.5f, 2.0f) * 20.0f;

			// Mountain noise
			mountain_noise[x][z] = powf((stb_perlin_noise3(world_x * mountain_scale, 0.0f, world_z * mountain_scale, 0, 0, 0) + 1.0f) * 0.5f, 2.5f) * 64;

			// Final height
			height_map[x][z] = (continent_height + mountain_noise[x][z] - flatness[x][z]) + (4 * CHUNK_SIZE);
			
			// Ocean/beach flags
			int height = (int)height_map[x][z];
			is_ocean_map[x][z] = (height < SEA_LEVEL);
			is_beach_map[x][z] = (height >= SEA_LEVEL - 3 && height <= SEA_LEVEL + 2);
		}
	}

	// Second pass: generate blocks using cached values
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		float world_x = world_x_base + x;
		for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
			float world_z = world_z_base + z;
			int height = (int)height_map[x][z];
			bool is_beach = is_beach_map[x][z];
			bool is_ocean = is_ocean_map[x][z];

			for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
				int absolute_y = chunk_base_y + y;

				// Only calculate cave noise if we're below surface - 5 blocks
				bool is_cave = false;
				if (absolute_y < height - 5) {
					float cave_noise = stb_perlin_noise3(world_x * cave_scale, absolute_y * cave_scale, world_z * cave_scale, 0, 0, 0);
					float cave_simplex = stb_perlin_noise3(world_x * cave_simplex_scale, absolute_y * cave_simplex_scale, world_z * cave_simplex_scale, 0, 0, 0);
					float cave_value = (cave_noise + cave_simplex) * 0.5f;

					float depth_factor = 1.0f - (float)(absolute_y) / (float)(height);
					depth_factor = powf(depth_factor, 3.0f);
					float cave_threshold = 0.55f - (0.3f * depth_factor);

					is_cave = (cave_value > cave_threshold);
				}

				if (absolute_y == 0) {
					chunk->blocks[x][y][z] = (Block){.id = 7};  // Bedrock
					empty_chunk = false;
				}
				else if (absolute_y > height) {
					// Air or Water
					if (absolute_y <= SEA_LEVEL) {
						chunk->blocks[x][y][z] = (Block){.id = 9};  // Water
						empty_chunk = false;
					} else {
						chunk->blocks[x][y][z] = (Block){.id = 0};  // Air
					}
				}
				else if (is_cave) {
					chunk->blocks[x][y][z] = (Block){.id = 0};  // Air (Cave)
				}
				else {
					// Surface and underground terrain
					if (absolute_y == height) {
						if (is_beach) {
							chunk->blocks[x][y][z] = (Block){.id = 12};  // Sand (Beach)
							empty_chunk = false;
						} else if (is_ocean) {
							chunk->blocks[x][y][z] = (Block){.id = 12};  // Sand (Ocean floor)
							empty_chunk = false;
						} else {
							chunk->blocks[x][y][z] = (Block){.id = 2};  // Grass
							empty_chunk = false;
						}
					}
					else if (absolute_y >= height - 3) {
						if (is_beach || (is_ocean && absolute_y >= SEA_LEVEL - 3)) {
							chunk->blocks[x][y][z] = (Block){.id = 12};  // Sand
							empty_chunk = false;
						} else {
							chunk->blocks[x][y][z] = (Block){.id = 1};  // Dirt
							empty_chunk = false;
						}
					}
					else {
						chunk->blocks[x][y][z] = (Block){.id = 3};  // Stone
						empty_chunk = false;
					}
				}
			}
		}
	}

	if (empty_chunk)
		chunk->needs_update = false;
}