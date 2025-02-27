#include "main.h"
#include <stdlib.h>
#include <string.h>

CombinedMesh combined_mesh = {0};
unsigned int combined_VAO = 0;
unsigned int combined_VBO = 0;
unsigned int combined_EBO = 0;

void init_gl_buffers() {
	if (combined_VAO == 0) {
		glGenVertexArrays(1, &combined_VAO);
		glGenBuffers(1, &combined_VBO);
		glGenBuffers(1, &combined_EBO);
		
		// Setup vertex attributes
		glBindVertexArray(combined_VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
		glEnableVertexAttribArray(1);

		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, width));
		glEnableVertexAttribArray(3);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
		
		// Initial allocation with 0 size
		glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
		
		glBindVertexArray(0);
	}
}

void combine_meshes() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE);
	#endif
	
	uint32_t total_vertices = 0;
	uint32_t total_indices = 0;

	// Calculate total sizes in a single pass
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
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_MERGE);
		#endif
		return;
	}
	
	// Reuse existing buffers if they're large enough
	if (combined_mesh.capacity_vertices < total_vertices) {
		free(combined_mesh.vertices);
		combined_mesh.vertices = malloc(total_vertices * sizeof(Vertex));
		combined_mesh.capacity_vertices = total_vertices;
	}
	
	if (combined_mesh.capacity_indices < total_indices) {
		free(combined_mesh.indices);
		combined_mesh.indices = malloc(total_indices * sizeof(uint32_t));
		combined_mesh.capacity_indices = total_indices;
	}

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

	// Update buffer data using glBufferSubData for better performance
	glBindVertexArray(combined_VAO);
	
	// Check if buffer sizes need to be increased
	GLint current_vbo_size = 0;
	GLint current_ebo_size = 0;
	glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_vbo_size);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_ebo_size);
	
	size_t required_vbo_size = combined_mesh.vertex_count * sizeof(Vertex);
	size_t required_ebo_size = combined_mesh.index_count * sizeof(uint32_t);
	
	// Resize buffers if needed
	if (current_vbo_size < required_vbo_size) {
		glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
		glBufferData(GL_ARRAY_BUFFER, required_vbo_size, NULL, GL_DYNAMIC_DRAW);
	}
	
	if (current_ebo_size < required_ebo_size) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, required_ebo_size, NULL, GL_DYNAMIC_DRAW);
	}
	
	// Update buffer contents with glBufferSubData
	glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, required_vbo_size, combined_mesh.vertices);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, required_ebo_size, combined_mesh.indices);
	
	glBindVertexArray(0);

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MERGE);
	#endif
}

void process_chunks() {
	#ifdef DEBUG
	static bool baking = false;
	#endif
	
	bool chunks_updated = false;

	// Process all chunks that need updating
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->needs_update) {
					generate_chunk_mesh(chunk);
					chunks_updated = true;
					#ifdef DEBUG
					if (!baking) {
						profiler_start(PROFILER_ID_BAKE);
						baking = true;
					}
					#endif
				}
			}
		}
	}

	if (chunks_updated)
		combine_meshes();

	#ifdef DEBUG
	if (baking) {
		profiler_stop(PROFILER_ID_BAKE);
		baking = false;
	}
	#endif
}

void render_chunks() {
	glUseProgram(shaderProgram);
	
	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);

	glBindVertexArray(combined_VAO);
	glDrawElements(GL_TRIANGLES, combined_mesh.index_count, GL_UNSIGNED_INT, 0);
}

void cleanup_renderer() {
	if (combined_VAO != 0) {
		glDeleteVertexArrays(1, &combined_VAO);
		glDeleteBuffers(1, &combined_VBO);
		glDeleteBuffers(1, &combined_EBO);
		combined_VAO = 0;
		combined_VBO = 0;
		combined_EBO = 0;
	}
	
	free(combined_mesh.vertices);
	free(combined_mesh.indices);
	combined_mesh.vertices = NULL;
	combined_mesh.indices = NULL;
	combined_mesh.capacity_vertices = 0;
	combined_mesh.capacity_indices = 0;
	combined_mesh.vertex_count = 0;
	combined_mesh.index_count = 0;
}