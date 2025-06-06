#include "main.h"
#include <stdlib.h>
#include <string.h>

ChunkRenderData chunk_render_data[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];

void update_gl_buffers() {
	// Update opaque face buffers
	for (uint8_t face = 0; face < 6; face++) {
		glBindVertexArray(opaque_VAOs[face]);
		glBindBuffer(GL_ARRAY_BUFFER, opaque_VBOs[face]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBOs[face]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	// Update transparent face buffers
	for (uint8_t face = 0; face < 6; face++) {
		glBindVertexArray(transparent_VAOs[face]);
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBOs[face]);
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBOs[face]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	}

	glBindVertexArray(0);
}

void combine_meshes() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MESH, false);
	#endif
	
	// Reset all face buffers
	update_gl_buffers();

	// For each face, combine all chunks' face data
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
					total_opaque_vertices += chunk->faces[face].vertex_count;
					total_opaque_indices += chunk->faces[face].index_count;
					total_transparent_vertices += chunk->transparent_faces[face].vertex_count;
					total_transparent_indices += chunk->transparent_faces[face].index_count;
				}
			}
		}

		// Skip if nothing to render for this face
		if (total_opaque_vertices == 0 && total_transparent_vertices == 0) continue;

		// Opaque face data
		if (total_opaque_vertices > 0) {
			glBindVertexArray(opaque_VAOs[face]);
			
			// Resize buffers if needed
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

				for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
					for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
						for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
							Chunk* chunk = &chunks[x][y][z];
							if (chunk->faces[face].vertex_count == 0) {
								chunk_render_data[x][y][z].index_count = 0;
								chunk_render_data[x][y][z].visible = false;
								continue;
							}

							chunk_render_data[x][y][z].start_index = index_offset;
							chunk_render_data[x][y][z].index_count = chunk->faces[face].index_count;
							chunk_render_data[x][y][z].visible = true;

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

		// Transparent face data (similar to opaque)
		if (total_transparent_vertices > 0) {
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

				for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
					for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
						for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
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
	
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MESH, false);
	#endif
}

void process_chunks() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE, false);
	#endif

	bool chunks_updated = false;
	
	// Process all chunks directly
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				
				if (chunk->needs_update) {
					// Only process if all neighbors are loaded
					if (are_all_neighbors_loaded(x, y, z)) {
						set_chunk_lighting(chunk);
						generate_chunk_mesh(chunk);
						chunk->needs_update = false;
						chunks_updated = true;
					}
				}
			}
		}
	}

	if (chunks_updated) {
		combine_meshes();
	}

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MERGE, false);
	#endif
}