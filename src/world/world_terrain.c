#include "main.h"
#include "world.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

const int SEA_LEVEL = 64;

const float continent_scale = 0.005f;
const float mountain_scale = 0.03f;
const float flatness_scale = 0.02f;
const float cave_scale = 0.1f;
const float cave_simplex_scale = 0.1f;
const float structure_scale = 0.1f;
const bool flat_world_gen = false;

typedef struct {
	int8_t x, y, z;
	uint8_t block_id;
} structure_block_t;

typedef struct {
	structure_block_t* blocks;
	uint8_t block_count;
	uint8_t width, height, depth;
} structure_t;

static structure_block_t tree_blocks[] = {
	// Trunk
	{0, 0, 0, 17},
	{0, 1, 0, 17},
	{0, 2, 0, 17},
	{0, 3, 0, 17},
	{0, 4, 0, 17},

	// Leaves
	{-2, 3, -2, 18}, {-2, 3, -1, 18}, {-2, 3, 0, 18}, {-2, 3, 1, 18}, {-2, 3, 2, 18},
	{-1, 3, -2, 18}, {-1, 3, -1, 18}, {-1, 3, 0, 18}, {-1, 3, 1, 18}, {-1, 3, 2, 18},
	{0, 3, -2, 18}, {0, 3, -1, 18}, {0, 3, 1, 18}, {0, 3, 2, 18},
	{1, 3, -2, 18}, {1, 3, -1, 18}, {1, 3, 0, 18}, {1, 3, 1, 18}, {1, 3, 2, 18},
	{2, 3, -2, 18}, {2, 3, -1, 18}, {2, 3, 0, 18}, {2, 3, 1, 18}, {2, 3, 2, 18},

	{-2, 4, -2, 18}, {-2, 4, -1, 18}, {-2, 4, 0, 18}, {-2, 4, 1, 18}, {-2, 4, 2, 18},
	{-1, 4, -2, 18}, {-1, 4, -1, 18}, {-1, 4, 0, 18}, {-1, 4, 1, 18}, {-1, 4, 2, 18},
	{0, 4, -2, 18}, {0, 4, -1, 18}, {0, 4, 1, 18}, {0, 4, 2, 18},
	{1, 4, -2, 18}, {1, 4, -1, 18}, {1, 4, 0, 18}, {1, 4, 1, 18}, {1, 4, 2, 18},
	{2, 4, -2, 18}, {2, 4, -1, 18}, {2, 4, 0, 18}, {2, 4, 1, 18}, {2, 4, 2, 18},

	{-1, 5, -1, 18},{-1, 5, 0, 18}, {-1, 5, 1, 18},
	{0, 5, -1, 18}, {0, 5, 0, 18}, {0, 5, 1, 18},
	{1, 5, -1, 18},	{1, 5, 0, 18}, {1, 5, 1, 18},

					{-1, 6, 0, 18},
	{0, 6, -1, 18}, {0, 6, 0, 18}, {0, 6, 1, 18},
					{1, 6, 0, 18},
};

static structure_t tree_structure = {
	.blocks = tree_blocks,
	.block_count = sizeof(tree_blocks) / sizeof(structure_block_t),
	.width = 5,
	.height = 7,
	.depth = 5
};

static bool should_generate_structure(int world_x, int world_z, float threshold) {
	float noise = stb_perlin_noise3(world_x * structure_scale, 0.0f, world_z * structure_scale, 0, 0, 0);
	return noise > threshold;
}

static bool can_place_tree(int world_x, int surface_y, int world_z, bool is_grass_surface) {
	// Trees only generate on grass surfaces above sea level
	if (!is_grass_surface || surface_y <= SEA_LEVEL) {
		return false;
	}
	
	// Use a grid-based approach to ensure minimum spacing
	int grid_size = 8;
	int grid_x = ((world_x % grid_size) + grid_size) % grid_size;
	int grid_z = ((world_z % grid_size) + grid_size) % grid_size;

	if (grid_x != 0 || grid_z != 0)
		return false;

	return should_generate_structure(world_x, world_z, 0.05f);
}

static void place_structure_block(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z, 
								 int local_x, int local_y, int local_z, uint8_t block_id, bool* empty_chunk) {
	// Check if the block position is within this chunk
	if (local_x >= 0 && local_x < CHUNK_SIZE && 
		local_y >= 0 && local_y < CHUNK_SIZE && 
		local_z >= 0 && local_z < CHUNK_SIZE) {
		
		// Only place if current block is air or can be replaced
		if (chunk->blocks[local_x][local_y][local_z].id == 0) {
			chunk->blocks[local_x][local_y][local_z] = (Block){.id = block_id};
			*empty_chunk = false;
		}
	}
}

static void generate_structure_in_chunk(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z,
									   structure_t* structure, int structure_world_x, int structure_world_y, int structure_world_z,
									   bool* empty_chunk) {
	int chunk_world_x_base = chunk_x * CHUNK_SIZE;
	int chunk_world_y_base = chunk_y * CHUNK_SIZE;
	int chunk_world_z_base = chunk_z * CHUNK_SIZE;

	// Check if structure intersects with this chunk
	int struct_min_x = structure_world_x - structure->width / 2;
	int struct_max_x = structure_world_x + structure->width / 2;
	int struct_min_y = structure_world_y;
	int struct_max_y = structure_world_y + structure->height;
	int struct_min_z = structure_world_z - structure->depth / 2;
	int struct_max_z = structure_world_z + structure->depth / 2;
	
	int chunk_min_x = chunk_world_x_base;
	int chunk_max_x = chunk_world_x_base + CHUNK_SIZE - 1;
	int chunk_min_y = chunk_world_y_base;
	int chunk_max_y = chunk_world_y_base + CHUNK_SIZE - 1;
	int chunk_min_z = chunk_world_z_base;
	int chunk_max_z = chunk_world_z_base + CHUNK_SIZE - 1;
	
	// Check if structure overlaps with chunk
	if (struct_max_x < chunk_min_x || struct_min_x > chunk_max_x ||
		struct_max_y < chunk_min_y || struct_min_y > chunk_max_y ||
		struct_max_z < chunk_min_z || struct_min_z > chunk_max_z) {
		return; // No overlap
	}
	
	// Place structure blocks that fall within this chunk
	for (uint8_t i = 0; i < structure->block_count; i++) {
		structure_block_t* block = &structure->blocks[i];
		
		int world_x = structure_world_x + block->x;
		int world_y = structure_world_y + block->y;
		int world_z = structure_world_z + block->z;
		
		int local_x = world_x - chunk_world_x_base;
		int local_y = world_y - chunk_world_y_base;
		int local_z = world_z - chunk_world_z_base;
		
		place_structure_block(chunk, chunk_x, chunk_y, chunk_z, local_x, local_y, local_z, block->block_id, empty_chunk);
	}
}

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
						} else {
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