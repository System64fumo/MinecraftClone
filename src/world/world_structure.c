#include "world.h"
#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

bool should_generate_structure(int world_x, int world_z, float threshold) {
	float noise = stb_perlin_noise3(world_x * structure_scale, 0.0f, world_z * structure_scale, 0, 0, 0);
	return noise > threshold;
}

bool can_place_tree(int world_x, int surface_y, int world_z, bool is_grass_surface) {
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

void place_structure_block(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z, 
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

void generate_structure_in_chunk(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z,
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