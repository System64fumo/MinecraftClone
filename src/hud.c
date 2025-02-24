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

unsigned int ui_vao, ui_vbo;

typedef struct {
	float x;
	float y;
	float width;
	float height;
	int tex_x;
	int tex_y;
	int tex_width;
	int tex_height;
} ui_element_t;

void render_ui_element(const ui_element_t* element, int texture_width, int texture_height) {
	float tx = (float)element->tex_x / texture_width;
	float ty = (float)element->tex_y / texture_height;
	float tw = (float)element->tex_width / texture_width;
	float th = (float)element->tex_height / texture_height;

	float vertices[] = {
		element->x - element->width, element->y + element->height, tx, ty,
		element->x - element->width, element->y - element->height, tx, ty + th,
		element->x + element->width, element->y - element->height, tx + tw, ty + th,
		element->x + element->width, element->y + element->height, tx + tw, ty
	};

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

void init_ui() {
	glGenVertexArrays(1, &ui_vao);
	glGenBuffers(1, &ui_vbo);
	glBindVertexArray(ui_vao);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void render_ui() {
	glDisable(GL_DEPTH_TEST);
	glUseProgram(ui_shader);

	float ortho[16];
	matrix4_identity(ortho);
	matrix4_translate(ortho, -1.0f, -1.0f, 0.0f);
	matrix4_scale(ortho, UI_SCALING / screen_width, UI_SCALING / screen_height, 1.0f);
	glUniformMatrix4fv(glGetUniformLocation(ui_shader, "projection"), 1, GL_FALSE, ortho);

	glBindVertexArray(ui_vao);
	glBindTexture(GL_TEXTURE_2D, ui_textures);

	ui_element_t crosshair = {
		.x = screen_width / UI_SCALING,
		.y = screen_height / UI_SCALING,
		.width = 16.0f,
		.height = 16.0f,
		.tex_x = 240,
		.tex_y = 0,
		.tex_width = 16,
		.tex_height = 16
	};
	render_ui_element(&crosshair, 256, 256);

	ui_element_t hotbar = {
		.x = screen_width / UI_SCALING,
		.y = 22,
		.width = 182,
		.height = 22,
		.tex_x = 0,
		.tex_y = 0,
		.tex_width = 182,
		.tex_height = 22
	};
	render_ui_element(&hotbar, 256, 256);
}

void cleanup_ui() {
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteProgram(ui_shader);
}