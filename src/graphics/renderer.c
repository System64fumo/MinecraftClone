#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Face-based VAOs/VBOs
unsigned int opaque_VAOs[6] = {0};
unsigned int opaque_VBOs[6] = {0};
unsigned int opaque_EBOs[6] = {0};
uint32_t opaque_index_counts[6] = {0};

unsigned int transparent_VAOs[6] = {0};
unsigned int transparent_VBOs[6] = {0};
unsigned int transparent_EBOs[6] = {0};
uint32_t transparent_index_counts[6] = {0};

bool mesh_mode = false;
uint16_t draw_calls = 0;
bool mesh_needs_rebuild = false;

void init_gl_buffers() {
	glGenVertexArrays(6, opaque_VAOs);
	glGenBuffers(6, opaque_VBOs);
	glGenBuffers(6, opaque_EBOs);

	glGenVertexArrays(6, transparent_VAOs);
	glGenBuffers(6, transparent_VBOs);
	glGenBuffers(6, transparent_EBOs);

	for (uint8_t face = 0; face < 6; face++) {
		for (uint8_t i = 0; i < 2; i++) {
			if (i == 0) {
				glBindVertexArray(opaque_VAOs[face]);
				glBindBuffer(GL_ARRAY_BUFFER, opaque_VBOs[face]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBOs[face]);
			}
			else {
				glBindVertexArray(transparent_VAOs[face]);
				glBindBuffer(GL_ARRAY_BUFFER, transparent_VBOs[face]);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBOs[face]);
			}
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
			glEnableVertexAttribArray(0);

			glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
			glEnableVertexAttribArray(1);

			glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
			glEnableVertexAttribArray(3);

			// TODO: Re-Enable lighting
			// glVertexAttribIPointer(4, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level0));
			// glEnableVertexAttribArray(4);
			// glVertexAttribIPointer(5, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level1));
			// glEnableVertexAttribArray(5);
			// glVertexAttribIPointer(6, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level2));
			// glEnableVertexAttribArray(6);
			// glVertexAttribIPointer(7, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level3));
			// glEnableVertexAttribArray(7);
			// glVertexAttribIPointer(8, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level4));
			// glEnableVertexAttribArray(8);
			// glVertexAttribIPointer(9, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level5));
			// glEnableVertexAttribArray(9);
			// glVertexAttribIPointer(10, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level6));
			// glEnableVertexAttribArray(10);
			// glVertexAttribIPointer(11, 4, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, ligh_level7));
			// glEnableVertexAttribArray(11);
		}
	}

	glBindVertexArray(0);
}

void rebuild_combined_visible_mesh() {
	// Reset all face counts
	for (uint8_t face = 0; face < 6; face++) {
		opaque_index_counts[face] = 0;
		transparent_index_counts[face] = 0;
		
		glBindVertexArray(opaque_VAOs[face]);
		glBindBuffer(GL_ARRAY_BUFFER, opaque_VBOs[face]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBOs[face]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

		glBindVertexArray(transparent_VAOs[face]);
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBOs[face]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBOs[face]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	// For each face, collect all visible chunks' face data
	for (uint8_t face = 0; face < 6; face++) {
		uint32_t total_opaque_vertices = 0;
		uint32_t total_opaque_indices = 0;
		uint32_t total_transparent_vertices = 0;
		uint32_t total_transparent_indices = 0;

		// First pass: count vertices and indices for this face
		for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (!chunk->is_visible || !chunk->is_loaded) continue;
					
					total_opaque_vertices += chunk->faces[face].vertex_count;
					total_opaque_indices += chunk->faces[face].index_count;
					total_transparent_vertices += chunk->transparent_faces[face].vertex_count;
					total_transparent_indices += chunk->transparent_faces[face].index_count;
				}
			}
		}

		// Skip if nothing to render for this face
		if (total_opaque_vertices == 0 && total_transparent_vertices == 0) continue;

		if (total_opaque_vertices > 0) {
			opaque_index_counts[face] = total_opaque_indices;
			glBindVertexArray(opaque_VAOs[face]);

			glBindBuffer(GL_ARRAY_BUFFER, opaque_VBOs[face]);
			glBufferData(GL_ARRAY_BUFFER, total_opaque_vertices * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
			Vertex* vbo_ptr = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBOs[face]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_opaque_indices * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
			uint32_t* ebo_ptr = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

			if (vbo_ptr && ebo_ptr) {
				uint32_t vertex_offset = 0;
				uint32_t index_offset = 0;
				uint32_t base_vertex = 0;

				for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
					for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
						for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
							if (!chunks[x][y][z].is_visible) continue;
							
							Chunk* chunk = &chunks[x][y][z];
							if (chunk->faces[face].vertex_count == 0) continue;

							memcpy(vbo_ptr + vertex_offset, chunk->faces[face].vertices,
									chunk->faces[face].vertex_count * sizeof(Vertex));

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

			if (vbo_ptr) glUnmapBuffer(GL_ARRAY_BUFFER);
			if (ebo_ptr) glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}

		if (total_transparent_vertices > 0) {
			transparent_index_counts[face] = total_transparent_indices;
			glBindVertexArray(transparent_VAOs[face]);
			
			glBindBuffer(GL_ARRAY_BUFFER, transparent_VBOs[face]);
			glBufferData(GL_ARRAY_BUFFER, total_transparent_vertices * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
			Vertex* vbo_ptr = (Vertex*)glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBOs[face]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_transparent_indices * sizeof(uint32_t), NULL, GL_DYNAMIC_DRAW);
			uint32_t* ebo_ptr = (uint32_t*)glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);

			if (vbo_ptr && ebo_ptr) {
				uint32_t vertex_offset = 0;
				uint32_t index_offset = 0;
				uint32_t base_vertex = 0;

				for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
					for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
						for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
							if (!chunks[x][y][z].is_visible) continue;
							
							Chunk* chunk = &chunks[x][y][z];
							if (chunk->transparent_faces[face].vertex_count == 0) continue;

							memcpy(vbo_ptr + vertex_offset, chunk->transparent_faces[face].vertices,
									chunk->transparent_faces[face].vertex_count * sizeof(Vertex));

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

			if (vbo_ptr) glUnmapBuffer(GL_ARRAY_BUFFER);
			if (ebo_ptr) glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}
	}

	glBindVertexArray(0);
	mesh_needs_rebuild = false;
}

void render_chunks() {
	if (mesh_needs_rebuild)
		rebuild_combined_visible_mesh();
	
	glUseProgram(world_shader);
	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);

	// Render opaque faces
	for (uint8_t face = 0; face < 6; face++) {
		if (opaque_index_counts[face] > 0) {
			glBindVertexArray(opaque_VAOs[face]);
			if (mesh_mode)
				glDrawElements(GL_LINES, opaque_index_counts[face], GL_UNSIGNED_INT, 0);
			else
				glDrawElements(GL_TRIANGLES, opaque_index_counts[face], GL_UNSIGNED_INT, 0);
			draw_calls++;
		}
	}

	// Render transparent faces
	//glDepthMask(GL_FALSE);
	for (uint8_t face = 0; face < 6; face++) {
		if (transparent_index_counts[face] > 0) {
			glBindVertexArray(transparent_VAOs[face]);
			if (mesh_mode)
				glDrawElements(GL_LINES, transparent_index_counts[face], GL_UNSIGNED_INT, 0);
			else
				glDrawElements(GL_TRIANGLES, transparent_index_counts[face], GL_UNSIGNED_INT, 0);
			draw_calls++;
		}
	}
	//glDepthMask(GL_TRUE);
}

void cleanup_renderer() {
	for (uint8_t i = 0; i < 6; i++) {
		glDeleteVertexArrays(1, &opaque_VAOs[i]);
		glDeleteBuffers(1, &opaque_VBOs[i]);
		glDeleteBuffers(1, &opaque_EBOs[i]);

		glDeleteVertexArrays(1, &transparent_VAOs[i]);
		glDeleteBuffers(1, &transparent_VBOs[i]);
		glDeleteBuffers(1, &transparent_EBOs[i]);
	}
}