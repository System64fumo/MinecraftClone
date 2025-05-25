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

void generate_vertices(uint8_t face, float x, float y, float z, uint8_t width, uint8_t height, Block* block, Vertex vertices[], uint32_t* vertex_count, uint8_t light_data) {
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
	// Safety check for null chunk pointer
	if (chunk == NULL) {
		fprintf(stderr, "Error: Null chunk pointer passed to generate_chunk_mesh\n");
		return;
	}

	// Initialize vertex and index arrays with safe maximum sizes
	Vertex vertices[MAX_VERTICES] = {0};
	uint32_t indices[MAX_VERTICES] = {0};
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;
	
	Vertex transparent_vertices[MAX_VERTICES] = {0};
	uint32_t transparent_indices[MAX_VERTICES] = {0};
	uint32_t transparent_vertex_count = 0;
	uint32_t transparent_index_count = 0;

	// Calculate world position with bounds checking
	if (chunk->x >= CHUNK_SIZE || chunk->y >= WORLD_HEIGHT || chunk->z >= CHUNK_SIZE) {
		fprintf(stderr, "Error: Invalid chunk coordinates (%d, %d, %d)\n", 
			   chunk->x, chunk->y, chunk->z);
		return;
	}

	const float world_x = chunk->x * CHUNK_SIZE;
	const float world_y = chunk->y * CHUNK_SIZE;
	const float world_z = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE] = {0};

	// First pass: Handle regular blocks using greedy meshing
	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			for (uint8_t v = 0; v < CHUNK_SIZE; v++) {
				for (uint8_t u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					uint8_t x, y, z;
					map_coordinates(face, u, v, d, &x, &y, &z);

					// Bounds checking for block access
					if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
						continue;
					}

					Block* block = &chunk->blocks[x][y][z];
					if (block == NULL || block->id == 0 || block_data[block->id][0] != 0 || !is_face_visible(chunk, x, y, z, face)) 
						continue;

					uint8_t width = find_width(chunk, face, u, v, x, y, z, mask, block);
					uint8_t height = find_height(chunk, face, u, v, x, y, z, mask, block, width);

					// Check array bounds before marking
					for (uint8_t dy = 0; dy < height && (v + dy) < CHUNK_SIZE; dy++) {
						uint8_t end_u = u + width;
						if (end_u > CHUNK_SIZE) end_u = CHUNK_SIZE;
						memset(&mask[v + dy][u], true, (end_u - u) * sizeof(bool));
					}

					uint8_t adjacent_light_data = 15;
					bool is_transparent = block_data[block->id][1] != 0;
					
					// Check vertex buffer capacity before adding
					if (is_transparent) {
						if (transparent_vertex_count + 4 > MAX_VERTICES || transparent_index_count + 6 > MAX_VERTICES) {
							fprintf(stderr, "Warning: Transparent vertex/index buffer overflow\n");
							continue;
						}
						uint16_t base_vertex = transparent_vertex_count;
						generate_vertices(face, x + world_x, y + world_y, z + world_z, width, height, block, 
										transparent_vertices, &transparent_vertex_count, adjacent_light_data);
						generate_indices(base_vertex, transparent_indices, &transparent_index_count);
					} else {
						if (vertex_count + 4 > MAX_VERTICES || index_count + 6 > MAX_VERTICES) {
							fprintf(stderr, "Warning: Opaque vertex/index buffer overflow\n");
							continue;
						}
						uint16_t base_vertex = vertex_count;
						generate_vertices(face, x + world_x, y + world_y, z + world_z, width, height, block, 
										vertices, &vertex_count, adjacent_light_data);
						generate_indices(base_vertex, indices, &index_count);
					}
				}
			}
		}
	}

	// Second pass: Handle special blocks (cross, slab) with bounds checking
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block == NULL) continue;
				
				uint8_t block_type = block_data[block->id][0];
				if (block_type == 0) continue;

				bool is_transparent = block_data[block->id][1] != 0;
				
				if (is_transparent) {
					// Check capacity for cross (16 vertices, 24 indices) or slab (24 vertices, 36 indices)
					if ((block_type == 2 && (transparent_vertex_count + 16 > MAX_VERTICES || transparent_index_count + 24 > MAX_VERTICES)) ||
						(block_type == 1 && (transparent_vertex_count + 24 > MAX_VERTICES || transparent_index_count + 36 > MAX_VERTICES))) {
						fprintf(stderr, "Warning: Transparent special block buffer overflow\n");
						continue;
					}
					
					uint32_t base_vertex = transparent_vertex_count;
					if (block_type == 2) {
						generate_cross_vertices(x + world_x, y + world_y, z + world_z, block, 
											  transparent_vertices, &transparent_vertex_count);
						for (uint8_t i = 0; i < 4; i++) {
							generate_indices(base_vertex + (i * 4), transparent_indices, &transparent_index_count);
						}
					} else if (block_type == 1) {
						generate_slab_vertices(x + world_x, y + world_y, z + world_z, block, 
											 transparent_vertices, &transparent_vertex_count);
						for (uint8_t i = 0; i < 6; i++) {
							generate_indices(base_vertex + (i * 4), transparent_indices, &transparent_index_count);
						}
					}
				} else {
					// Similar capacity check for opaque blocks
					if ((block_type == 2 && (vertex_count + 16 > MAX_VERTICES || index_count + 24 > MAX_VERTICES)) ||
						(block_type == 1 && (vertex_count + 24 > MAX_VERTICES || index_count + 36 > MAX_VERTICES))) {
						fprintf(stderr, "Warning: Opaque special block buffer overflow\n");
						continue;
					}
					
					uint32_t base_vertex = vertex_count;
					if (block_type == 2) {
						generate_cross_vertices(x + world_x, y + world_y, z + world_z, block, 
											  vertices, &vertex_count);
						for (uint8_t i = 0; i < 4; i++) {
							generate_indices(base_vertex + (i * 4), indices, &index_count);
						}
					} else if (block_type == 1) {
						generate_slab_vertices(x + world_x, y + world_y, z + world_z, block, 
											 vertices, &vertex_count);
						for (uint8_t i = 0; i < 6; i++) {
							generate_indices(base_vertex + (i * 4), indices, &index_count);
						}
					}
				}
			}
		}
	}

	// Free existing data if any
	free(chunk->vertices);
	free(chunk->indices);
	free(chunk->transparent_vertices);
	free(chunk->transparent_indices);
	
	// Initialize to NULL
	chunk->vertices = NULL;
	chunk->indices = NULL;
	chunk->transparent_vertices = NULL;
	chunk->transparent_indices = NULL;

	// Allocate and copy only if we have data
	if (vertex_count > 0) {
		chunk->vertices = malloc(vertex_count * sizeof(Vertex));
		chunk->indices = malloc(index_count * sizeof(uint32_t));
		if (chunk->vertices && chunk->indices) {
			memcpy(chunk->vertices, vertices, vertex_count * sizeof(Vertex));
			memcpy(chunk->indices, indices, index_count * sizeof(uint32_t));
		} else {
			fprintf(stderr, "Error: Failed to allocate memory for chunk mesh\n");
			free(chunk->vertices);
			free(chunk->indices);
			chunk->vertices = NULL;
			chunk->indices = NULL;
			vertex_count = 0;
			index_count = 0;
		}
	}

	if (transparent_vertex_count > 0) {
		chunk->transparent_vertices = malloc(transparent_vertex_count * sizeof(Vertex));
		chunk->transparent_indices = malloc(transparent_index_count * sizeof(uint32_t));
		if (chunk->transparent_vertices && chunk->transparent_indices) {
			memcpy(chunk->transparent_vertices, transparent_vertices, transparent_vertex_count * sizeof(Vertex));
			memcpy(chunk->transparent_indices, transparent_indices, transparent_index_count * sizeof(uint32_t));
		} else {
			fprintf(stderr, "Error: Failed to allocate memory for transparent chunk mesh\n");
			free(chunk->transparent_vertices);
			free(chunk->transparent_indices);
			chunk->transparent_vertices = NULL;
			chunk->transparent_indices = NULL;
			transparent_vertex_count = 0;
			transparent_index_count = 0;
		}
	}

	// Update counts only after successful allocation
	chunk->vertex_count = vertex_count;
	chunk->index_count = index_count;
	chunk->transparent_vertex_count = transparent_vertex_count;
	chunk->transparent_index_count = transparent_index_count;
	chunk->needs_update = false;
}