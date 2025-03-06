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

void print_rendered_chunks(vec3 pos, vec3 dir) {
	printf("\e[1;1H\e[2J");
	printf("Rendered chunks:\n");

	int player_cx = (int)(pos.x / CHUNK_SIZE);
	int player_cz = (int)(pos.z / CHUNK_SIZE);

	int center_cx = last_cx + (RENDER_DISTANCE / 2);
	int center_cz = last_cz + (RENDER_DISTANCE / 2);

	int relative_cx = player_cx - (center_cx - (RENDER_DISTANCE / 2));
	int relative_cz = player_cz - (center_cz - (RENDER_DISTANCE / 2));

	printf("Position: (%f, %f, %f)\n", pos.x, pos.y, pos.z);
	printf("Player chunk: (%d, %d)\n", player_cx, player_cz);
	printf("Center chunk: (%d, %d)\n", center_cx, center_cz);

	float dir_length = sqrt(dir.x * dir.x + dir.z * dir.z);
	vec3 normalized_dir = {dir.x / dir_length, dir.y / dir_length, dir.z / dir_length};

	for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
		for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
			int chunk_x = center_cx - (RENDER_DISTANCE / 2) + x;
			int chunk_z = center_cz - (RENDER_DISTANCE / 2) + z;
			vec3 chunk_pos = {chunk_x * CHUNK_SIZE + CHUNK_SIZE / 2, 0, chunk_z * CHUNK_SIZE + CHUNK_SIZE / 2};
			vec3 player_to_chunk = {chunk_pos.x - pos.x, 0, chunk_pos.z - pos.z};

			float player_to_chunk_length = sqrt(player_to_chunk.x * player_to_chunk.x + player_to_chunk.z * player_to_chunk.z);
			vec3 normalized_player_to_chunk = {player_to_chunk.x / player_to_chunk_length, 0, player_to_chunk.z / player_to_chunk_length};

			float dot_product = normalized_dir.x * normalized_player_to_chunk.x + normalized_dir.z * normalized_player_to_chunk.z;

			float fov_angle = cos(45.0f * M_PI / 180.0f);

			bool is_in_frustum = dot_product >= fov_angle;

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