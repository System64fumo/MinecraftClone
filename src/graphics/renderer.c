#include "main.h"
#include "world.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Face-based VAOs/VBOs
unsigned int opaque_VAOs[6] = {0};
unsigned int opaque_VBOs[6] = {0};
unsigned int opaque_EBOs[6] = {0};

unsigned int transparent_VAOs[6] = {0};
unsigned int transparent_VBOs[6] = {0};
unsigned int transparent_EBOs[6] = {0};

bool mesh_mode = false;
uint16_t draw_calls = 0;
bool frustum_culling_enabled = true;
float frustum_offset = CHUNK_SIZE * 2;
bool mesh_needs_rebuild = false;

void init_gl_buffers() {
	glGenVertexArrays(6, opaque_VAOs);
	glGenBuffers(6, opaque_VBOs);
	glGenBuffers(6, opaque_EBOs);

	glGenVertexArrays(6, transparent_VAOs);
	glGenBuffers(6, transparent_VBOs);
	glGenBuffers(6, transparent_EBOs);

	// Setup each face VAO
	for (uint8_t face = 0; face < 6; face++) {
		// Opaque faces
		glBindVertexArray(opaque_VAOs[face]);
		glBindBuffer(GL_ARRAY_BUFFER, opaque_VBOs[face]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, opaque_EBOs[face]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
		glEnableVertexAttribArray(0);

		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
		glEnableVertexAttribArray(1);

		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
		glEnableVertexAttribArray(3);

		glBindVertexArray(transparent_VAOs[face]);
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBOs[face]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBOs[face]);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
		glEnableVertexAttribArray(0);

		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
		glEnableVertexAttribArray(1);

		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
		glEnableVertexAttribArray(3);
	}

	glBindVertexArray(0);
}

bool is_chunk_in_frustum(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	float dir_length = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	
	if (dir_length < 0.001f) {
		return true;
	}

	vec3 normalized_dir = {dir.x / dir_length, dir.y / dir_length, dir.z / dir_length};

	vec3 frustum_origin = {
		pos.x - normalized_dir.x * frustum_offset,
		pos.y - (CHUNK_SIZE * 4) - normalized_dir.y * frustum_offset,
		pos.z - normalized_dir.z * frustum_offset
	};

	vec3 chunk_pos = {
		chunk_x * CHUNK_SIZE + CHUNK_SIZE / 2,
		chunk_y * CHUNK_SIZE + CHUNK_SIZE / 2,
		chunk_z * CHUNK_SIZE + CHUNK_SIZE / 2
	};

	vec3 origin_to_chunk = {
		chunk_pos.x - frustum_origin.x,
		chunk_pos.y - frustum_origin.y,
		chunk_pos.z - frustum_origin.z
	};

	float origin_to_chunk_length = sqrt(
		origin_to_chunk.x * origin_to_chunk.x + 
		origin_to_chunk.y * origin_to_chunk.y + 
		origin_to_chunk.z * origin_to_chunk.z
	);

	if (origin_to_chunk_length < 0.001f) {
		return true;
	}

	vec3 normalized_origin_to_chunk = {
		origin_to_chunk.x / origin_to_chunk_length, 
		origin_to_chunk.y / origin_to_chunk_length, 
		origin_to_chunk.z / origin_to_chunk_length
	};

	float dot_product = 
		normalized_dir.x * normalized_origin_to_chunk.x + 
		normalized_dir.y * normalized_origin_to_chunk.y +
		normalized_dir.z * normalized_origin_to_chunk.z;

	return dot_product >= fov_angle;
}

void update_frustum() {
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].pos.x;
	pos.y = global_entities[0].pos.y + global_entities[0].eye_level;
	pos.z = global_entities[0].pos.z;

	float fov_angle = cos(settings.fov * M_PI / 180.0f);

	int center_cx = last_cx + (RENDER_DISTANCE / 2);
	int center_cy = last_cy + (WORLD_HEIGHT / 2);
	int center_cz = last_cz + (RENDER_DISTANCE / 2);

	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				int chunk_x = center_cx - (RENDER_DISTANCE / 2) + x;
				int chunk_y = center_cy - (WORLD_HEIGHT / 2) + y;
				int chunk_z = center_cz - (RENDER_DISTANCE / 2) + z;

				bool visible = true;

				if (frustum_culling_enabled) {
					visible = is_chunk_in_frustum(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle);
				}

				// If visibility changed, mark for potential mesh rebuild
				if (chunk_render_data[x][y][z].visible != visible) {
					mesh_needs_rebuild = true;
				}
				
				chunk_render_data[x][y][z].visible = visible;
			}
		}
	}
}

void rebuild_combined_visible_mesh() {
	// Reset all face counts
	for (int face = 0; face < 6; face++) {
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
	for (int face = 0; face < 6; face++) {
		uint32_t total_opaque_vertices = 0;
		uint32_t total_opaque_indices = 0;
		uint32_t total_transparent_vertices = 0;
		uint32_t total_transparent_indices = 0;

		// First pass: count vertices and indices for this face
		for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					ChunkRenderData* render_data = &chunk_render_data[x][y][z];
					if (!render_data->visible) continue;
					
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

				for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
					for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
						for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
							ChunkRenderData* render_data = &chunk_render_data[x][y][z];
							if (!render_data->visible) continue;
							
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

				for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
					for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
						for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
							ChunkRenderData* render_data = &chunk_render_data[x][y][z];
							if (!render_data->visible) continue;
							
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

	glUseProgram(shaderProgram);
	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);

	// Render opaque faces
	for (int face = 0; face < 6; face++) {
		glBindVertexArray(opaque_VAOs[face]);
		GLint index_count;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &index_count);
		index_count /= sizeof(uint32_t);
		
		if (index_count > 0) {
			if (mesh_mode)
				glDrawElements(GL_LINES, index_count, GL_UNSIGNED_INT, 0);
			else
				glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
			draw_calls++;
		}
	}

	// Render transparent faces (back to front sorting might be needed)
	for (int face = 0; face < 6; face++) {
		glBindVertexArray(transparent_VAOs[face]);
		GLint index_count;
		glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &index_count);
		index_count /= sizeof(uint32_t);
		
		if (index_count > 0) {
			if (mesh_mode)
				glDrawElements(GL_LINES, index_count, GL_UNSIGNED_INT, 0);
			else
				glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, 0);
			draw_calls++;
		}
	}
}

void cleanup_renderer() {
	for (int i = 0; i < 6; i++) {
		glDeleteVertexArrays(1, &opaque_VAOs[i]);
		glDeleteBuffers(1, &opaque_VBOs[i]);
		glDeleteBuffers(1, &opaque_EBOs[i]);

		glDeleteVertexArrays(1, &transparent_VAOs[i]);
		glDeleteBuffers(1, &transparent_VBOs[i]);
		glDeleteBuffers(1, &transparent_EBOs[i]);
	}
}