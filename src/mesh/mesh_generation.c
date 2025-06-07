#include "main.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	float pos[3];
	float uv[2];
} face_vertex_t;

static const face_vertex_t cube_faces[6][4] = {
	{{{1,1,1}, {1,0}}, {{0,1,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,1}, {1,1}}},	// Front (Z+)
	{{{1,1,0}, {1,0}}, {{1,1,1}, {0,0}}, {{1,0,1}, {0,1}}, {{1,0,0}, {1,1}}},	// Left (X-)
	{{{0,1,0}, {1,0}}, {{1,1,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,0}, {1,1}}},	// Back (Z-)
	{{{0,1,1}, {1,0}}, {{0,1,0}, {0,0}}, {{0,0,0}, {0,1}}, {{0,0,1}, {1,1}}},	// Right (X+)
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},	// Bottom (Y-)
	{{{0,1,1}, {0,0}}, {{1,1,1}, {1,0}}, {{1,1,0}, {1,1}}, {{0,1,0}, {0,1}}}	// Top (Y+)
};

// Slab faces (half-height)
static const face_vertex_t slab_faces[6][4] = {
	{{{1,0.5,1}, {1,0}}, {{0,0.5,1}, {0,0}}, {{0,0,1}, {0,0.5}}, {{1,0,1}, {1,0.5}}},	// Front
	{{{0,0.5,0}, {1,0}}, {{0,0.5,1}, {0,0}}, {{0,0,1}, {0,0.5}}, {{0,0,0}, {1,0.5}}},	// Left
	{{{0,0.5,0}, {1,0}}, {{1,0.5,0}, {0,0}}, {{1,0,0}, {0,0.5}}, {{0,0,0}, {1,0.5}}},	// Back
	{{{1,0.5,1}, {1,0}}, {{1,0.5,0}, {0,0}}, {{1,0,0}, {0,0.5}}, {{1,0,1}, {1,0.5}}},	// Right
	{{{0,0,0}, {0,1}}, {{1,0,0}, {1,1}}, {{1,0,1}, {1,0}}, {{0,0,1}, {0,0}}},			// Bottom
	{{{0,0.5,1}, {0,0}}, {{1,0.5,1}, {1,0}}, {{1,0.5,0}, {1,1}}, {{0,0.5,0}, {0,1}}}	// Top
};

// Cross faces (4 diagonal faces)
static const face_vertex_t cross_faces[4][4] = {
	{{{1,1,1}, {1,0}}, {{0,1,0}, {0,0}}, {{0,0,0}, {0,1}}, {{1,0,1}, {1,1}}},
	{{{0,1,1}, {1,0}}, {{1,1,0}, {0,0}}, {{1,0,0}, {0,1}}, {{0,0,1}, {1,1}}},
	{{{0,1,0}, {1,0}}, {{1,1,1}, {0,0}}, {{1,0,1}, {0,1}}, {{0,0,0}, {1,1}}},
	{{{1,1,0}, {1,0}}, {{0,1,1}, {0,0}}, {{0,0,1}, {0,1}}, {{1,0,0}, {1,1}}}
};

static const uint32_t quad_indices[6] = {0, 1, 2, 0, 2, 3};

void add_quad(float x, float y, float z, uint8_t face_id, uint8_t texture_id,
			 const face_vertex_t face_data[4], Vertex vertices[], uint32_t indices[],
			 uint32_t* vertex_count, uint32_t* index_count) {
	
	uint32_t base_vertex = *vertex_count;

	for (uint8_t i = 0; i < 4; i++) {
		vertices[(*vertex_count)++] = (Vertex){
			x + face_data[i].pos[0],
			y + face_data[i].pos[1],
			z + face_data[i].pos[2],
			face_id,
			texture_id,
			face_data[i].uv[0],
			face_data[i].uv[1]
		};
	}

	for (uint8_t i = 0; i < 6; i++) {
		indices[(*index_count)++] = base_vertex + quad_indices[i];
	}
}

void generate_block_mesh(float x, float y, float z, Block* block, uint8_t block_type,
						Vertex vertices[], uint32_t indices[], uint32_t* vertex_count, uint32_t* index_count) {
	
	switch (block_type) {
		case 0: // Regular cube
			for (int face = 0; face < 6; face++) {
				uint8_t texture_id = block_data[block->id][2 + face];
				add_quad(x, y, z, face, texture_id, cube_faces[face], 
						vertices, indices, vertex_count, index_count);
			}
			break;
			
		case 1: // Slab
			for (int face = 0; face < 6; face++) {
				uint8_t texture_id = block_data[block->id][2 + face];
				add_quad(x, y, z, face, texture_id, slab_faces[face], 
						vertices, indices, vertex_count, index_count);
			}
			break;
			
		case 2: // Cross
			for (int face = 0; face < 4; face++) {
				uint8_t texture_id = block_data[block->id][2 + face];
				add_quad(x, y, z, face, texture_id, cross_faces[face], 
						vertices, indices, vertex_count, index_count);
			}
			break;
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

void generate_chunk_mesh(Chunk* chunk) {
	if (chunk == NULL) return;
	
	// Clear existing data
	clear_face_data(chunk->faces);
	clear_face_data(chunk->transparent_faces);
	
	const float world_x = chunk->x * CHUNK_SIZE;
	const float world_y = chunk->y * CHUNK_SIZE;
	const float world_z = chunk->z * CHUNK_SIZE;

	Vertex opaque_vertices[MAX_VERTICES] = {0};
	uint32_t opaque_indices[MAX_VERTICES] = {0};
	uint32_t opaque_vertex_count = 0;
	uint32_t opaque_index_count = 0;
	
	Vertex transparent_vertices[MAX_VERTICES] = {0};
	uint32_t transparent_indices[MAX_VERTICES] = {0};
	uint32_t transparent_vertex_count = 0;
	uint32_t transparent_index_count = 0;
	
	// Generate mesh for all blocks
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block == NULL || block->id == 0) continue;
				
				uint8_t block_type = block_data[block->id][0];
				bool is_transparent = block_data[block->id][1] != 0;
				
				// For regular blocks, check face visibility (greedy meshing can be added later)
				if (block_type == 0) {
					for (uint8_t face = 0; face < 6; face++) {
						if (!is_face_visible(chunk, x, y, z, face)) continue;
						
						uint8_t texture_id = block_data[block->id][2 + face];
						
						if (is_transparent) {
							add_quad(x + world_x, y + world_y, z + world_z, face, texture_id,
									cube_faces[face], transparent_vertices, transparent_indices,
									&transparent_vertex_count, &transparent_index_count);
						} else {
							add_quad(x + world_x, y + world_y, z + world_z, face, texture_id,
									cube_faces[face], opaque_vertices, opaque_indices,
									&opaque_vertex_count, &opaque_index_count);
						}
					}
				} else {
					// Special blocks (slab, cross) - always generate all faces
					if (is_transparent) {
						generate_block_mesh(x + world_x, y + world_y, z + world_z, block, block_type,
											transparent_vertices, transparent_indices,
											&transparent_vertex_count, &transparent_index_count);
					} else {
						generate_block_mesh(x + world_x, y + world_y, z + world_z, block, block_type,
											opaque_vertices, opaque_indices,
											&opaque_vertex_count, &opaque_index_count);
					}
				}
			}
		}
	}
	
	// Distribute vertices to faces based on face_id
	for (uint8_t face = 0; face < 6; face++) {
		Vertex face_vertices[MAX_VERTICES / 6] = {0};
		uint32_t face_indices[MAX_VERTICES / 6] = {0};
		uint32_t face_vertex_count = 0;
		uint32_t face_index_count = 0;
		
		// Extract vertices for this face from opaque geometry
		for (uint32_t i = 0; i < opaque_vertex_count; i++) {
			if (opaque_vertices[i].face_id == face) {
				face_vertices[face_vertex_count++] = opaque_vertices[i];
			}
		}
		
		// Generate indices (assuming quads)
		for (uint32_t i = 0; i < face_vertex_count; i += 4) {
			for (int j = 0; j < 6; j++) {
				face_indices[face_index_count++] = i + quad_indices[j];
			}
		}
		
		store_face_data(&chunk->faces[face], face_vertices, face_indices, 
						face_vertex_count, face_index_count);
		
		// Same for transparent faces
		face_vertex_count = 0;
		face_index_count = 0;
		
		for (uint32_t i = 0; i < transparent_vertex_count; i++) {
			if (transparent_vertices[i].face_id == face) {
				face_vertices[face_vertex_count++] = transparent_vertices[i];
			}
		}
		
		for (uint32_t i = 0; i < face_vertex_count; i += 4) {
			for (uint8_t j = 0; j < 6; j++) {
				face_indices[face_index_count++] = i + quad_indices[j];
			}
		}
		
		store_face_data(&chunk->transparent_faces[face], face_vertices, face_indices, 
						face_vertex_count, face_index_count);
	}
	
	chunk->needs_update = false;
}