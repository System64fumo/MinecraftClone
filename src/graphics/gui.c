#include "main.h"
#include "gui.h"
#include <math.h>
#include <string.h>

uint8_t ui_state = 0;
unsigned int highlight_vbo = 0, highlight_vao = 0;
unsigned int ui_vao, ui_vbo;
float ortho[16];
float cube_projection[16];
float cube_view[16];
float highlight_matrix[16];
float cube_matrix[16];

ui_element_t ui_elements[MAX_UI_ELEMENTS];
cube_element_t cube_elements[MAX_CUBE_ELEMENTS];
GLuint highlight_vao, highlight_vbo, highlight_ebo;
GLuint cube_vao, cube_vbo, cube_ebo;
GLuint cube_color_vbo;
GLuint cube_normal_vbo;
uint16_t ui_center_x, ui_center_y;

void update_cube_projection() {
	float left = 0;
	float right = settings.window_width;
	float bottom = 0;
	float top = settings.window_height;
	float near = -100.0f;
	float far = 100.0f;
	
	matrix4_identity(cube_projection);
	cube_projection[0] = 2.0f / (right - left);
	cube_projection[5] = 2.0f / (top - bottom);
	cube_projection[10] = -2.0f / (far - near);
	cube_projection[12] = -(right + left) / (right - left);
	cube_projection[13] = -(top + bottom) / (top - bottom);
	cube_projection[14] = -(far + near) / (far - near);
	
	matrix4_identity(cube_view);
}

void init_cube_rendering() {	
	// Create and bind VAO
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);

	// Create and bind VBO for vertex positions
	glGenBuffers(1, &cube_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	
	// Set up vertex attributes (location 0 - positions)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	
	// Texture coordinates (location 1)
	glGenBuffers(1, &cube_color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex_coords), cube_tex_coords, GL_STATIC_DRAW);
	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	
	// Normals (location 2)
	glGenBuffers(1, &cube_normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_normal_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
	
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);
	
	// Create and bind EBO
	glGenBuffers(1, &cube_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);
	
	// Unbind VAO
	glBindVertexArray(0);

	update_cube_projection();
}

void init_block_highlight() {
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

void init_ui() {
	init_block_highlight();
	init_cube_rendering();

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

void draw_block_highlight(vec3 pos) {
	glDisable(GL_BLEND);

	matrix4_identity(highlight_matrix);
	matrix4_translate(highlight_matrix, pos.x, pos.y + 0.001f, pos.z);

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, highlight_matrix);

	glLineWidth(2.0f);
	glBindVertexArray(highlight_vao);
	glDrawElements(GL_LINES, sizeof(edge_indices), GL_UNSIGNED_BYTE, (void*)0);
	draw_calls++;

	glEnable(GL_BLEND);
}


void draw_cube_element(const cube_element_t* cube) {
	glUseProgram(cube_shader);

	matrix4_identity(cube_matrix);

	float temp[16], rotation[16];

	if (cube->rotation_z != 0.0f) {
		matrix4_rotate_z(rotation, cube->rotation_z);
		memcpy(temp, cube_matrix, sizeof(float) * 16);
		matrix4_multiply(cube_matrix, temp, rotation);
	}
	
	if (cube->rotation_y != 0.0f) {
		matrix4_rotate_y(rotation, cube->rotation_y);
		memcpy(temp, cube_matrix, sizeof(float) * 16);
		matrix4_multiply(cube_matrix, temp, rotation);
	}
	
	if (cube->rotation_x != 0.0f) {
		matrix4_rotate_x(rotation, cube->rotation_x);
		memcpy(temp, cube_matrix, sizeof(float) * 16);
		matrix4_multiply(cube_matrix, temp, rotation);
	}

	matrix4_translate(cube_matrix, cube->pos.x, cube->pos.y, 0);
	matrix4_scale(cube_matrix, cube->width, cube->height, cube->depth);

	GLint proj_loc = glGetUniformLocation(cube_shader, "projection");
	GLint view_loc = glGetUniformLocation(cube_shader, "view");
	GLint model_loc = glGetUniformLocation(cube_shader, "model");
	GLint tex_loc = glGetUniformLocation(cube_shader, "texParams");

	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, cube_projection);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, cube_view);
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, cube_matrix);

	// Calculate texture parameters
	float tx = (float)cube->tex_x / 256.0f;
	float ty = (float)cube->tex_y / 256.0f;
	float tw = (float)cube->tex_width / 256.0f;
	float th = (float)cube->tex_height / 256.0f;
	glUniform4f(tex_loc, tx, ty, tw, th);

	glBindTexture(GL_TEXTURE_2D, block_textures);
	glBindVertexArray(cube_vao);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);
	draw_calls++;
}

void update_ui_buffer() {
	// Each UI element now requires 4 vertices (1 triangle strip), each with 4 floats (x, y, tx, ty)
	float vertices[MAX_UI_ELEMENTS * 4 * 4]; 

	for (uint8_t i = 0; i < MAX_UI_ELEMENTS; i++) {
		ui_element_t* element = &ui_elements[i];
		float tx = (float)element->tex_x / 256.0f;
		float ty = (float)element->tex_y / 256.0f;
		float tw = (float)element->tex_width / 256.0f;
		float th = (float)element->tex_height / 256.0f;

		uint8_t offset = i * 4 * 4; // 4 vertices per element, 4 floats per vertex

		// Calculate pixel-perfect coordinates with 0.375 offset
		float x0 = floor(element->x - element->width) + 0.375f;
		float x1 = floor(element->x + element->width) + 0.375f;
		float y0 = floor(element->y - element->height) + 0.375f;
		float y1 = floor(element->y + element->height) + 0.375f;

		// Vertex order for triangle strip:
		// Top-left -> Bottom-left -> Top-right -> Bottom-right
		vertices[offset + 0] = x0;  // top-left x
		vertices[offset + 1] = y1;  // top-left y
		vertices[offset + 2] = tx;
		vertices[offset + 3] = ty;

		vertices[offset + 4] = x0;  // bottom-left x
		vertices[offset + 5] = y0;  // bottom-left y
		vertices[offset + 6] = tx;
		vertices[offset + 7] = ty + th;

		vertices[offset + 8] = x1;  // top-right x
		vertices[offset + 9] = y1;  // top-right y
		vertices[offset + 10] = tx + tw;
		vertices[offset + 11] = ty;

		vertices[offset + 12] = x1;  // bottom-right x
		vertices[offset + 13] = y0;  // bottom-right y
		vertices[offset + 14] = tx + tw;
		vertices[offset + 15] = ty + th;
	}

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
}

void setup_ui_elements() {
	// Crosshair
	ui_elements[0] = (ui_element_t) {
		.x = settings.window_width / UI_SCALING,
		.y = settings.window_height / UI_SCALING,
		.width = 16,
		.height = 16,
		.tex_x = 240,
		.tex_y = 0,
		.tex_width = 16,
		.tex_height = 16
	};

	// Hotbar
	ui_elements[1] = (ui_element_t) {
		.x = settings.window_width / UI_SCALING,
		.y = 20,
		.width = 182,
		.height = 22,
		.tex_x = 0,
		.tex_y = 0,
		.tex_width = 182,
		.tex_height = 22
	};

	// Hotbar slot
	ui_elements[2] = (ui_element_t) {
		.x = settings.window_width / UI_SCALING - 182 + 22 + (40 * (hotbar_slot % 9)),
		.y = 20,
		.width = 24,
		.height = 24,
		.tex_x = 0,
		.tex_y = 22,
		.tex_width = 24,
		.tex_height = 24
	};

	// Hotbar blocks
	const cube_element_t base_cube = {
		.pos.y = 10.333f * UI_SCALING,
		.width = 10.05 * UI_SCALING,
		.height = 10.05 * UI_SCALING,
		.depth = 10.05 * UI_SCALING,
		.rotation_x = -30 * (M_PI / 180.0f),
		.rotation_y = 45 * (M_PI / 180.0f),
		.tex_width = 16,
		.tex_height = 16
	};

	uint8_t tex_x_coords[] = {32, 48, 16, 0, 64, 240, 16, 224, 224};
	uint8_t tex_y_coords[] = {0, 0, 0, 16, 0, 0, 16, 208, 208};

	for (int i = 0; i < MAX_CUBE_ELEMENTS; i++) {
		cube_elements[i] = base_cube;
		cube_elements[i].tex_x = tex_x_coords[i];
		cube_elements[i].tex_y = tex_y_coords[i];
	}

	update_ui_buffer();
}

void render_ui() {
	glUseProgram(ui_shader);
	glUniformMatrix4fv(ui_projection_uniform_location, 1, GL_FALSE, ortho);
	glBindVertexArray(ui_vao);
	glBindTexture(GL_TEXTURE_2D, ui_textures);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, MAX_UI_ELEMENTS * 4);
	draw_calls++;

	// Hotbar blocks
	for (uint8_t i = 0; i < MAX_CUBE_ELEMENTS; i++)
		draw_cube_element(&cube_elements[i]);
}

void update_ui() {
	ui_center_x = settings.window_width / UI_SCALING;
	ui_center_y = settings.window_height / UI_SCALING;
	switch (ui_state) {
		case 0: // Game view
			uint16_t hotbar_offset = 80 * UI_SCALING;

			// Crosshair
			ui_elements[0].x = ui_center_x;
			ui_elements[0].y = ui_center_y;
		
			// Hotbar
			ui_elements[1].x = ui_center_x;
		
			// Hotbar slot
			ui_elements[2].x = ui_center_x - 182 + 22 + (40 * (hotbar_slot % 9));
		
			// Hotbar blocks
			for (uint8_t i = 0; i < MAX_CUBE_ELEMENTS; i++)
				cube_elements[i].pos.x = screen_center_x + 1 - hotbar_offset + ((20 * UI_SCALING) * i);
			break;
	}

	update_ui_buffer();

	matrix4_identity(ortho);
	matrix4_translate(ortho, -1.0f, -1.0f, 0.0f);
	matrix4_scale(ortho, UI_SCALING / settings.window_width, UI_SCALING / settings.window_height, 1.0f);
	update_cube_projection();
}

void cleanup_ui() {
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteBuffers(1, &cube_vbo);
	glDeleteBuffers(1, &cube_color_vbo);
	glDeleteBuffers(1, &cube_normal_vbo);
	glDeleteBuffers(1, &cube_ebo);
	glDeleteVertexArrays(1, &cube_vao);
	glDeleteProgram(ui_shader);
	glDeleteProgram(cube_shader);
}