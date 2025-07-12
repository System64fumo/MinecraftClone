#include "main.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

static const face_vertex_t cube_faces[6][4] = {
	{{{1,1,1}, {1,0}}, {{0,1,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,1}, {1,1}}},
	{{{1,1,0}, {1,0}}, {{1,1,1}, {0,0}}, {{1,0,1}, {0,1}}, {{1,0,0}, {1,1}}},
	{{{0,1,0}, {1,0}}, {{1,1,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,0}, {1,1}}},
	{{{0,1,1}, {1,0}}, {{0,1,0}, {0,0}}, {{0,0,0}, {0,1}}, {{0,0,1}, {1,1}}},
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},
	{{{0,1,1}, {0,0}}, {{1,1,1}, {1,0}}, {{1,1,0}, {1,1}}, {{0,1,0}, {0,1}}}
};

static const face_vertex_t slab_faces[6][4] = {
	{{{1,0.5,1}, {1,0}}, {{0,0.5,1}, {0,0}}, {{0,0,1}, {0,0.5}}, {{1,0,1}, {1,0.5}}},
	{{{1,0.5,0}, {1,0}}, {{1,0.5,1}, {0,0}}, {{1,0,1}, {0,0.5}}, {{1,0,0}, {1,0.5}}},
	{{{0,0.5,0}, {1,0}}, {{1,0.5,0}, {0,0}}, {{1,0,0}, {0,0.5}}, {{0,0,0}, {1,0.5}}},
	{{{0,0.5,1}, {1,0}}, {{0,0.5,0}, {0,0}}, {{0,0,0}, {0,0.5}}, {{0,0,1}, {1,0.5}}},
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},
	{{{0,0.5,1}, {0,0}}, {{1,0.5,1}, {1,0}}, {{1,0.5,0}, {1,1}}, {{0,0.5,0}, {0,1}}}
};

static const face_vertex_t cross_faces[4][4] = {
	{{{1,1,1}, {1,0}}, {{0,1,0}, {0,0}}, {{0,0,0}, {0,1}}, {{1,0,1}, {1,1}}},
	{{{0,1,1}, {1,0}}, {{1,1,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,1}, {1,1}}},
	{{{0,1,0}, {1,0}}, {{1,1,1}, {0,0}}, {{1,0,1}, {0,1}}, {{0,0,0}, {1,1}}},
	{{{1,1,0}, {1,0}}, {{0,1,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,0}, {1,1}}}
};

static const uint32_t quad_indices[6] = {0, 1, 2, 0, 2, 3};

void add_quad(Chunk* chunk, float x, float y, float z, uint8_t normal, uint8_t texture_id,
			  const face_vertex_t face_data[4], uint8_t width, uint8_t height,
			  Vertex vertices[], uint32_t indices[],
			  uint32_t* vertex_count, uint32_t* index_count) {
	
	uint32_t base_vertex = *vertex_count;

	float width_blocks = (normal == 1 || normal == 3) ? 1.0f : (float)width;
	float height_blocks = (normal >= 4) ? 1.0f : (float)height;
	float depth_blocks = (normal == 0 || normal == 2) ? 1.0f : (normal >= 4 ? (float)height : (float)width);

	uint8_t block_id = 0;
	if (chunk) {
		uint8_t local_x = (uint8_t)(x - (chunk->x * CHUNK_SIZE));
		uint8_t local_y = (uint8_t)(y - (chunk->y * CHUNK_SIZE));
		uint8_t local_z = (uint8_t)(z - (chunk->z * CHUNK_SIZE));
		
		if (local_x < CHUNK_SIZE && local_y < CHUNK_SIZE && local_z < CHUNK_SIZE) {
			Block* block = &chunk->blocks[local_x][local_y][local_z];
			block_id = block ? block->id : 0;
		}
	}

	uint8_t block_type = block_data[block_id][0];
	bool is_liquid = (block_type == 3);
	const float liquid_adjustment = is_liquid ? 0.125f : 0.0f;

	for (uint8_t i = 0; i < 4; i++) {
		float pos_x = x + face_data[i].pos[0];
		float pos_y = y + face_data[i].pos[1];
		float pos_z = z + face_data[i].pos[2];
		float uv_u = face_data[i].uv[0];
		float uv_v = face_data[i].uv[1];

		if (width > 1 || height > 1) {
			switch (normal) {
				case 0: // Front (Z+)
					if (face_data[i].pos[0] == 1.0f) pos_x = x + width;
					if (face_data[i].pos[1] == 1.0f) pos_y = y + height;
					uv_u *= width_blocks;
					uv_v *= height_blocks;
					break;
				case 1: // Left (X-)
					if (face_data[i].pos[1] == 1.0f) pos_y = y + height;
					if (face_data[i].pos[2] == 1.0f) pos_z = z + width;
					uv_u *= depth_blocks;
					uv_v *= height_blocks;
					break;
				case 2: // Back (Z-)
					if (face_data[i].pos[0] == 1.0f) pos_x = x + width;
					if (face_data[i].pos[1] == 1.0f) pos_y = y + height;
					uv_u *= width_blocks;
					uv_v *= height_blocks;
					break;
				case 3: // Right (X+)
					if (face_data[i].pos[1] == 1.0f) pos_y = y + height;
					if (face_data[i].pos[2] == 1.0f) pos_z = z + width;
					uv_u *= depth_blocks;
					uv_v *= height_blocks;
					break;
				case 4: // Bottom (Y-)
					if (face_data[i].pos[0] == 1.0f) pos_x = x + width;
					if (face_data[i].pos[2] == 1.0f) pos_z = z + height;
					uv_u *= width_blocks;
					uv_v *= depth_blocks;
					break;
				case 5: // Top (Y+)
					if (face_data[i].pos[0] == 1.0f) pos_x = x + width;
					if (face_data[i].pos[2] == 1.0f) pos_z = z + height;
					uv_u *= width_blocks;
					uv_v *= depth_blocks;
					break;
			}
		}

		// Apply liquid adjustment
		if (is_liquid) {
			if (normal == 5) {
				pos_y -= liquid_adjustment;
			}
			else if (normal != 4) {
				bool is_top_vertex = (face_data[i].pos[1] == 1.0f);
				if (is_top_vertex) {
					pos_y -= liquid_adjustment;
				}
			}
		}

		vertices[(*vertex_count)++] = (Vertex) {
			(int32_t)(pos_x * 16.0f),	   // X as int32_t
			(uint16_t)(pos_y * 16.0f),	  // Y as uint16_t
			(int32_t)(pos_z * 16.0f),	   // Z as int32_t
			PACK_VERTEX_DATA(normal, texture_id),
			(uint32_t)(uv_u * 16) | ((uint32_t)(uv_v * 16) << 9)
		};
	}

	for (uint8_t i = 0; i < 6; i++) {
		indices[(*index_count)++] = base_vertex + quad_indices[i];
	}
}

void clear_face_data(Mesh faces[6]) {
	for (uint8_t face = 0; face < 6; face++) {
		if(faces[face].vertices != NULL)
			free(faces[face].vertices);

		if(faces[face].indices != NULL)
			free(faces[face].indices);

		faces[face].vertices = NULL;
		faces[face].indices = NULL;
		faces[face].vertex_count = 0;
		faces[face].index_count = 0;
	}
}

void store_face_data(Mesh* face_mesh, Vertex vertices[], uint32_t indices[], 
					uint32_t vertex_count, uint32_t index_count) {
	if (vertex_count == 0) return;
	
	face_mesh->vertices = malloc(vertex_count * sizeof(Vertex));
	face_mesh->indices = malloc(index_count * sizeof(uint32_t));
	
	if (face_mesh->vertices && face_mesh->indices) {
		memcpy(face_mesh->vertices, vertices, vertex_count * sizeof(Vertex));
		memcpy(face_mesh->indices, indices, index_count * sizeof(uint32_t));
		face_mesh->vertex_count = vertex_count;
		face_mesh->index_count = index_count;
	}
}

void generate_single_block_mesh(float x, float y, float z, uint8_t block_id, Mesh faces[6]) {
	clear_face_data(faces);

	uint8_t block_type = block_data[block_id][0];

	if (block_type == BTYPE_REGULAR || block_type == BTYPE_SLAB || block_type == BTYPE_LIQUID || block_type == BTYPE_LEAF) {
		const face_vertex_t (*face_data)[4] = (block_type == BTYPE_REGULAR || block_type == BTYPE_LIQUID || block_type == BTYPE_LEAF) ? cube_faces : slab_faces;
		
		for (uint8_t face = 0; face < 6; face++) {
			Vertex vertices[4];
			uint32_t indices[6];
			uint32_t vertex_count = 0;
			uint32_t index_count = 0;

			uint8_t texture_id = block_data[block_id][2 + face];
			add_quad(NULL, x, y, z, face, texture_id, face_data[face], 1, 1,
					vertices, indices, &vertex_count, &index_count);

			store_face_data(&faces[face], vertices, indices, vertex_count, index_count);
		}
	}
	else if (block_type == BTYPE_CROSS) {
		for (uint8_t face = 0; face < 4; face++) {
			Vertex vertices[4];
			uint32_t indices[6];
			uint32_t vertex_count = 0;
			uint32_t index_count = 0;

			uint8_t texture_id = block_data[block_id][2 + face];
			add_quad(NULL, x, y, z, face, texture_id, cross_faces[face], 1, 1,
					vertices, indices, &vertex_count, &index_count);

			store_face_data(&faces[face], vertices, indices, vertex_count, index_count);
		}
	}
}

void generate_chunk_mesh(Chunk* chunk) {
	if (chunk == NULL)
		return;

	clear_face_data(chunk->faces);
	clear_face_data(chunk->transparent_faces);

	const float world_x = chunk->x * CHUNK_SIZE;
	const float world_y = chunk->y * CHUNK_SIZE;
	const float world_z = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE] = {0};

	static Vertex face_vertices[MAX_VERTICES / 6];
	static uint32_t face_indices[MAX_VERTICES / 6];
	static Vertex transparent_face_vertices[MAX_VERTICES / 6];
	static uint32_t transparent_face_indices[MAX_VERTICES / 6];

	for (uint8_t face = 0; face < 6; face++) {
		uint32_t face_vertex_count = 0;
		uint32_t face_index_count = 0;
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
					if (block == NULL || block->id == 0) 
						continue;

					uint8_t block_type = block_data[block->id][0];

					if (block_type != BTYPE_REGULAR && block_type != BTYPE_LIQUID && block_type != BTYPE_LEAF) continue;

					if (!(block_type == BTYPE_LEAF && settings.fancy_graphics)) {
						if (!is_face_visible(chunk, x, y, z, face)) 
							continue;
					}

					uint8_t width = find_width(chunk, face, u, v, x, y, z, mask, block);
					uint8_t height = find_height(chunk, face, u, v, x, y, z, mask, block, width);

					for (uint8_t dy = 0; dy < height && (v + dy) < CHUNK_SIZE; dy++) {
						uint8_t end_u = u + width;
						if (end_u > CHUNK_SIZE) end_u = CHUNK_SIZE;
						memset(&mask[v + dy][u], true, (end_u - u) * sizeof(bool));
					}

					bool is_transparent = block_data[block->id][1] != 0;
					uint8_t texture_id = block_data[block->id][2 + face];
					if (is_transparent) {
						add_quad(chunk, x + world_x, y + world_y, z + world_z, face, texture_id, 
							cube_faces[face], width, height,
							transparent_face_vertices, transparent_face_indices,
							&transparent_face_vertex_count, &transparent_face_index_count);
					} else {
						add_quad(chunk, x + world_x, y + world_y, z + world_z, face, texture_id, 
							cube_faces[face], width, height,
							face_vertices, face_indices,
							&face_vertex_count, &face_index_count);
					}
				}
			}
		}

		store_face_data(&chunk->faces[face], face_vertices, face_indices,
						face_vertex_count, face_index_count);
		store_face_data(&chunk->transparent_faces[face], transparent_face_vertices, transparent_face_indices,
						transparent_face_vertex_count, transparent_face_index_count);
	}

	// Handle non-full blocks (slabs and crosses)
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block == NULL || block->id == 0) continue;

				uint8_t block_type = block_data[block->id][0];
				if (block_type != BTYPE_SLAB && block_type != BTYPE_CROSS) continue;

				bool is_transparent = block_data[block->id][1] != 0;
				Mesh* target_faces = is_transparent ? chunk->transparent_faces : chunk->faces;

				int face_count = (block_type == BTYPE_CROSS) ? 4 : 6;
				for (int face = 0; face < face_count; face++) {
					uint8_t texture_id = block_data[block->id][2 + face];
					uint32_t base_vertex = target_faces[face].vertex_count;
					uint32_t base_index = target_faces[face].index_count;
					target_faces[face].vertices = realloc(target_faces[face].vertices, (base_vertex + 4) * sizeof(Vertex));
					target_faces[face].indices = realloc(target_faces[face].indices, (base_index + 6) * sizeof(uint32_t));
					const face_vertex_t* face_data;
					if (block_type == BTYPE_CROSS) {
						face_data = cross_faces[face];
					} else if (block_type == BTYPE_SLAB) {
						face_data = slab_faces[face];
					} else {
						face_data = cube_faces[face];
					}
					Vertex temp_vertices[4];
					uint32_t temp_indices[6];
					uint32_t temp_vertex_count = 0;
					uint32_t temp_index_count = 0;
					add_quad(chunk, x + world_x, y + world_y, z + world_z, face, texture_id, face_data, 1, 1, temp_vertices, temp_indices, &temp_vertex_count, &temp_index_count);
					memcpy(&target_faces[face].vertices[base_vertex], temp_vertices, temp_vertex_count * sizeof(Vertex));
					for (uint8_t i = 0; i < temp_index_count; i++) {
						target_faces[face].indices[base_index + i] = temp_indices[i] + base_vertex;
					}
					target_faces[face].vertex_count += temp_vertex_count;
					target_faces[face].index_count += temp_index_count;
				}
			}
		}
	}
	chunk->needs_update = false;
}