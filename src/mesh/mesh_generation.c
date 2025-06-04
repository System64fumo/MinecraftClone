#include "main.h"
#include <stdlib.h>
#include <string.h>

void generate_cross_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	uint8_t texture_id = 0;

	// First diagonal plane
	texture_id = block_data[block->id][2+0];
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 1.0f, 0, texture_id, 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 0.0f, 0, texture_id, 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 0.0f, 0, texture_id, 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 1.0f, 0, texture_id, 1, 1};

	// Second diagonal plane
	texture_id = block_data[block->id][2+1];
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 1.0f, 1, texture_id, 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 0.0f, 1, texture_id, 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 0.0f, 1, texture_id, 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 1.0f, 1, texture_id, 1, 1};

	// Back face of first plane
	texture_id = block_data[block->id][2+2];
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 0.0f, 2, texture_id, 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 1.0f, 2, texture_id, 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 1.0f, 2, texture_id, 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 0.0f, 2, texture_id, 1, 1};

	// Back face of second plane
	texture_id = block_data[block->id][2+3];
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 0.0f, 3, texture_id, 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 1.0f, 3, texture_id, 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 1.0f, 3, texture_id, 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 0.0f, 3, texture_id, 1, 1};
}

void generate_slab_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	float height = 0.5f;
	uint8_t texture_id = 0;

	// Generate vertices for each face
	texture_id = block_data[block->id][2+0];
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 0, texture_id, 1, 0}; // Front
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 0, texture_id, 0, 0}; // Front
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 0, texture_id, 0, 0.5}; // Front
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 0, texture_id, 1, 0.5}; // Front

	texture_id = block_data[block->id][2+1];
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 1, texture_id, 1, 0}; // Left
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 1, texture_id, 0, 0}; // Left
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 1, texture_id, 0, 0.5}; // Left
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 1, texture_id, 1, 0.5}; // Left

	texture_id = block_data[block->id][2+2];
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 2, texture_id, 1, 0}; // Back
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 2, texture_id, 0, 0}; // Back
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 2, texture_id, 0, 0.5}; // Back
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 2, texture_id, 1, 0.5}; // Back

	texture_id = block_data[block->id][2+3];
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 3, texture_id, 1, 0}; // Right
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 3, texture_id, 0, 0}; // Right
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 3, texture_id, 0, 0.5}; // Right
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 3, texture_id, 1, 0.5}; // Right

	texture_id = block_data[block->id][2+4];
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 4, texture_id, 1, 0}; // Bottom
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 4, texture_id, 0, 0}; // Bottom
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 4, texture_id, 0, 1}; // Bottom
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 4, texture_id, 1, 1}; // Bottom

	texture_id = block_data[block->id][2+5];
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 5, texture_id, 1, 0}; // Top
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 5, texture_id, 0, 0}; // Top
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 5, texture_id, 0, 1}; // Top
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 5, texture_id, 1, 1}; // Top
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

	switch (face) {
		case 0: // Front (Z+)
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, texture_id, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, texture_id, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, texture_id, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, texture_id, width_blocks, height_blocks};
			break;
		case 1: // Left (X-)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, texture_id, depth_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, texture_id, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, texture_id, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, texture_id, depth_blocks, height_blocks};
			break;
		case 2: // Back (Z-)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, texture_id, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, texture_id, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, texture_id, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, texture_id, width_blocks, height_blocks};
			break;
		case 3: // Right (X+)
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, texture_id, depth_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, texture_id, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, texture_id, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, texture_id, depth_blocks, height_blocks};
			break;
		case 4: // Bottom (Y-)
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, texture_id, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, texture_id, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, texture_id, 0.0f, depth_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, texture_id, width_blocks, depth_blocks};
			break;
		case 5: // Top (Y+)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, texture_id, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, texture_id, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, texture_id, 0.0f, depth_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, texture_id, width_blocks, depth_blocks};
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
	if (chunk == NULL) {
		fprintf(stderr, "Error: Null chunk pointer passed to generate_chunk_mesh\n");
		return;
	}

	// Clear existing face data
	for (int face = 0; face < 6; face++) {
		free(chunk->faces[face].vertices);
		free(chunk->faces[face].indices);
		chunk->faces[face].vertices = NULL;
		chunk->faces[face].indices = NULL;
		chunk->faces[face].vertex_count = 0;
		chunk->faces[face].index_count = 0;

		free(chunk->transparent_faces[face].vertices);
		free(chunk->transparent_faces[face].indices);
		chunk->transparent_faces[face].vertices = NULL;
		chunk->transparent_faces[face].indices = NULL;
		chunk->transparent_faces[face].vertex_count = 0;
		chunk->transparent_faces[face].index_count = 0;
	}

	const float world_x = chunk->x * CHUNK_SIZE;
	const float world_y = chunk->y * CHUNK_SIZE;
	const float world_z = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE] = {0};

	// First pass: Handle regular blocks using greedy meshing
	for (uint8_t face = 0; face < 6; face++) {
		Vertex face_vertices[MAX_VERTICES / 6] = {0};
		uint32_t face_indices[MAX_VERTICES / 6] = {0};
		uint32_t face_vertex_count = 0;
		uint32_t face_index_count = 0;

		Vertex transparent_face_vertices[MAX_VERTICES / 6] = {0};
		uint32_t transparent_face_indices[MAX_VERTICES / 6] = {0};
		uint32_t transparent_face_vertex_count = 0;
		uint32_t transparent_face_index_count = 0;

		for (uint8_t d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			for (uint8_t v = 0; v < CHUNK_SIZE; v++) {
				for (uint8_t u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					uint8_t x, y, z;
					map_coordinates(face, u, v, d, &x, &y, &z);

					if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
						continue;
					}

					Block* block = &chunk->blocks[x][y][z];
					if (block == NULL || block->id == 0 || block_data[block->id][0] != 0 || !is_face_visible(chunk, x, y, z, face)) 
						continue;

					uint8_t width = find_width(chunk, face, u, v, x, y, z, mask, block);
					uint8_t height = find_height(chunk, face, u, v, x, y, z, mask, block, width);

					for (uint8_t dy = 0; dy < height && (v + dy) < CHUNK_SIZE; dy++) {
						uint8_t end_u = u + width;
						if (end_u > CHUNK_SIZE) end_u = CHUNK_SIZE;
						memset(&mask[v + dy][u], true, (end_u - u) * sizeof(bool));
					}

					bool is_transparent = block_data[block->id][1] != 0;
					
					if (is_transparent) {
						uint16_t base_vertex = transparent_face_vertex_count;
						generate_vertices(face, x + world_x, y + world_y, z + world_z, width, height, block, 
										transparent_face_vertices, &transparent_face_vertex_count);
						generate_indices(base_vertex, transparent_face_indices, &transparent_face_index_count);
					} else {
						uint16_t base_vertex = face_vertex_count;
						generate_vertices(face, x + world_x, y + world_y, z + world_z, width, height, block, 
										face_vertices, &face_vertex_count);
						generate_indices(base_vertex, face_indices, &face_index_count);
					}
				}
			}
		}

		// Store face data
		if (face_vertex_count > 0) {
			chunk->faces[face].vertices = malloc(face_vertex_count * sizeof(Vertex));
			chunk->faces[face].indices = malloc(face_index_count * sizeof(uint32_t));
			if (chunk->faces[face].vertices && chunk->faces[face].indices) {
				memcpy(chunk->faces[face].vertices, face_vertices, face_vertex_count * sizeof(Vertex));
				memcpy(chunk->faces[face].indices, face_indices, face_index_count * sizeof(uint32_t));
				chunk->faces[face].vertex_count = face_vertex_count;
				chunk->faces[face].index_count = face_index_count;
			}
		}

		if (transparent_face_vertex_count > 0) {
			chunk->transparent_faces[face].vertices = malloc(transparent_face_vertex_count * sizeof(Vertex));
			chunk->transparent_faces[face].indices = malloc(transparent_face_index_count * sizeof(uint32_t));
			if (chunk->transparent_faces[face].vertices && chunk->transparent_faces[face].indices) {
				memcpy(chunk->transparent_faces[face].vertices, transparent_face_vertices, transparent_face_vertex_count * sizeof(Vertex));
				memcpy(chunk->transparent_faces[face].indices, transparent_face_indices, transparent_face_index_count * sizeof(uint32_t));
				chunk->transparent_faces[face].vertex_count = transparent_face_vertex_count;
				chunk->transparent_faces[face].index_count = transparent_face_index_count;
			}
		}
	}

	// Second pass: Handle special blocks (cross, slab)
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block == NULL) continue;
				
				uint8_t block_type = block_data[block->id][0];
				if (block_type == 0) continue;

				bool is_transparent = block_data[block->id][1] != 0;
				FaceMesh* target_faces = is_transparent ? chunk->transparent_faces : chunk->faces;

				Vertex special_vertices[24] = {0};
				uint32_t vertex_count = 0;

				if (block_type == 2) {
					generate_cross_vertices(x + world_x, y + world_y, z + world_z, block, special_vertices, &vertex_count);
				} else if (block_type == 1) {
					generate_slab_vertices(x + world_x, y + world_y, z + world_z, block, special_vertices, &vertex_count);
				}

				// Distribute vertices to appropriate faces
				for (uint32_t i = 0; i < vertex_count; i++) {
					Vertex v = special_vertices[i];
					uint8_t face = v.face_id;
					if (face >= 6) continue;

					// Add vertex to appropriate face
					size_t new_vertex_count = target_faces[face].vertex_count + 1;
					target_faces[face].vertices = realloc(target_faces[face].vertices, new_vertex_count * sizeof(Vertex));
					target_faces[face].vertices[target_faces[face].vertex_count++] = v;
				}

				// Generate indices for each face
				for (uint8_t face = 0; face < 6; face++) {
					if (target_faces[face].vertex_count == 0) continue;

					// For each quad (4 vertices) in this face, generate indices
					uint32_t quads = target_faces[face].vertex_count / 4;
					size_t new_index_count = target_faces[face].index_count + quads * 6;
					target_faces[face].indices = realloc(target_faces[face].indices, new_index_count * sizeof(uint32_t));

					for (uint32_t q = 0; q < quads; q++) {
						uint32_t base_vertex = target_faces[face].vertex_count - (quads - q) * 4;
						generate_indices(base_vertex, target_faces[face].indices, &target_faces[face].index_count);
					}
				}
			}
		}
	}

	chunk->needs_update = false;
}