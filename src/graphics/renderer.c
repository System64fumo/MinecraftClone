#include "main.h"
#include "world.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Single VAO/VBO per mesh type
uint32_t opaque_VAO, opaque_VBO, opaque_EBO, opaque_index_count;
uint32_t transparent_VAO, transparent_VBO, transparent_EBO, transparent_index_count;

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

void render_chunks() {
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