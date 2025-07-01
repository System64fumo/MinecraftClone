#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Single VAO/VBO per mesh type
unsigned int opaque_VAO = 0;
unsigned int opaque_VBO = 0;
unsigned int opaque_EBO = 0;
uint32_t opaque_index_count = 0;

unsigned int transparent_VAO = 0;
unsigned int transparent_VBO = 0;
unsigned int transparent_EBO = 0;
uint32_t transparent_index_count = 0;

bool mesh_mode = false;
uint16_t draw_calls = 0;
bool mesh_needs_rebuild = false;
uint8_t ***visibility_map = NULL;

void init_gl_buffers() {
	glGenVertexArrays(1, &opaque_VAO);
	glGenBuffers(1, &opaque_VBO);
	glGenBuffers(1, &opaque_EBO);

	glGenVertexArrays(1, &transparent_VAO);
	glGenBuffers(1, &transparent_VBO);
	glGenBuffers(1, &transparent_EBO);

	glBindVertexArray(opaque_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, opaque_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBO);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
	glEnableVertexAttribArray(3);

	glBindVertexArray(transparent_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, transparent_VBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBO);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);

	// Allocate visibility map
	if (visibility_map) {
		for (int x = 0; x < settings.render_distance; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				free(visibility_map[x][y]);
			}
			free(visibility_map[x]);
		}
		free(visibility_map);
	}
	visibility_map = malloc(settings.render_distance * sizeof(uint8_t**));
	for (int x = 0; x < settings.render_distance; x++) {
		visibility_map[x] = malloc(WORLD_HEIGHT * sizeof(uint8_t*));
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			visibility_map[x][y] = malloc(settings.render_distance * sizeof(uint8_t));
			memset(visibility_map[x][y], 0, settings.render_distance * sizeof(uint8_t));
		}
	}
}

void rebuild_combined_visible_mesh() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE, false);
	#endif
	
	opaque_index_count = 0;
	transparent_index_count = 0;

	glBindVertexArray(opaque_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, opaque_VBO);
	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

	glBindVertexArray(transparent_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, transparent_VBO);
	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

	// First pass: count vertices and indices for visible faces
	uint32_t total_opaque_vertices = 0;
	uint32_t total_opaque_indices = 0;
	uint32_t total_transparent_vertices = 0;
	uint32_t total_transparent_indices = 0;

	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t x = 0; x < settings.render_distance; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < settings.render_distance; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (!(visibility_map[x][y][z] & (1 << face)) || !chunk->is_loaded) continue;
					
					// Only count faces that are visible
					total_opaque_vertices += chunk->faces[face].vertex_count;
					total_opaque_indices += chunk->faces[face].index_count;
					total_transparent_vertices += chunk->transparent_faces[face].vertex_count;
					total_transparent_indices += chunk->transparent_faces[face].index_count;
				}
			}
		}
	}

	if (total_opaque_vertices == 0 && total_transparent_vertices == 0) {
		mesh_needs_rebuild = false;
		return;
	}

	// Second pass: build opaque mesh
	if (total_opaque_vertices > 0) {
		opaque_index_count = total_opaque_indices;
		glBindVertexArray(opaque_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, opaque_VBO);
		glBufferData(GL_ARRAY_BUFFER, total_opaque_vertices * sizeof(Vertex), NULL, GL_STATIC_DRAW);
		Vertex* vbo_ptr = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_opaque_indices * sizeof(uint32_t), NULL, GL_STATIC_DRAW);
		uint32_t* ebo_ptr = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		if (vbo_ptr && ebo_ptr) {
			uint32_t vertex_offset = 0;
			uint32_t index_offset = 0;
			uint32_t base_vertex = 0;

			for (uint8_t face = 0; face < 6; face++) {
				for (uint8_t x = 0; x < settings.render_distance; x++) {
					for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
						for (uint8_t z = 0; z < settings.render_distance; z++) {
							Chunk* chunk = &chunks[x][y][z];
							if (!(visibility_map[x][y][z] & (1 << face)) || !chunk->is_loaded) continue;
							if (chunk->faces[face].vertex_count == 0) continue;

							// Copy vertices
							memcpy(vbo_ptr + vertex_offset, chunk->faces[face].vertices,
									chunk->faces[face].vertex_count * sizeof(Vertex));

							// Copy indices with offset
							for (uint32_t i = 0; i < chunk->faces[face].index_count; i++) {
								ebo_ptr[index_offset + i] = chunk->faces[face].indices[i] + base_vertex;
							}

							vertex_offset += chunk->faces[face].vertex_count;
							index_offset += chunk->faces[face].index_count;
							base_vertex += chunk->faces[face].vertex_count;
						}
					}
				}
			}
		}

		if (vbo_ptr) glUnmapBuffer(GL_ARRAY_BUFFER);
		if (ebo_ptr) glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}

	// Third pass: build transparent mesh
	if (total_transparent_vertices > 0) {
		transparent_index_count = total_transparent_indices;
		glBindVertexArray(transparent_VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBO);
		glBufferData(GL_ARRAY_BUFFER, total_transparent_vertices * sizeof(Vertex), NULL, GL_STATIC_DRAW);
		Vertex* vbo_ptr = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_transparent_indices * sizeof(uint32_t), NULL, GL_STATIC_DRAW);
		uint32_t* ebo_ptr = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

		if (vbo_ptr && ebo_ptr) {
			uint32_t vertex_offset = 0;
			uint32_t index_offset = 0;
			uint32_t base_vertex = 0;

			for (uint8_t face = 0; face < 6; face++) {
				for (uint8_t x = 0; x < settings.render_distance; x++) {
					for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
						for (uint8_t z = 0; z < settings.render_distance; z++) {
							Chunk* chunk = &chunks[x][y][z];
							if (!(visibility_map[x][y][z] & (1 << face)) || !chunk->is_loaded) continue;
							if (chunk->transparent_faces[face].vertex_count == 0) continue;

							// Copy vertices
							memcpy(vbo_ptr + vertex_offset, chunk->transparent_faces[face].vertices,
									chunk->transparent_faces[face].vertex_count * sizeof(Vertex));

							// Copy indices with offset
							for (uint32_t i = 0; i < chunk->transparent_faces[face].index_count; i++) {
								ebo_ptr[index_offset + i] = chunk->transparent_faces[face].indices[i] + base_vertex;
							}

							vertex_offset += chunk->transparent_faces[face].vertex_count;
							index_offset += chunk->transparent_faces[face].index_count;
							base_vertex += chunk->transparent_faces[face].vertex_count;
						}
					}
				}
			}
		}

		if (vbo_ptr) glUnmapBuffer(GL_ARRAY_BUFFER);
		if (ebo_ptr) glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	}

	glBindVertexArray(0);
	mesh_needs_rebuild = false;
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MERGE, false);
	#endif
}

void render_chunks() {
	if (mesh_needs_rebuild)
		rebuild_combined_visible_mesh();

	if (mesh_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	if (opaque_index_count > 0) {
		glBindVertexArray(opaque_VAO);
		glDrawElements(GL_TRIANGLES, opaque_index_count, GL_UNSIGNED_INT, 0);
		draw_calls++;
	}

	if (transparent_index_count > 0) {
		glBindVertexArray(transparent_VAO);
		glDrawElements(GL_TRIANGLES, transparent_index_count, GL_UNSIGNED_INT, 0);
		draw_calls++;
	}

	if (mesh_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void cleanup_renderer() {
	glDeleteVertexArrays(1, &opaque_VAO);
	glDeleteBuffers(1, &opaque_VBO);
	glDeleteBuffers(1, &opaque_EBO);

	glDeleteVertexArrays(1, &transparent_VAO);
	glDeleteBuffers(1, &transparent_VBO);
	glDeleteBuffers(1, &transparent_EBO);
}