#include "main.h"
#include <stdlib.h>
#include <string.h>

static const face_vertex_t cube_faces[6][4] = {
	{{{1,1,1}, {1,0}}, {{0,1,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,1}, {1,1}}},	// Front (Z+)
	{{{1,1,0}, {1,0}}, {{1,1,1}, {0,0}}, {{1,0,1}, {0,1}}, {{1,0,0}, {1,1}}},	// Left (X-)
	{{{0,1,0}, {1,0}}, {{1,1,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,0}, {1,1}}},	// Back (Z-)
	{{{0,1,1}, {1,0}}, {{0,1,0}, {0,0}}, {{0,0,0}, {0,1}}, {{0,0,1}, {1,1}}},	// Right (X+)
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},	// Bottom (Y-)
	{{{0,1,1}, {0,0}}, {{1,1,1}, {1,0}}, {{1,1,0}, {1,1}}, {{0,1,0}, {0,1}}}	// Top (Y+)
};

static const face_vertex_t slab_faces[6][4] = {
	{{{1,0.5,1}, {1,0}}, {{0,0.5,1}, {0,0}}, {{0,0,1}, {0,0.5}}, {{1,0,1}, {1,0.5}}},	// Front
	{{{1,0.5,0}, {1,0}}, {{1,0.5,1}, {0,0}}, {{1,0,1}, {0,0.5}}, {{1,0,0}, {1,0.5}}},	// Left
	{{{0,0.5,0}, {1,0}}, {{1,0.5,0}, {0,0}}, {{1,0,0}, {0,0.5}}, {{0,0,0}, {1,0.5}}},	// Back
	{{{0,0.5,1}, {1,0}}, {{0,0.5,0}, {0,0}}, {{0,0,0}, {0,0.5}}, {{0,0,1}, {1,0.5}}},	// Right
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},			// Bottom
	{{{0,0.5,1}, {0,0}}, {{1,0.5,1}, {1,0}}, {{1,0.5,0}, {1,1}}, {{0,0.5,0}, {0,1}}}	// Top
};

static const face_vertex_t cross_faces[4][4] = {
	{{{1,1,1}, {1,0}}, {{0,1,0}, {0,0}}, {{0,0,0}, {0,1}}, {{1,0,1}, {1,1}}},
	{{{0,1,1}, {1,0}}, {{1,1,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,1}, {1,1}}},
	{{{0,1,0}, {1,0}}, {{1,1,1}, {0,0}}, {{1,0,1}, {0,1}}, {{0,0,0}, {1,1}}},
	{{{1,1,0}, {1,0}}, {{0,1,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,0}, {1,1}}}
};

static const face_vertex_t liquid_faces[6][4] = {
	{{{1,0.875,1}, {1,0}}, {{0,0.875,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,1}, {1,1}}},			// Front (Z+)
	{{{1,0.875,0}, {1,0}}, {{1,0.875,1}, {0,0}}, {{1,0,1}, {0,1}}, {{1,0,0}, {1,1}}},			// Left (X-)
	{{{0,0.875,0}, {1,0}}, {{1,0.875,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,0}, {1,1}}},			// Back (Z-)
	{{{0,0.875,1}, {1,0}}, {{0,0.875,0}, {0,0}}, {{0,0,0}, {0,1}}, {{0,0,1}, {1,1}}},			// Right (X+)
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},					// Bottom (Y-)
	{{{0,0.875,1}, {0,0}}, {{1,0.875,1}, {1,0}}, {{1,0.875,0}, {1,1}}, {{0,0.875,0}, {0,1}}}	// Top (Y+)
};

static const uint32_t quad_indices[6] = {0, 1, 2, 0, 2, 3};

uint8_t get_light_level_at(Chunk* chunk, int x, int y, int z) {
	if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
		return 0;
	}
	
	Block* block = &chunk->blocks[x][y][z];
	if (block == NULL) return 0;
	
	return block->light_level;
}

uint32_t pack_light_values(uint8_t values[8]) {
	uint32_t packed = 0;
	for (int i = 0; i < 8; i++) {
		uint8_t clamped = values[i] & 0xF;
		packed |= ((uint32_t)clamped) << (i * 4);
	}
	return packed;
}

void sample_face_lighting(Chunk* chunk, uint8_t face, uint8_t start_x, uint8_t start_y, uint8_t start_z,
						 uint8_t width, uint8_t height, uvec4 light_data[8]) {

	for (int i = 0; i < 8; i++) {
		light_data[i].x = 0;
		light_data[i].y = 0;
		light_data[i].z = 0;
		light_data[i].w = 0;
	}

	int face_offsets[6][3] = {
		{0, 0, 1},	// Front (Z+)
		{1, 0, 0},	// Left (X-)
		{0, 0, -1},	// Back (Z-)
		{-1, 0, 0},	// Right (X+)
		{0, -1, 0},	// Bottom (Y-)
		{0, 1, 0}	// Top (Y+)
	};
	
	int dx = face_offsets[face][0];
	int dy = face_offsets[face][1];
	int dz = face_offsets[face][2];

	uint8_t light_values[256];
	
	for (int v = 0; v < 16; v++) {
		for (int u = 0; u < 16; u++) {
			int sample_x, sample_y, sample_z;

			switch (face) {
				case 0: // Front (Z+)
					sample_x = start_x + (u * width) / 16;
					sample_y = start_y + (v * height) / 16;
					sample_z = start_z;
					break;
				case 1: // Left (X-)
					sample_x = start_x;
					sample_y = start_y + (v * height) / 16;
					sample_z = start_z + (u * width) / 16;
					break;
				case 2: // Back (Z-)
					sample_x = start_x + ((15 - u) * width) / 16;
					sample_y = start_y + (v * height) / 16;
					sample_z = start_z;
					break;
				case 3: // Right (X+)
					sample_x = start_x;
					sample_y = start_y + (v * height) / 16;
					sample_z = start_z + ((15 - u) * width) / 16;
					break;
				case 4: // Bottom (Y-)
					sample_x = start_x + (u * width) / 16;
					sample_y = start_y;
					sample_z = start_z + (v * height) / 16;
					break;
				case 5: // Top (Y+)
					sample_x = start_x + (u * width) / 16;
					sample_y = start_y;
					sample_z = start_z + ((15 - v) * height) / 16;
					break;
				default:
					sample_x = start_x;
					sample_y = start_y;
					sample_z = start_z;
					break;
			}

			uint8_t light_level = get_light_level_at(chunk, sample_x + dx, sample_y + dy, sample_z + dz);
			light_values[v * 16 + u] = light_level;
		}
	}

	uint32_t packed_values[32];
	for (int i = 0; i < 32; i++) {
		uint8_t values[8];
		for (int j = 0; j < 8; j++) {
			int index = i * 8 + j;
			if (index < 256) {
				values[j] = light_values[index];
			} else {
				values[j] = 0;
			}
		}
		packed_values[i] = pack_light_values(values);
	}

	for (int i = 0; i < 8; i++) {
		light_data[i].x = packed_values[i * 4 + 0];
		light_data[i].y = packed_values[i * 4 + 1];
		light_data[i].z = packed_values[i * 4 + 2];
		light_data[i].w = packed_values[i * 4 + 3];
	}
}

void add_quad(Chunk* chunk, float x, float y, float z, uint8_t face_id, uint8_t texture_id,
							const face_vertex_t face_data[4], uint8_t width, uint8_t height,
							Vertex vertices[], uint32_t indices[],
							uint32_t* vertex_count, uint32_t* index_count) {
	
	uint32_t base_vertex = *vertex_count;

	float width_blocks = (face_id == 1 || face_id == 3) ? 1.0f : (float)width;
	float height_blocks = (face_id >= 4) ? 1.0f : (float)height;
	float depth_blocks = (face_id == 0 || face_id == 2) ? 1.0f : (face_id >= 4 ? (float)height : (float)width);

	// uvec4 vertex_lights[8];
	// if (chunk) {
	// 	uint8_t local_x = (uint8_t)(x - (chunk->x * CHUNK_SIZE));
	// 	uint8_t local_y = (uint8_t)(y - (chunk->y * CHUNK_SIZE));
	// 	uint8_t local_z = (uint8_t)(z - (chunk->z * CHUNK_SIZE));
	// 	sample_face_lighting(chunk, face_id, local_x, local_y, local_z, width, height, vertex_lights);
	// }

	for (uint8_t i = 0; i < 4; i++) {
		float pos_x = x + face_data[i].pos[0];
		float pos_y = y + face_data[i].pos[1];
		float pos_z = z + face_data[i].pos[2];
		float uv_u = face_data[i].uv[0];
		float uv_v = face_data[i].uv[1];

		if (width > 1 || height > 1) {
			switch (face_id) {
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

		vertices[(*vertex_count)++] = (Vertex){
			pos_x, pos_y, pos_z,
			face_id,
			texture_id,
			uv_u,
			uv_v,
			// vertex_lights[0],
			// vertex_lights[1],
			// vertex_lights[2],
			// vertex_lights[3],
			// vertex_lights[4],
			// vertex_lights[5],
			// vertex_lights[6],
			// vertex_lights[7],
		};
	}

	for (uint8_t i = 0; i < 6; i++) {
		indices[(*index_count)++] = base_vertex + quad_indices[i];
	}
}

void clear_face_data(FaceMesh faces[6]) {
	for (int face = 0; face < 6; face++) {
		free(faces[face].vertices);
		free(faces[face].indices);
		faces[face].vertices = NULL;
		faces[face].indices = NULL;
		faces[face].vertex_count = 0;
		faces[face].index_count = 0;
	}
}

void store_face_data(FaceMesh* face_mesh, Vertex vertices[], uint32_t indices[], 
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

void generate_single_block_mesh(float x, float y, float z, uint8_t block_id, FaceMesh faces[6]) {
	clear_face_data(faces);

	uint8_t block_type = block_data[block_id][0];
	bool is_liquid = (block_id >= 8 && block_id <= 11);

	if (block_type == 0 || block_type == 1) {
		const face_vertex_t (*face_data)[4] = (block_type == 0) ? 
			(is_liquid ? liquid_faces : cube_faces) : slab_faces;
		
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
	else if (block_type == 2) {
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

// TODO: Fix water greedy mesh
void generate_chunk_mesh(Chunk* chunk) {
	if (chunk == NULL)
		return;

	clear_face_data(chunk->faces);
	clear_face_data(chunk->transparent_faces);

	const float world_x = chunk->x * CHUNK_SIZE;
	const float world_y = chunk->y * CHUNK_SIZE;
	const float world_z = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE] = {0};

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
					uint8_t texture_id = block_data[block->id][2 + face];

					const face_vertex_t (*face_data)[4] = (block->id >= 8 && block->id <= 11) ? liquid_faces : cube_faces;
					
					if (is_transparent) {
						add_quad(chunk, x + world_x, y + world_y, z + world_z, face, texture_id, 
								face_data[face], width, height,
								transparent_face_vertices, transparent_face_indices,
								&transparent_face_vertex_count, &transparent_face_index_count);
					} else {
						add_quad(chunk, x + world_x, y + world_y, z + world_z, face, texture_id, 
								face_data[face], width, height,
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

	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block == NULL || block->id == 0) continue;

				uint8_t block_type = block_data[block->id][0];
				if (block_type == 0) continue; // Skip full blocks (already handled above)

				bool is_transparent = block_data[block->id][1] != 0;
				FaceMesh* target_faces = is_transparent ? chunk->transparent_faces : chunk->faces;

				int face_count = (block_type == 2) ? 4 : 6;
				for (int face = 0; face < face_count; face++) {
					uint8_t texture_id = block_data[block->id][2 + face];

					uint32_t base_vertex = target_faces[face].vertex_count;
					uint32_t base_index = target_faces[face].index_count;

					target_faces[face].vertices = realloc(target_faces[face].vertices, 
						(base_vertex + 4) * sizeof(Vertex));
					target_faces[face].indices = realloc(target_faces[face].indices, 
						(base_index + 6) * sizeof(uint32_t));

					// Use liquid_faces for water blocks if they're non-cube type (though water should be type 0)
					const face_vertex_t* face_data;
					if (block_type == 2) {
						face_data = cross_faces[face];
					} else if (block_type == 1) {
						face_data = slab_faces[face];
					} else {
						face_data = (block->id >= 8 && block->id <= 11) ? liquid_faces[face] : cube_faces[face];
					}
					
					Vertex temp_vertices[4];
					uint32_t temp_indices[6];
					uint32_t temp_vertex_count = 0;
					uint32_t temp_index_count = 0;
					
					add_quad(chunk, x + world_x, y + world_y, z + world_z, face, texture_id, 
							face_data, 1, 1, temp_vertices, temp_indices, 
							&temp_vertex_count, &temp_index_count);

					memcpy(&target_faces[face].vertices[base_vertex], temp_vertices, 
							temp_vertex_count * sizeof(Vertex));
					
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