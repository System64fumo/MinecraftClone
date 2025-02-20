#include "main.h"

static const float vertices_template[] = {
	// Front face (Z+)
	-0.505f, -0.505f, 0.505f,
	0.505f, -0.505f, 0.505f,
	0.505f, 0.505f, 0.505f,
	-0.505f, 0.505f, 0.505f,
	-0.505f, -0.505f, 0.505f,

	// Back face (Z-)
	-0.505f, -0.505f, -0.505f,
	-0.505f, 0.505f, -0.505f,
	0.505f, 0.505f, -0.505f,
	0.505f, -0.505f, -0.505f,
	-0.505f, -0.505f, -0.505f,

	// Left face (X-)
	-0.505f, -0.505f, 0.505f,
	-0.505f, -0.505f, -0.505f,
	-0.505f, 0.505f, -0.505f,
	-0.505f, 0.505f, 0.505f,
	-0.505f, -0.505f, 0.505f,

	// Right face (X+)
	0.505f, -0.505f, 0.505f,
	0.505f, 0.505f, 0.505f,
	0.505f, 0.505f, -0.505f,
	0.505f, -0.505f, -0.505f,
	0.505f, -0.505f, 0.505f,

	// Bottom face (Y-)
	-0.505f, -0.505f, 0.505f,
	0.505f, -0.505f, 0.505f,
	0.505f, -0.505f, -0.505f,
	-0.505f, -0.505f, -0.505f,
	-0.505f, -0.505f, 0.505f,

	// Top face (Y+)
	-0.505f, 0.505f, 0.505f,
	-0.505f, 0.505f, -0.505f,
	0.505f, 0.505f, -0.505f,
	0.505f, 0.505f, 0.505f,
	-0.505f, 0.505f, 0.505f, 
};

static unsigned int vbo = 0, vao = 0;

void draw_block_highlight(float x, float y, float z) {
	glDisable(GL_BLEND);
	if (vbo == 0) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_template), vertices_template, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}

	float translationMatrix[16];
	matrix4_identity(translationMatrix);
	matrix4_translate(translationMatrix, x - 0.5f, y - 0.5f, z - 0.5f);

	glUseProgram(shaderProgram);

	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

	glLineWidth(2.0f);

	glBindVertexArray(vao);

	for (int i = 0; i < 6; i++) {
		glDrawArrays(GL_LINE_LOOP, i * 5, 5);
	}

	glBindVertexArray(0);
	glUseProgram(0);
	glEnable(GL_BLEND);
}