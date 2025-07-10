#include "main.h"
#include "world.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"
#include <math.h>

void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z) {
	int chunk_base_y = chunk_y * CHUNK_SIZE;
	int world_x_base = chunk_x * CHUNK_SIZE;
	int world_z_base = chunk_z * CHUNK_SIZE;
	bool empty_chunk = true;

	if (flat_world_gen) {
		for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
					int absolute_y = chunk_base_y + y;
					
					if (absolute_y == 0) {
						chunk->blocks[x][y][z] = (Block){.id = 7};  // Bedrock
						empty_chunk = false;
					}
					else if (absolute_y == 4) {
						chunk->blocks[x][y][z] = (Block){.id = 2};  // Grass
						empty_chunk = false;
					}
					else if (absolute_y > 0 && absolute_y < 4) {
						if (absolute_y >= 4 - 2) {
							chunk->blocks[x][y][z] = (Block){.id = 1};  // Dirt (2 layers)
						}
						else {
							chunk->blocks[x][y][z] = (Block){.id = 3};  // Stone
						}
						empty_chunk = false;
					}
					else {
						chunk->blocks[x][y][z] = (Block){.id = 0};  // Air
					}
				}
			}
		}
		
		if (empty_chunk)
			chunk->needs_update = false;
		return;
	}

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

			continent_noise[x][z] = (stb_perlin_noise3(world_x * continent_scale, 0.0f, world_z * continent_scale, 0, 0, 0) + 1.0f) * 0.5f;
			float continent_height = (continent_noise[x][z] - 0.5f) * 128;
			flatness[x][z] = powf((stb_perlin_noise3(world_x * flatness_scale, 0.0f, world_z * flatness_scale, 0, 0, 0) + 1.0f) * 0.5f, 2.0f) * 20.0f;
			mountain_noise[x][z] = powf((stb_perlin_noise3(world_x * mountain_scale, 0.0f, world_z * mountain_scale, 0, 0, 0) + 1.0f) * 0.5f, 2.5f) * 64;
			height_map[x][z] = (continent_height + mountain_noise[x][z] - flatness[x][z]) + (4 * CHUNK_SIZE);

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
					}
					else {
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
						}
						else if (is_ocean) {
							chunk->blocks[x][y][z] = (Block){.id = 12};  // Sand (Ocean floor)
							empty_chunk = false;
						}
						else {
							chunk->blocks[x][y][z] = (Block){.id = 2};  // Grass
							empty_chunk = false;
						}
					}
					else if (absolute_y >= height - 3) {
						if (is_beach || (is_ocean && absolute_y >= SEA_LEVEL - 3)) {
							chunk->blocks[x][y][z] = (Block){.id = 12};  // Sand
							empty_chunk = false;
						}
						else {
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

	// Third pass: Generate structures
	// Check for structures that could affect this chunk by checking a grid pattern
	// This ensures consistent structure placement regardless of chunk generation order
	int chunk_world_x_min = chunk_x * CHUNK_SIZE;
	int chunk_world_x_max = chunk_world_x_min + CHUNK_SIZE - 1;
	int chunk_world_z_min = chunk_z * CHUNK_SIZE;
	int chunk_world_z_max = chunk_world_z_min + CHUNK_SIZE - 1;
	
	// Calculate the range of structure origins that could affect this chunk
	int struct_search_min_x = chunk_world_x_min - (tree_structure.width / 2);
	int struct_search_max_x = chunk_world_x_max + (tree_structure.width / 2);
	int struct_search_min_z = chunk_world_z_min - (tree_structure.depth / 2);
	int struct_search_max_z = chunk_world_z_max + (tree_structure.depth / 2);
	
	// Check every possible structure origin position
	for (int structure_world_x = struct_search_min_x; structure_world_x <= struct_search_max_x; structure_world_x++) {
		for (int structure_world_z = struct_search_min_z; structure_world_z <= struct_search_max_z; structure_world_z++) {
			// Use a consistent way to determine surface height for structure placement
			float continent_n = (stb_perlin_noise3(structure_world_x * continent_scale, 0.0f, structure_world_z * continent_scale, 0, 0, 0) + 1.0f) * 0.5f;
			float continent_h = (continent_n - 0.5f) * 128;
			float flatness_n = powf((stb_perlin_noise3(structure_world_x * flatness_scale, 0.0f, structure_world_z * flatness_scale, 0, 0, 0) + 1.0f) * 0.5f, 2.0f) * 20.0f;
			float mountain_n = powf((stb_perlin_noise3(structure_world_x * mountain_scale, 0.0f, structure_world_z * mountain_scale, 0, 0, 0) + 1.0f) * 0.5f, 2.5f) * 64;
			float surface_height = (continent_h + mountain_n - flatness_n) + (4 * CHUNK_SIZE);
			
			int surface_y = (int)surface_height;
			bool is_grass = (surface_y > SEA_LEVEL && surface_y < SEA_LEVEL + 50); // Reasonable grass range
			
			// Check if we should place a tree at this position
			if (can_place_tree(structure_world_x, surface_y, structure_world_z, is_grass)) {
				// Generate only the part of the structure that falls within this chunk
				generate_structure_in_chunk(chunk, chunk_x, chunk_y, chunk_z, 
										   &tree_structure, structure_world_x, surface_y + 1, structure_world_z, 
										   &empty_chunk);
			}
		}
	}

	if (empty_chunk)
		chunk->needs_update = false;
}