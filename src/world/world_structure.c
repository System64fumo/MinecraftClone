#include "world.h"
#include "stb_perlin.h"

structure_block_t tree_blocks[] = {
	{0,0,0,17},{0,1,0,17},{0,2,0,17},{0,3,0,17},{0,4,0,17},

	{-2,3,-2,18},{-2,3,-1,18},{-2,3,0,18},{-2,3,1,18},{-2,3,2,18},
	{-1,3,-2,18},{-1,3,-1,18},{-1,3,0,18},{-1,3,1,18},{-1,3,2,18},
	{ 0,3,-2,18},{ 0,3,-1,18},            { 0,3,1,18},{ 0,3,2,18},
	{ 1,3,-2,18},{ 1,3,-1,18},{ 1,3,0,18},{ 1,3,1,18},{ 1,3,2,18},
	{ 2,3,-2,18},{ 2,3,-1,18},{ 2,3,0,18},{ 2,3,1,18},{ 2,3,2,18},

	{-2,4,-2,18},{-2,4,-1,18},{-2,4,0,18},{-2,4,1,18},{-2,4,2,18},
	{-1,4,-2,18},{-1,4,-1,18},{-1,4,0,18},{-1,4,1,18},{-1,4,2,18},
	{ 0,4,-2,18},{ 0,4,-1,18},            { 0,4,1,18},{ 0,4,2,18},
	{ 1,4,-2,18},{ 1,4,-1,18},{ 1,4,0,18},{ 1,4,1,18},{ 1,4,2,18},
	{ 2,4,-2,18},{ 2,4,-1,18},{ 2,4,0,18},{ 2,4,1,18},{ 2,4,2,18},

	{-1,5,-1,18},{-1,5,0,18},{-1,5,1,18},
	{ 0,5,-1,18},{ 0,5,0,18},{ 0,5,1,18},
	{ 1,5,-1,18},{ 1,5,0,18},{ 1,5,1,18},

	           {-1,6,0,18},
	{0,6,-1,18},{ 0,6,0,18},{0,6,1,18},
	            { 1,6,0,18},
};

structure_t tree_structure = {
	.blocks      = tree_blocks,
	.block_count = sizeof(tree_blocks) / sizeof(structure_block_t),
	.width  = 5,
	.height = 7,
	.depth  = 5
};

bool can_place_tree(int world_x, int surface_y, int world_z, bool is_grass_surface) {
	if (!is_grass_surface || surface_y <= SEA_LEVEL) return false;
	int grid = 8;
	if (((world_x % grid) + grid) % grid != 0) return false;
	if (((world_z % grid) + grid) % grid != 0) return false;
	float n = stb_perlin_noise3(world_x * structure_scale, 0.f, world_z * structure_scale, 0,0,0);
	return n > 0.05f;
}

static void place_block(Chunk *chunk, int lx, int ly, int lz,
                         uint8_t id, bool *empty) {
	if (lx < 0 || lx >= CHUNK_SIZE || ly < 0 || ly >= CHUNK_SIZE || lz < 0 || lz >= CHUNK_SIZE)
		return;
	if (chunk->blocks[lx][ly][lz].id == 0) {
		chunk->blocks[lx][ly][lz] = (Block){.id = id};
		*empty = false;
	}
}

void generate_structure_in_chunk(Chunk *chunk, int chunk_x, int chunk_y, int chunk_z,
                                 structure_t *s, int sx, int sy, int sz, bool *empty) {
	int bx0 = chunk_x * CHUNK_SIZE;
	int by0 = chunk_y * CHUNK_SIZE;
	int bz0 = chunk_z * CHUNK_SIZE;

	int smn_x = sx - s->width  / 2, smx_x = sx + s->width  / 2;
	int smn_y = sy,                  smx_y = sy + s->height;
	int smn_z = sz - s->depth  / 2, smx_z = sz + s->depth  / 2;

	if (smx_x < bx0 || smn_x > bx0 + CHUNK_SIZE - 1 ||
	    smx_y < by0 || smn_y > by0 + CHUNK_SIZE - 1 ||
	    smx_z < bz0 || smn_z > bz0 + CHUNK_SIZE - 1) return;

	for (int i = 0; i < s->block_count; i++) {
		structure_block_t *b = &s->blocks[i];
		place_block(chunk,
		            sx + b->x - bx0,
		            sy + b->y - by0,
		            sz + b->z - bz0,
		            b->block_id, empty);
	}
}
