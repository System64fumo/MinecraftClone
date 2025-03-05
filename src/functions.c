#include "main.h"
#include "world.h"
#include <stdlib.h>

Chunk*** allocate_chunks(int render_distance, int world_height) {
	Chunk ***chunks = (Chunk ***)malloc(render_distance * sizeof(Chunk **));
	for (int i = 0; i < render_distance; i++) {
		chunks[i] = (Chunk **)malloc(world_height * sizeof(Chunk *));
		for (int j = 0; j < world_height; j++) {
			chunks[i][j] = (Chunk *)malloc(render_distance * sizeof(Chunk));
		}
	}
	return chunks;
}

void free_chunks(Chunk ***chunks, int render_distance, int world_height) {
	for (int i = 0; i < render_distance; i++) {
		for (int j = 0; j < world_height; j++) {
			free(chunks[i][j]);
		}
		free(chunks[i]);
	}
	free(chunks);
}

void calculate_chunk_and_block(int world_coord, int* chunk_coord, int* block_coord) {
	*chunk_coord = (world_coord < 0) ? ((world_coord + 1) / CHUNK_SIZE - 1) : (world_coord / CHUNK_SIZE);
	*block_coord = ((world_coord % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
}

bool is_chunk_in_bounds(int render_x, int chunk_y, int render_z) {
	return (render_x >= 0 && render_x < RENDER_DISTANCE &&
			chunk_y >= 0 && chunk_y < WORLD_HEIGHT &&
			render_z >= 0 && render_z < RENDER_DISTANCE);
}

void update_adjacent_chunks(int render_x, int chunk_y, int render_z, int block_x, int block_y, int block_z) {
	if (block_x == 0 && render_x > 0)
		chunks[render_x - 1][chunk_y][render_z].needs_update = true;
	else if (block_x == CHUNK_SIZE - 1 && render_x < RENDER_DISTANCE - 1)
		chunks[render_x + 1][chunk_y][render_z].needs_update = true;

	if (block_y == 0 && chunk_y > 0)
		chunks[render_x][chunk_y - 1][render_z].needs_update = true;
	else if (block_y == CHUNK_SIZE - 1 && chunk_y < WORLD_HEIGHT - 1)
		chunks[render_x][chunk_y + 1][render_z].needs_update = true;

	if (block_z == 0 && render_z > 0)
		chunks[render_x][chunk_y][render_z - 1].needs_update = true;
	else if (block_z == CHUNK_SIZE - 1 && render_z < RENDER_DISTANCE - 1)
		chunks[render_x][chunk_y][render_z + 1].needs_update = true;
}

// TODO: Avoid using last_cx and last_cz
Block* get_block_at(int world_block_x, int world_block_y, int world_block_z) {
	int chunk_x, chunk_z, block_x, block_z;
	calculate_chunk_and_block(world_block_x, &chunk_x, &block_x);
	calculate_chunk_and_block(world_block_z, &chunk_z, &block_z);

	int chunk_y = world_block_y / CHUNK_SIZE;
	int block_y = ((world_block_y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

	int render_x = chunk_x - last_cx;
	int render_z = chunk_z - last_cz;

	if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
		Chunk* chunk = &chunks[render_x][chunk_y][render_z];
		if (chunk->needs_update)
			return NULL;
		Block* block = &chunk->blocks[block_x][block_y][block_z];
		return block;
	}

	return NULL;
}

bool is_block_solid(int world_block_x, int world_block_y, int world_block_z) {
	Block* block = get_block_at(world_block_x, world_block_y, world_block_z);
	bool is_solid = block != NULL && block->id != 0;
	return is_solid;
}