#include "main.h"
#include "world.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

CombinedMesh combined_mesh = {0};
unsigned int combined_VAO = 0;
unsigned int combined_VBO = 0;
unsigned int combined_EBO = 0;
bool mesh_mode = false;
uint16_t draw_calls = 0;
bool frustum_culling_enabled = true;
float frustum_offset = CHUNK_SIZE * 2;
bool mesh_needs_rebuild = false;

void init_gl_buffers() {
	glGenVertexArrays(1, &combined_VAO);
	glGenBuffers(1, &combined_VBO);
	glGenBuffers(1, &combined_EBO);

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

	glVertexAttribPointer(4, 1, GL_UNSIGNED_INT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, light_data));
	glEnableVertexAttribArray(4);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);

	glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);

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
		pos.y - normalized_dir.y * frustum_offset,
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

void update_chunks_visibility(vec3 pos, vec3 dir) {
	float fov_angle = cos(fov * M_PI / 180.0f);

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

void render_chunks_combined() {
	if (combined_mesh.index_count == 0)
		return;

	glUseProgram(shaderProgram);
	
	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);

	glBindVertexArray(combined_VAO);

	// Single draw call for all visible chunks
	if (mesh_mode)
		glDrawElements(GL_LINES, combined_mesh.index_count, GL_UNSIGNED_INT, 0);
	else
		glDrawElements(GL_TRIANGLES, combined_mesh.index_count, GL_UNSIGNED_INT, 0);
	
	draw_calls = 1;
}

void rebuild_combined_visible_mesh() {
	pthread_mutex_lock(&mesh_mutex);
	
	uint32_t total_vertices = 0;
	uint32_t total_indices = 0;

	// First pass: count total vertices and indices for visible chunks only
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				ChunkRenderData* render_data = &chunk_render_data[x][y][z];
				if (!render_data->visible || render_data->index_count == 0)
					continue;
				
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
		pthread_mutex_unlock(&mesh_mutex);
		mesh_needs_rebuild = false;
		return;
	}

	// Reuse existing buffers if they're large enough
	if (combined_mesh.capacity_vertices < total_vertices) {
		void* new_vertices = malloc(total_vertices * sizeof(Vertex));
		if (new_vertices) {
			free(combined_mesh.vertices);
			combined_mesh.vertices = new_vertices;
			combined_mesh.capacity_vertices = total_vertices;
		}
		else {
			pthread_mutex_unlock(&mesh_mutex);
			fprintf(stderr, "Failed to allocate vertex memory for visible mesh\n");
			return;
		}
	}

	if (combined_mesh.capacity_indices < total_indices) {
		void* new_indices = malloc(total_indices * sizeof(uint32_t));
		if (new_indices) {
			free(combined_mesh.indices);
			combined_mesh.indices = new_indices;
			combined_mesh.capacity_indices = total_indices;
		}
		else {
			pthread_mutex_unlock(&mesh_mutex);
			fprintf(stderr, "Failed to allocate index memory for visible mesh\n");
			return;
		}
	}

	// Combine only visible meshes
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	uint32_t base_vertex = 0;

	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				ChunkRenderData* render_data = &chunk_render_data[x][y][z];
				if (!render_data->visible || render_data->index_count == 0)
					continue;
				
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->vertex_count == 0 || chunk->vertices == NULL)
					continue;

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
	
	pthread_mutex_unlock(&mesh_mutex);

	// Update buffer data
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
	
	// Update buffer contents
	glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, required_vbo_size, combined_mesh.vertices);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, required_ebo_size, combined_mesh.indices);
	
	glBindVertexArray(0);
	
	mesh_needs_rebuild = false;
}

void render_chunks() {
	if (mesh_needs_rebuild)
		rebuild_combined_visible_mesh();

	render_chunks_combined();
}

void cleanup_renderer() {
	glDeleteVertexArrays(1, &combined_VAO);
	glDeleteBuffers(1, &combined_VBO);
	glDeleteBuffers(1, &combined_EBO);
	combined_VAO = 0;
	combined_VBO = 0;
	combined_EBO = 0;

	free(combined_mesh.vertices);
	free(combined_mesh.indices);
	combined_mesh.vertices = NULL;
	combined_mesh.indices = NULL;
	combined_mesh.capacity_vertices = 0;
	combined_mesh.capacity_indices = 0;
	combined_mesh.vertex_count = 0;
	combined_mesh.index_count = 0;
}