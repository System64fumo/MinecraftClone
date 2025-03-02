#include "main.h"
#include <stdlib.h>
#include <string.h>

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

void render_chunks() {
	if (combined_mesh.index_count == 0)
		return;

	glUseProgram(shaderProgram);
	
	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);

	glBindVertexArray(combined_VAO);
	if (mesh_mode)
		glDrawElements(GL_LINES, combined_mesh.index_count, GL_UNSIGNED_INT, 0);
	else
		glDrawElements(GL_TRIANGLES, combined_mesh.index_count, GL_UNSIGNED_INT, 0);
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