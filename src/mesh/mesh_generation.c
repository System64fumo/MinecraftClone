#include "main.h"
#include <stdlib.h>
#include <string.h>

void generate_cross_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	uint8_t texture_id = 0;
	uint8_t light_data = block->light_data;

	// First diagonal plane
	texture_id = block_data[block->id][2+0];
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 1.0f, 0, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 0.0f, 0, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 0.0f, 0, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 1.0f, 0, texture_id, 1, 1, light_data};

	// Second diagonal plane
	texture_id = block_data[block->id][2+1];
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 1.0f, 1, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 0.0f, 1, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 0.0f, 1, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 1.0f, 1, texture_id, 1, 1, light_data};

	// Back face of first plane
	texture_id = block_data[block->id][2+2];
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 0.0f, 2, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 1.0f, 2, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 1.0f, 2, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 0.0f, 2, texture_id, 1, 1, light_data};

	// Back face of second plane
	texture_id = block_data[block->id][2+3];
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 0.0f, 3, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 1.0f, 3, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 1.0f, 3, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 0.0f, 3, texture_id, 1, 1, light_data};
}

void generate_slab_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	float height = 0.5f;
	uint8_t texture_id = 0;
	uint8_t light_data = block->light_data;

	// Generate vertices for a half-height block
	// Top face
	texture_id = block_data[block->id][2+5];
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 5, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 5, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 5, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 5, texture_id, 1, 1, light_data};

	// Bottom face
	texture_id = block_data[block->id][2+4];
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 4, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 4, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 4, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 4, texture_id, 1, 1, light_data};

	// Front face
	texture_id = block_data[block->id][2+0];
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 0, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 0, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 0, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 0, texture_id, 1, 1, light_data};

	// Back face
	texture_id = block_data[block->id][2+2];
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 2, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 2, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 2, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 2, texture_id, 1, 1, light_data};

	// Left face
	texture_id = block_data[block->id][2+1];
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 1, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 1, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 1, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 1, texture_id, 1, 1, light_data};

	// Right face
	texture_id = block_data[block->id][2+3];
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 3, texture_id, 1, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 3, texture_id, 0, 0, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 3, texture_id, 0, 1, light_data};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 3, texture_id, 1, 1, light_data};
}

void generate_vertices(uint8_t face, float x, float y, float z, uint8_t width, uint8_t height, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	uint8_t width_blocks = (face == 1 || face == 3) ? 1 : width;
	uint8_t height_blocks = (face >= 4) ? 1 : height;
	uint8_t depth_blocks = (face == 0 || face == 2) ? 1 : (face >= 4 ? height : width);

	float x1 = x, x2 = x, y1 = y, y2 = y + height, z1 = z, z2 = z;

	if (face == 3 || face == 1) {
		z2 += width;
		if (face == 1) x1 += 1;
	} else {
		x2 += (face == 0 || face == 2 || face >= 4) ? width : 1;
		y2 = y + (face >= 4 ? 1 : height);
		z2 += (face == 0 || face == 2) ? 1 : (face >= 4 ? height : 1);
	}

	uint8_t texture_id = block_data[block->id][2+face];

	// Calculate the coordinates of the adjacent block
	int adjacent_x = (int)x;
	int adjacent_y = (int)y;
	int adjacent_z = (int)z;

	switch (face) {
		case 0: adjacent_z += 1; break; // Front (Z+)
		case 1: adjacent_x += 1; break; // Left (X-)
		case 2: adjacent_z -= 1; break; // Back (Z-)
		case 3: adjacent_x -= 1; break; // Right (X+)
		case 4: adjacent_y -= 1; break; // Bottom (Y-)
		case 5: adjacent_y += 1; break; // Top (Y+)
	}

	// Temporarily disabled due to block at pos issues
	// Get the adjacent block using the get_block_at function
	//Block* adjacent_block = get_block_at(adjacent_x, adjacent_y, adjacent_z);
	//uint8_t light_data = adjacent_block ? adjacent_block->light_data : 15; // Default to max light if no block is found

	uint8_t light_data = 15;

	switch (face) {
		case 0: // Front (Z+)
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, texture_id, width_blocks, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, texture_id, 0.0f, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, texture_id, 0.0f, height_blocks, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, texture_id, width_blocks, height_blocks, light_data};
			break;
		case 1: // Left (X-)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, texture_id, depth_blocks, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, texture_id, 0.0f, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, texture_id, 0.0f, height_blocks, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, texture_id, depth_blocks, height_blocks, light_data};
			break;
		case 2: // Back (Z-)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, texture_id, width_blocks, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, texture_id, 0.0f, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, texture_id, 0.0f, height_blocks, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, texture_id, width_blocks, height_blocks, light_data};
			break;
		case 3: // Right (X+)
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, texture_id, depth_blocks, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, texture_id, 0.0f, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, texture_id, 0.0f, height_blocks, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, texture_id, depth_blocks, height_blocks, light_data};
			break;
		case 4: // Bottom (Y-)
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, texture_id, width_blocks, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, texture_id, 0.0f, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, texture_id, 0.0f, depth_blocks, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, texture_id, width_blocks, depth_blocks, light_data};
			break;
		case 5: // Top (Y+)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, texture_id, width_blocks, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, texture_id, 0.0f, 0.0f, light_data};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, texture_id, 0.0f, depth_blocks, light_data};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, texture_id, width_blocks, depth_blocks, light_data};
			break;
	}
}

void generate_indices(uint32_t base_vertex, uint32_t indices[], uint32_t* index_count) {
	indices[(*index_count)++] = base_vertex;
	indices[(*index_count)++] = base_vertex + 1;
	indices[(*index_count)++] = base_vertex + 2;
	indices[(*index_count)++] = base_vertex;
	indices[(*index_count)++] = base_vertex + 2;
	indices[(*index_count)++] = base_vertex + 3;
}

void generate_chunk_mesh(Chunk* chunk) {
	Vertex vertices[MAX_VERTICES];
	uint32_t indices[MAX_VERTICES * 6];
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	const float world_x = chunk->x * CHUNK_SIZE;
	const float world_y = chunk->y * CHUNK_SIZE;
	const float world_z = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE];

	// First pass: Handle regular blocks using greedy meshing
	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			for (uint8_t v = 0; v < CHUNK_SIZE; v++) {
				for (uint8_t u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					uint8_t x, y, z;
					map_coordinates(face, u, v, d, &x, &y, &z);

					Block* block = &chunk->blocks[x][y][z];
					if (block->id == 0 || block_data[block->id][0] != 0 || !is_face_visible(chunk, x, y, z, face)) 
						continue;

					uint8_t width = find_width(chunk, face, u, v, x, y, z, mask, block);
					uint8_t height = find_height(chunk, face, u, v, x, y, z, mask, block, width);

					// Mark cells as processed
					for (uint8_t dy = 0; dy < height; dy++) {
						memset(&mask[v + dy][u], true, width * sizeof(bool));
					}

					uint16_t base_vertex = vertex_count;
					generate_vertices(face, x + world_x, y + world_y, z + world_z, width, height, block, vertices, &vertex_count);
					generate_indices(base_vertex, indices, &index_count);
				}
			}
		}
	}

	// Second pass: Handle special blocks (cross, slab)
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				uint8_t block_type = block_data[block->id][0];
				
				if (block_type == 0) continue;

				uint32_t base_vertex = vertex_count;
				if (block_type == 2) {
					generate_cross_vertices(x + world_x, y + world_y, z + world_z, block, vertices, &vertex_count);
					for (uint8_t i = 0; i < 4; i++) {
						generate_indices(base_vertex + (i * 4), indices, &index_count);
					}
				} else if (block_type == 1) {
					generate_slab_vertices(x + world_x, y + world_y, z + world_z, block, vertices, &vertex_count);
					for (uint8_t i = 0; i < 6; i++) {
						generate_indices(base_vertex + (i * 4), indices, &index_count);
					}
				}
			}
		}
	}

	chunk->vertex_count = vertex_count;
	chunk->index_count = index_count;

	free(chunk->vertices);
	free(chunk->indices);
	chunk->vertices = NULL;
	chunk->indices = NULL;

	// Only allocate if we have vertices to store
	if (vertex_count > 0) {
		size_t vertex_size = vertex_count * sizeof(Vertex);
		size_t index_size = index_count * sizeof(uint32_t);

		chunk->vertices = malloc(vertex_size);
		chunk->indices = malloc(index_size);

		memcpy(chunk->vertices, vertices, vertex_size);
		memcpy(chunk->indices, indices, index_size);
	}

	chunk->needs_update = false;
}