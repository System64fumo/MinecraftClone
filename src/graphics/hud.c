#include "main.h"

#define MAX_UI_ELEMENTS 3
unsigned int highlight_vbo = 0, highlight_vao = 0;
unsigned int ui_vao, ui_vbo;
float ortho[16];
float highlight_matrix[16];

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

ui_element_t ui_elements[MAX_UI_ELEMENTS];
GLuint highlight_vao, highlight_vbo, highlight_ebo;

static const float vertices_template[] = {
	// Front face (Z+)
	-0.505f, -0.505f,  0.505f, // 0
	 0.505f, -0.505f,  0.505f, // 1
	 0.505f,  0.505f,  0.505f, // 2
	-0.505f,  0.505f,  0.505f, // 3

	// Back face (Z-)
	-0.505f, -0.505f, -0.505f, // 4
	 0.505f, -0.505f, -0.505f, // 5
	 0.505f,  0.505f, -0.505f, // 6
	-0.505f,  0.505f, -0.505f  // 7
};

static const unsigned int edge_indices[] = {
	// Front face edges
	0, 1, // Bottom edge
	1, 2, // Right edge
	2, 3, // Top edge
	3, 0, // Left edge

	// Back face edges
	4, 5, // Bottom edge
	5, 6, // Right edge
	6, 7, // Top edge
	7, 4, // Left edge

	// Side edges
	0, 4, // Bottom-left edge
	1, 5, // Bottom-right edge
	2, 6, // Top-right edge
	3, 7  // Top-left edge
};

void init_highlight() {
	glGenVertexArrays(1, &highlight_vao);
	glGenBuffers(1, &highlight_vbo);
	glGenBuffers(1, &highlight_ebo);

	glBindVertexArray(highlight_vao);

	// Bind and upload vertex data
	glBindBuffer(GL_ARRAY_BUFFER, highlight_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_template), vertices_template, GL_STATIC_DRAW);

	// Bind and upload edge index data
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, highlight_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(edge_indices), edge_indices, GL_STATIC_DRAW);

	// Set up vertex attribute pointers
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void draw_block_highlight(float x, float y, float z) {
	glDisable(GL_BLEND);

	matrix4_identity(highlight_matrix);
	matrix4_translate(highlight_matrix, x - 0.5f, y - 0.5f, z - 0.5f);

	glUseProgram(shaderProgram);

	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, highlight_matrix);

	glLineWidth(2.0f);
	glBindVertexArray(highlight_vao);
	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, (void*)0);

	glEnable(GL_BLEND);
}

// Function to update the UI buffer
void update_ui_buffer() {
	// Each UI element now requires 4 vertices (1 triangle strip), each with 4 floats (x, y, tx, ty)
	float vertices[MAX_UI_ELEMENTS * 4 * 4]; 

	for (int i = 0; i < MAX_UI_ELEMENTS; i++) {
		ui_element_t* element = &ui_elements[i];
		float tx = (float)element->tex_x / 256.0f;
		float ty = (float)element->tex_y / 256.0f;
		float tw = (float)element->tex_width / 256.0f;
		float th = (float)element->tex_height / 256.0f;

		int offset = i * 4 * 4; // 4 vertices per element, 4 floats per vertex

		// Vertex order for a triangle strip:
		// Top-left -> Bottom-left -> Top-right -> Bottom-right
		vertices[offset + 0] = element->x - element->width;		// x (top-left)
		vertices[offset + 1] = element->y + element->height;	// y (top-left)
		vertices[offset + 2] = tx;								// tx (top-left)
		vertices[offset + 3] = ty;								// ty (top-left)

		vertices[offset + 4] = element->x - element->width;		// x (bottom-left)
		vertices[offset + 5] = element->y - element->height;	// y (bottom-left)
		vertices[offset + 6] = tx;								// tx (bottom-left)
		vertices[offset + 7] = ty + th;							// ty (bottom-left)

		vertices[offset + 8] = element->x + element->width;		// x (top-right)
		vertices[offset + 9] = element->y + element->height;	// y (top-right)
		vertices[offset + 10] = tx + tw;						// tx (top-right)
		vertices[offset + 11] = ty;								// ty (top-right)

		vertices[offset + 12] = element->x + element->width;	// x (bottom-right)
		vertices[offset + 13] = element->y - element->height;	// y (bottom-right)
		vertices[offset + 14] = tx + tw;						// tx (bottom-right)
		vertices[offset + 15] = ty + th;						// ty (bottom-right)
	}

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
}

void setup_ui_elements() {
	// Crosshair
	ui_elements[0] = (ui_element_t) {
		.x = screen_width / UI_SCALING,
		.y = screen_height / UI_SCALING,
		.width = 16.0f,
		.height = 16.0f,
		.tex_x = 240,
		.tex_y = 0,
		.tex_width = 16,
		.tex_height = 16
	};

	// Hotbar
	ui_elements[1] = (ui_element_t) {
		.x = screen_width / UI_SCALING,
		.y = 22,
		.width = 182,
		.height = 22,
		.tex_x = 0,
		.tex_y = 0,
		.tex_width = 182,
		.tex_height = 22
	};

	// Hotbar slot
	ui_elements[2] = (ui_element_t) {
		.x = screen_width / UI_SCALING - 182 + 22 + (40 * (hotbar_slot % 9)),
		.y = 22,
		.width = 24,
		.height = 24,
		.tex_x = 0,
		.tex_y = 22,
		.tex_width = 24,
		.tex_height = 24
	};

	update_ui_buffer();
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

	setup_ui_elements();
}

void render_ui() {
	glUseProgram(ui_shader);
	glUniformMatrix4fv(glGetUniformLocation(ui_shader, "projection"), 1, GL_FALSE, ortho);
	glBindVertexArray(ui_vao);
	glBindTexture(GL_TEXTURE_2D, ui_textures);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, MAX_UI_ELEMENTS * 4);
}

void cleanup_ui() {
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteProgram(ui_shader);
}

void update_ui() {
	// Crosshair
	ui_elements[0].x = screen_width / UI_SCALING;
	ui_elements[0].y = screen_height / UI_SCALING;

	// Hotbar
	ui_elements[1].x = screen_width / UI_SCALING;
	ui_elements[1].y = 22;

	// Hotbar slot
	ui_elements[2].x = screen_width / UI_SCALING - 182 + 22 + (40 * (hotbar_slot % 9));
	ui_elements[2].y = 22;

	update_ui_buffer();

	matrix4_identity(ortho);
	matrix4_translate(ortho, -1.0f, -1.0f, 0.0f);
	matrix4_scale(ortho, UI_SCALING / screen_width, UI_SCALING / screen_height, 1.0f);
}
