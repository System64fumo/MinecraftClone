#include "main.h"
#include <stdlib.h>
#include <string.h>

void pre_process_chunk(Chunk* chunk) {
	Vertex vertices[MAX_VERTICES];
	uint32_t indices[MAX_VERTICES];

	uint16_t vertex_count = 0;
	uint16_t index_count = 0;

	bool mask[CHUNK_SIZE][CHUNK_SIZE];
	uint8_t x = 0, y = 0, z = 0;

	// First pass: Handle special blocks (cross, slab)
	for (x = 0; x < CHUNK_SIZE; x++) {
		for (y = 0; y < CHUNK_SIZE; y++) {
			for (z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block->id == 0) continue;

				if (block->id == 6) {
					uint16_t base_vertex = vertex_count;
					generate_cross_vertices(x, y, z, block, vertices, &vertex_count);
					
					// Generate indices for 4 quads (8 triangles)
					for (int i = 0; i < 4; i++) {
						generate_indices(base_vertex + (i * 4), indices, &index_count);
					}
				}
				else if (block->id == 44) {
					uint16_t base_vertex = vertex_count;
					generate_slab_vertices(x, y, z, block, vertices, &vertex_count);
					
					// Generate indices for 6 faces
					for (int i = 0; i < 6; i++) {
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

					map_coordinates(face, u, v, d, &x, &y, &z);

					Block* block = &chunk->blocks[x][y][z];
					if (block->id == 0 || !is_face_visible(chunk, x, y, z, face) ||
						block->id == 6 || block->id == 44) // Non full blocks
						continue;

					uint8_t width = find_width(chunk, face, u, v, x, y, z, mask, block);
					uint8_t height = find_height(chunk, face, u, v, x, y, z, mask, block, width);

					for (uint8_t dy = 0; dy < height; dy++) {
						for (uint8_t dx = 0; dx < width; dx++) {
							mask[v + dy][u + dx] = true;
						}
					}

					uint16_t base_vertex = vertex_count;
					generate_vertices(face, x, y, z, width, height, block, vertices, &vertex_count);
					generate_indices(base_vertex, indices, &index_count);
				}
			}
		}
	}

	chunk->vertex_count = vertex_count;
	chunk->index_count = index_count;

	// Delete existing buffers if they exist
	if (chunk->VBO) {
		glDeleteBuffers(1, &chunk->VBO);
		chunk->VBO = 0;
	}
	if (chunk->EBO) {
		glDeleteBuffers(1, &chunk->EBO);
		chunk->EBO = 0;
	}
	if (chunk->VAO) {
		glDeleteVertexArrays(1, &chunk->VAO);
		chunk->VAO = 0;
	}

	// Only create new buffers if there are vertices to process
	if (vertex_count > 0) {
		glGenVertexArrays(1, &chunk->VAO);
		glGenBuffers(1, &chunk->VBO);
		glGenBuffers(1, &chunk->EBO);

		glBindVertexArray(chunk->VAO);

		glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint32_t), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
		glEnableVertexAttribArray(1);

		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, width));
		glEnableVertexAttribArray(3);

		glBindVertexArray(0);
	}
	chunk->needs_update = false;
}

void render_chunks() {
	GLint modelUniformLocation = glGetUniformLocation(shaderProgram, "model");
	static bool baking = false;
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];

				if (chunk->needs_update) {
					#ifdef DEBUG
					if (!baking) {
						profiler_start(PROFILER_ID_BAKE);
						baking = true;
					}
					#endif
					pre_process_chunk(chunk);
					chunk->needs_update = false;
				}
			}
		}
	}
	#ifdef DEBUG
	if (baking) {
		profiler_stop(PROFILER_ID_BAKE);
		baking = false;
	}
	#endif

	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER);
	#endif
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->vertex_count == 0)
					continue;

				matrix4_identity(model);
				matrix4_translate(model, 
					chunk->x * CHUNK_SIZE, 
					chunk->y * CHUNK_SIZE, 
					chunk->z * CHUNK_SIZE
				);
				glBindVertexArray(chunk->VAO);
				glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model);
				glDrawElements(GL_TRIANGLES, chunk->index_count, GL_UNSIGNED_INT, 0);
			}
		}
	}
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER);
	#endif
}