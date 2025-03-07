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
	float dir_length_xz = sqrt(dir.x * dir.x + dir.z * dir.z);
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
	int center_cx = last_cx + (RENDER_DISTANCE / 2);
	int center_cy = last_cy + (WORLD_HEIGHT / 2);
	int center_cz = last_cz + (RENDER_DISTANCE / 2);

	float fov_angle = cos(fov * M_PI / 180.0f);

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

				chunk_render_data[x][y][z].visible = visible;
			}
		}
	}
}

void print_rendered_chunks(vec3 pos, vec3 dir) {
	printf("\e[1;1H\e[2J");
	printf("Rendered chunks:\n");

	int player_cx = (int)(pos.x / CHUNK_SIZE);
	int player_cy = (int)(pos.y / CHUNK_SIZE);
	int player_cz = (int)(pos.z / CHUNK_SIZE);

	int center_cx = last_cx + (RENDER_DISTANCE / 2);
	int center_cy = last_cy + (WORLD_HEIGHT / 2);
	int center_cz = last_cz + (RENDER_DISTANCE / 2);

	int relative_cx = player_cx - (center_cx - (RENDER_DISTANCE / 2));
	int relative_cy = player_cy - (center_cy - (WORLD_HEIGHT / 2));
	int relative_cz = player_cz - (center_cz - (RENDER_DISTANCE / 2));

	printf("Position: (%f, %f, %f)\n", pos.x, pos.y, pos.z);
	printf("Player chunk: (%d, %d, %d)\n", player_cx, player_cy, player_cz);
	printf("Center chunk: (%d, %d, %d)\n", center_cx, center_cy, center_cz);

	float fov_angle = cos(fov * M_PI / 180.0f);

	printf("Layer y=%d:\n", center_cy);
	for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
		for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
			int chunk_x = center_cx - (RENDER_DISTANCE / 2) + x;
			int chunk_y = center_cy;
			int chunk_z = center_cz - (RENDER_DISTANCE / 2) + z;
			
			bool is_in_frustum = is_chunk_in_frustum(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle);

			if (x == relative_cx && z == relative_cz) {
				printf("[o]");
			} else if (is_in_frustum) {
				printf("[x]");
			} else {
				printf("[ ]");
			}
		}
		printf("\n");
	}
}

void render_chunks() {
	draw_calls = 0;
	if (combined_mesh.index_count == 0)
		return;

	glUseProgram(shaderProgram);
	
	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);

	glBindVertexArray(combined_VAO);

	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				ChunkRenderData* render_data = &chunk_render_data[x][y][z];
				if (!render_data->visible || render_data->index_count == 0)
					continue;
					
				if (mesh_mode)
					glDrawElements(GL_LINES, render_data->index_count, GL_UNSIGNED_INT, 
								 (void*)(render_data->start_index * sizeof(uint32_t)));
				else
					glDrawElements(GL_TRIANGLES, render_data->index_count, GL_UNSIGNED_INT, 
								 (void*)(render_data->start_index * sizeof(uint32_t)));

				 draw_calls++;
			}
		}
	}
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