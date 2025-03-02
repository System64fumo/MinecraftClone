#include "main.h"
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
		Block* block = &chunk->blocks[block_x][block_y][block_z];
		return block;
	}

	return NULL;
}

bool is_block_solid(int world_block_x, int world_block_y, int world_block_z) {
	Block* block = get_block_at(world_block_x, world_block_y, world_block_z);
	if (block != NULL)
		return block->id != 0;
	else
		return 0;
}