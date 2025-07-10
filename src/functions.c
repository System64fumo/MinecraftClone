#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>

Chunk*** allocate_chunks() {
	Chunk*** chunks = (Chunk ***)malloc(settings.render_distance * sizeof(Chunk **));
	if (!chunks) return NULL;
	
	chunks[0] = (Chunk **)malloc(settings.render_distance * WORLD_HEIGHT * sizeof(Chunk *));
	if (!chunks[0]) {
		free(chunks);
		return NULL;
	}

	for (uint8_t i = 1; i < settings.render_distance; i++) {
		chunks[i] = chunks[i-1] + WORLD_HEIGHT;
	}

	Chunk* chunk_block = (Chunk *)calloc(settings.render_distance * WORLD_HEIGHT * settings.render_distance, sizeof(Chunk));
	if (!chunk_block) {
		free(chunks[0]);
		free(chunks);
		return NULL;
	}

	for (uint8_t i = 0; i < settings.render_distance; i++) {
		for (uint8_t j = 0; j < WORLD_HEIGHT; j++) {
			chunks[i][j] = chunk_block + (i * WORLD_HEIGHT * settings.render_distance) + (j * settings.render_distance);
		}
	}
	
	return chunks;
}

void free_chunks(Chunk*** chunks) {
	if (!chunks) return;
	free(chunks[0][0]);
	free(chunks[0]);
	free(chunks);
}

void calculate_chunk_and_block(int world_coord, int* chunk_coord, int* block_coord) {
	*chunk_coord = (world_coord < 0) ? ((world_coord + 1) / CHUNK_SIZE - 1) : (world_coord / CHUNK_SIZE);
	*block_coord = ((world_coord % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
}

bool is_chunk_in_bounds(int render_x, int chunk_y, int render_z) {
	return (render_x >= 0 && render_x < settings.render_distance &&
			chunk_y >= 0 && chunk_y < WORLD_HEIGHT &&
			render_z >= 0 && render_z < settings.render_distance);
}

void update_adjacent_chunks(Chunk*** chunks, uint8_t render_x, uint8_t render_y, uint8_t render_z, int block_x, int block_y, int block_z) {
	if (block_x == 0 && render_x > 0)
		chunks[render_x - 1][render_y][render_z].needs_update = true;
	else if (block_x == CHUNK_SIZE - 1 && render_x < settings.render_distance - 1)
		chunks[render_x + 1][render_y][render_z].needs_update = true;

	if (block_y == 0 && render_y > 0)
		chunks[render_x][render_y - 1][render_z].needs_update = true;
	else if (block_y == CHUNK_SIZE - 1 && render_y < WORLD_HEIGHT - 1)
		chunks[render_x][render_y + 1][render_z].needs_update = true;

	if (block_z == 0 && render_z > 0)
		chunks[render_x][render_y][render_z - 1].needs_update = true;
	else if (block_z == CHUNK_SIZE - 1 && render_z < settings.render_distance - 1)
		chunks[render_x][render_y][render_z + 1].needs_update = true;
}

Block* get_block_at(Chunk*** chunks, int world_block_x, int world_block_y, int world_block_z) {
	Block* block = NULL;
	int chunk_x, chunk_z, block_x, block_z;
	calculate_chunk_and_block(world_block_x, &chunk_x, &block_x);
	calculate_chunk_and_block(world_block_z, &chunk_z, &block_z);

	int chunk_y = world_block_y / CHUNK_SIZE;
	int block_y = ((world_block_y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

	int render_x = chunk_x - world_offset_x;
	int render_z = chunk_z - world_offset_z;

	if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
		Chunk* chunk = &chunks[render_x][chunk_y][render_z];
		Block* block = &chunk->blocks[block_x][block_y][block_z];
		return block;
	}

	return NULL;
}

int is_block_solid(Chunk*** chunks, int world_block_x, int world_block_y, int world_block_z) {
	Block* block = get_block_at(chunks, world_block_x, world_block_y, world_block_z);
	if (block == NULL)
		return -1;
	else
		return !(block->id == 0 || block->id == 6 || block->id == 37 || block->id == 38 || block->id == 39 || block->id == 40 || block->id == 8 || block->id == 9 || block->id == 10 || block->id == 11);
}