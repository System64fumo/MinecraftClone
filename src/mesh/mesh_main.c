#include "main.h"
#include <stdlib.h>
#include <string.h>

ChunkRenderData chunk_render_data[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];

void update_gl_buffers() {
	// Update opaque buffers
	glBindVertexArray(combined_VAO);
	
	if (combined_mesh.vertex_count > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
		glBufferData(GL_ARRAY_BUFFER, 
					combined_mesh.vertex_count * sizeof(Vertex), 
					combined_mesh.vertices, GL_STREAM_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
					combined_mesh.index_count * sizeof(uint32_t), 
					combined_mesh.indices, GL_STREAM_DRAW);
	}
	
	// Update transparent buffers
	glBindVertexArray(transparent_VAO);
	
	if (transparent_mesh.vertex_count > 0) {
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBO);
		glBufferData(GL_ARRAY_BUFFER, 
					transparent_mesh.vertex_count * sizeof(Vertex), 
					transparent_mesh.vertices, GL_STREAM_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
					transparent_mesh.index_count * sizeof(uint32_t), 
					transparent_mesh.indices, GL_STREAM_DRAW);
	}
	
	glBindVertexArray(0);
}

void combine_meshes() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MESH, false);
	#endif
	
	uint32_t total_vertices = 0;
	uint32_t total_indices = 0;
	uint32_t total_transparent_vertices = 0;
	uint32_t total_transparent_indices = 0;

	// Calculate total sizes
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				total_vertices += chunk->vertex_count;
				total_indices += chunk->index_count;
				total_transparent_vertices += chunk->transparent_vertex_count;
				total_transparent_indices += chunk->transparent_index_count;
			}
		}
	}

	// Handle opaque mesh
	if (total_vertices > 0 && total_indices > 0) {
		// Reallocate if needed
		combined_mesh.vertices = realloc(combined_mesh.vertices, total_vertices * sizeof(Vertex));
		combined_mesh.indices = realloc(combined_mesh.indices, total_indices * sizeof(uint32_t));
		combined_mesh.capacity_vertices = total_vertices;
		combined_mesh.capacity_indices = total_indices;

		// Combine meshes
		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;
		uint32_t base_vertex = 0;

		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (chunk->vertex_count == 0) {
						chunk_render_data[x][y][z].index_count = 0;
						chunk_render_data[x][y][z].visible = false;
						continue;
					}

					chunk_render_data[x][y][z].start_index = index_offset;
					chunk_render_data[x][y][z].index_count = chunk->index_count;
					chunk_render_data[x][y][z].visible = true;

					memcpy(combined_mesh.vertices + vertex_offset, chunk->vertices,
						   chunk->vertex_count * sizeof(Vertex));

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
	} else {
		combined_mesh.vertex_count = 0;
		combined_mesh.index_count = 0;
	}

	// Handle transparent mesh
	if (total_transparent_vertices > 0 && total_transparent_indices > 0) {
		// Reallocate if needed
		transparent_mesh.vertices = realloc(transparent_mesh.vertices, 
			total_transparent_vertices * sizeof(Vertex));
		transparent_mesh.indices = realloc(transparent_mesh.indices, 
			total_transparent_indices * sizeof(uint32_t));
		transparent_mesh.capacity_vertices = total_transparent_vertices;
		transparent_mesh.capacity_indices = total_transparent_indices;

		// Combine transparent meshes
		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;
		uint32_t base_vertex = 0;

		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (chunk->transparent_vertex_count == 0) continue;

					memcpy(transparent_mesh.vertices + vertex_offset, 
						   chunk->transparent_vertices,
						   chunk->transparent_vertex_count * sizeof(Vertex));

					for (uint32_t i = 0; i < chunk->transparent_index_count; i++) {
						transparent_mesh.indices[index_offset + i] = 
							chunk->transparent_indices[i] + base_vertex;
					}

					vertex_offset += chunk->transparent_vertex_count;
					index_offset += chunk->transparent_index_count;
					base_vertex += chunk->transparent_vertex_count;
				}
			}
		}

		transparent_mesh.vertex_count = vertex_offset;
		transparent_mesh.index_count = index_offset;
	} else {
		transparent_mesh.vertex_count = 0;
		transparent_mesh.index_count = 0;
	}
	
	update_gl_buffers();
	
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
					set_chunk_lighting(chunk);
					generate_chunk_mesh(chunk);
					chunk->needs_update = false;
					chunks_updated = true;
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