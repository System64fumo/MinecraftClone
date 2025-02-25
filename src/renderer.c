#include "main.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	Vertex* vertices;
	uint32_t* indices;
	uint32_t vertex_count;
	uint32_t index_count;
} CombinedMesh;

static CombinedMesh combined_mesh = {0};
static unsigned int combined_VAO = 0;
static unsigned int combined_VBO = 0;
static unsigned int combined_EBO = 0;

void pre_process_chunk(Chunk* chunk) {
	Vertex vertices[MAX_VERTICES];
	uint32_t indices[MAX_VERTICES];
	uint32_t vertex_count = 0;
	uint32_t index_count = 0;

	float world_x = chunk->x * CHUNK_SIZE;
	float world_y = chunk->y * CHUNK_SIZE;
	float world_z = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE];

	// First pass: Handle special blocks (cross, slab)
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block_data[block->id][0] == 0) continue;

				if (block_data[block->id][0] == 2) {
					uint32_t base_vertex = vertex_count;
					generate_cross_vertices(x + world_x, y + world_y, z + world_z, block, vertices, &vertex_count);
					for (uint8_t i = 0; i < 4; i++) {
						generate_indices(base_vertex + (i * 4), indices, &index_count);
					}
				}
				else if (block_data[block->id][0] == 1) {
					uint32_t base_vertex = vertex_count;
					generate_slab_vertices(x + world_x, y + world_y, z + world_z, block, vertices, &vertex_count);
					for (uint8_t i = 0; i < 6; i++) {
						generate_indices(base_vertex + (i * 4), indices, &index_count);
					}
				}
			}
		}
	}

	// Second pass: Handle regular blocks using greedy meshing
	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			for (uint8_t v = 0; v < CHUNK_SIZE; v++) {
				for (uint8_t u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					uint8_t x, y, z;
					map_coordinates(face, u, v, d, &x, &y, &z);

					Block* block = &chunk->blocks[x][y][z];
					if (block->id == 0 || block_data[block->id][0] != 0 || !is_face_visible(chunk, x, y, z, face)) continue;

					uint8_t width = find_width(chunk, face, u, v, x, y, z, mask, block);
					uint8_t height = find_height(chunk, face, u, v, x, y, z, mask, block, width);

					for (uint8_t dy = 0; dy < height; dy++) {
						for (uint8_t dx = 0; dx < width; dx++) {
							mask[v + dy][u + dx] = true;
						}
					}

					uint16_t base_vertex = vertex_count;
					generate_vertices(face, x + world_x, y + world_y, z + world_z, width, height, block, vertices, &vertex_count);
					generate_indices(base_vertex, indices, &index_count);
				}
			}
		}
	}

	chunk->vertex_count = vertex_count;
	chunk->index_count = index_count;

	// Store the mesh data for later combination
	size_t vertex_size = vertex_count * sizeof(Vertex);
	size_t index_size = index_count * sizeof(uint32_t);
	
	// Free previous memory if it exists
	if (chunk->vertices != NULL) {
		free(chunk->vertices);
		chunk->vertices = NULL; // Set to NULL after freeing
	}
	if (chunk->indices != NULL) {
		free(chunk->indices);
		chunk->indices = NULL; // Set to NULL after freeing
	}
	
	// Only allocate if we have vertices to store
	if (vertex_count > 0) {
		chunk->vertices = malloc(vertex_size);
		chunk->indices = malloc(index_size);
		
		if (!chunk->vertices || !chunk->indices) {
			fprintf(stderr, "Failed to allocate memory for chunk mesh\n");
			exit(EXIT_FAILURE);
		}
		
		memcpy(chunk->vertices, vertices, vertex_size);
		memcpy(chunk->indices, indices, index_size);
	}
	else {
		chunk->vertices = NULL;
		chunk->indices = NULL;
	}

	chunk->needs_update = false;
}

void combine_meshes() {
	uint32_t total_vertices = 0;
	uint32_t total_indices = 0;

	// Calculate total sizes
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				total_vertices += chunk->vertex_count;
				total_indices += chunk->index_count;
			}
		}
	}

	// Skip if there's nothing to render
	if (total_vertices == 0 || total_indices == 0) {
		combined_mesh.vertex_count = 0;
		combined_mesh.index_count = 0;
		return;
	}
	// Allocate combined buffers
	combined_mesh.vertices = malloc(total_vertices * sizeof(Vertex));
	combined_mesh.indices = malloc(total_indices * sizeof(uint32_t));

	// Check if malloc succeeded
	if (combined_mesh.vertices == NULL || combined_mesh.indices == NULL) {
		fprintf(stderr, "Failed to allocate memory for combined mesh\n");
		exit(EXIT_FAILURE);
	}

	// Combine all meshes
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	uint32_t base_vertex = 0;

	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->vertex_count == 0 || chunk->vertices == NULL) {
					continue;
				}

				// Copy vertices
				memcpy(combined_mesh.vertices + vertex_offset, chunk->vertices,
					   chunk->vertex_count * sizeof(Vertex));

				// Copy and adjust indices
				for (uint32_t i = 0; i < chunk->index_count; i++) {
					combined_mesh.indices[index_offset + i] = chunk->indices[i] + base_vertex;
				}

				vertex_offset += chunk->vertex_count;
				index_offset += chunk->index_count;
				base_vertex += chunk->vertex_count;
			}
		}
	}

	combined_mesh.vertex_count = vertex_offset;
	combined_mesh.index_count = index_offset;

	// Create combined VAO/VBO/EBO
	if (combined_VAO == 0) {
		glGenVertexArrays(1, &combined_VAO);
		glGenBuffers(1, &combined_VBO);
		glGenBuffers(1, &combined_EBO);
	}

	glBindVertexArray(combined_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
	glBufferData(GL_ARRAY_BUFFER, combined_mesh.vertex_count * sizeof(Vertex), combined_mesh.vertices, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, combined_mesh.index_count * sizeof(uint32_t), combined_mesh.indices, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
	glEnableVertexAttribArray(1);

	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
	glEnableVertexAttribArray(2);

	glVertexAttribPointer(3, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, width));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	free(combined_mesh.vertices);
	combined_mesh.vertices = NULL;

	free(combined_mesh.indices);
	combined_mesh.indices = NULL;
}

void process_chunks() {
	static bool baking = false;
	bool chunks_updated = false;

	#ifdef DEBUG
	if (!baking) {
		profiler_start(PROFILER_ID_BAKE);
		baking = true;
	}
	#endif

	// Process all chunks that need updating
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->needs_update) {
					pre_process_chunk(chunk);
					chunks_updated = true;
				}
			}
		}
	}

	if (chunks_updated) {
		combine_meshes();
	}

	#ifdef DEBUG
	if (baking) {
		profiler_stop(PROFILER_ID_BAKE);
		baking = false;
	}
	#endif
}

void render_chunks() {
	// Skip rendering if there are no vertices
	if (combined_mesh.vertex_count == 0 || combined_mesh.index_count == 0) {
		return;
	}

	GLint modelUniformLocation = glGetUniformLocation(shaderProgram, "model");
	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER);
	#endif

	glUseProgram(shaderProgram);
	matrix4_identity(model);
	glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model);

	glBindVertexArray(combined_VAO);
	glDrawElements(GL_TRIANGLES, combined_mesh.index_count, GL_UNSIGNED_INT, 0);

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER);
	#endif
}