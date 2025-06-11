#include "main.h"
#include "gui.h"
#include <math.h>
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
	vec2 position;
	vec2 tex_coord;
	uint32_t tex_id;
} ui_vertex_t;

uint8_t ui_state = 0;
uint8_t ui_active_2d_elements = 0;
uint8_t ui_active_3d_elements = 0;
ui_vertex_t *vertex_buffer = NULL;
uint16_t vertex_buffer_capacity = 0;
unsigned int highlight_vbo = 0, highlight_vao = 0;
unsigned int ui_vao, ui_vbo;
float ortho[16];
float cube_projection[16];
float cube_view[16];
float highlight_matrix[16];
float cube_matrix[16];

// Dynamic UI elements
ui_element_t* ui_elements = NULL;
uint8_t ui_elements_capacity = 0;
float *vertices = NULL;
uint8_t vertices_capacity = 0;

cube_element_t cube_elements[MAX_CUBE_ELEMENTS];
GLuint highlight_vao, highlight_vbo, highlight_ebo;
GLuint cube_vao, cube_vbo, cube_ebo;
GLuint cube_color_vbo;
GLuint cube_normal_vbo;
GLuint cube_tex_id_vbo;
uint16_t ui_center_x, ui_center_y;
GLint proj_loc, view_loc, model_loc;

static const float cube_vertices[] = {
	// Front face (Z+)
	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	
	// Back face (Z-)
	-0.5f, -0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	
	// Left face (X-)
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f,
	
	// Right face (X+)
	 0.5f, -0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	
	// Bottom face (Y-)
	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f,
	
	// Top face (Y+)
	-0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f
};

static const uint8_t cube_indices[] = {
	// Front face
	0, 1, 2,   2, 3, 0,
	// Back face
	4, 5, 6,   6, 7, 4,
	// Left face
	8, 9, 10,  10, 11, 8,
	// Right face
	12, 13, 14, 14, 15, 12,
	// Bottom face
	16, 17, 18, 18, 19, 16,
	// Top face
	20, 21, 22, 22, 23, 20
};

static const float cube_tex_coords[] = {
	// Front face
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	
	// Back face
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	0.0f, 1.0f,
	
	// Left face
	1.0f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	
	// Right face
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	1.0f, 1.0f,
	
	// Bottom face
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f,
	
	// Top face
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f
};

static const float cube_normals[] = {
	// Front face (Z+)
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	0.0f, 0.0f, 1.0f,
	
	// Back face (Z-)
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	0.0f, 0.0f, -1.0f,
	
	// Left face (X-)
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	-1.0f, 0.0f, 0.0f,
	
	// Right face (X+)
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	
	// Bottom face (Y-)
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	0.0f, -1.0f, 0.0f,
	
	// Top face (Y+)
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
};

// Dynamic UI element management functions
bool resize_ui_elements(uint8_t new_capacity) {
	ui_element_t *new_elements = realloc(ui_elements, sizeof(ui_element_t) * new_capacity);
	if (!new_elements) {
		return false;
	}
	
	ui_elements = new_elements;
	ui_elements_capacity = new_capacity;

	// 4 vertices per UI element
	uint16_t vertices_needed = new_capacity * 4;
	ui_vertex_t *new_vertex_buffer = realloc(vertex_buffer, sizeof(ui_vertex_t) * vertices_needed);
	if (!new_vertex_buffer) {
		return false;
	}
	vertex_buffer = new_vertex_buffer;
	vertex_buffer_capacity = vertices_needed;

	return true;
}

void clear_ui_elements() {
	ui_active_2d_elements = 0;
}

bool add_ui_element(const ui_element_t *element) {
	if (ui_active_2d_elements >= ui_elements_capacity) {
		uint8_t new_capacity = ui_elements_capacity == 0 ? 16 : ui_elements_capacity * 2;
		if (!resize_ui_elements(new_capacity)) {
			return false;
		}
	}
	
	ui_elements[ui_active_2d_elements] = *element;
	ui_active_2d_elements++;
	return true;
}

void remove_ui_element(uint8_t index) {
	if (index >= ui_active_2d_elements) {
		return;
	}

	for (uint8_t i = index; i < ui_active_2d_elements - 1; i++) {
		ui_elements[i] = ui_elements[i + 1];
	}
	
	ui_active_2d_elements--;
}

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
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);

	glGenBuffers(1, &cube_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &cube_color_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_color_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_tex_coords), cube_tex_coords, GL_STATIC_DRAW);
	
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &cube_normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_normal_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);
	
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(2);

	glGenBuffers(1, &cube_tex_id_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_tex_id_vbo);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(uint8_t), (void*)0);
	glEnableVertexAttribArray(3);

	glGenBuffers(1, &cube_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	glBindVertexArray(0);

	proj_loc = glGetUniformLocation(cube_shader, "projection");
	view_loc = glGetUniformLocation(cube_shader, "view");
	model_loc = glGetUniformLocation(cube_shader, "model");

	update_cube_projection();
}

void init_block_highlight() {
	glGenVertexArrays(1, &highlight_vao);
	glGenBuffers(1, &highlight_vbo);
	glGenBuffers(1, &highlight_ebo);

	glBindVertexArray(highlight_vao);

	glBindBuffer(GL_ARRAY_BUFFER, highlight_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, highlight_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);
}

void init_ui() {
	init_block_highlight();
	init_cube_rendering();

	// Initialize with a small capacity
	if (!resize_ui_elements(16)) {
		printf("Failed to initialize UI elements\n");
		return;
	}

	glGenVertexArrays(1, &ui_vao);
	glGenBuffers(1, &ui_vbo);
	glBindVertexArray(ui_vao);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);

	// Position attribute (location 0)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 
		sizeof(ui_vertex_t), (void*)offsetof(ui_vertex_t, position));
	glEnableVertexAttribArray(0);

	// Texture coordinate attribute (location 1)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
		sizeof(ui_vertex_t), (void*)offsetof(ui_vertex_t, tex_coord));
	glEnableVertexAttribArray(1);

	// Texture ID attribute (location 2)
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, 
		sizeof(ui_vertex_t), (void*)offsetof(ui_vertex_t, tex_id));
	glEnableVertexAttribArray(2);
}

void draw_block_highlight(vec3 pos) {
	matrix4_identity(highlight_matrix);
	matrix4_translate(highlight_matrix, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
	matrix4_scale(highlight_matrix, 1.001, 1.001, 1.001);

	glUseProgram(shaderProgram);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, highlight_matrix);
	glUniform1i(atlas_uniform_location, 1);

	glLineWidth(2.0f);
	glBindVertexArray(highlight_vao);
	glDrawElements(GL_LINE_STRIP, sizeof(cube_indices), GL_UNSIGNED_BYTE, (void*)0);
	draw_calls++;
}

void draw_cube_element(const cube_element_t* cube) {
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

	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, cube_projection);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, cube_view);
	glUniformMatrix4fv(model_loc, 1, GL_FALSE, cube_matrix);

	uint8_t block_type = cube->id;
	uint8_t face_tex_ids[24]; // 4 vertices per face, 6 faces
	
	for (uint8_t face = 0; face < 6; face++) {
		uint8_t tex_id = block_data[block_type][2 + face];
		for (uint8_t vertex = 0; vertex < 4; vertex++) {
			face_tex_ids[face * 4 + vertex] = tex_id;
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, cube_tex_id_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(face_tex_ids), face_tex_ids, GL_DYNAMIC_DRAW);

	glBindTexture(GL_TEXTURE_2D, block_textures);
	glBindVertexArray(cube_vao);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, 0);
}

void update_ui_buffer() {
	if (ui_active_2d_elements == 0) {
		return;
	}
	
	for (uint8_t i = 0; i < ui_active_2d_elements; i++) {
		ui_element_t* element = &ui_elements[i];
		float tx = (float)element->tex_x / 256.0f;
		float ty = (float)element->tex_y / 256.0f;
		float tw = (float)element->tex_width / 256.0f;
		float th = (float)element->tex_height / 256.0f;

		float x0 = floor(element->x - element->width) + 0.375f;
		float x1 = floor(element->x + element->width) + 0.375f;
		float y0 = floor(element->y - element->height) + 0.375f;
		float y1 = floor(element->y + element->height) + 0.375f;

		uint16_t vertex_offset = i * 4;

		// Top-left vertex
		vertex_buffer[vertex_offset + 0].position.x = x0;
		vertex_buffer[vertex_offset + 0].position.y = y1;
		vertex_buffer[vertex_offset + 0].tex_coord.x = tx;
		vertex_buffer[vertex_offset + 0].tex_coord.y = ty;
		vertex_buffer[vertex_offset + 0].tex_id = element->texture_id;

		// Bottom-left vertex
		vertex_buffer[vertex_offset + 1].position.x = x0;
		vertex_buffer[vertex_offset + 1].position.y = y0;
		vertex_buffer[vertex_offset + 1].tex_coord.x = tx;
		vertex_buffer[vertex_offset + 1].tex_coord.y = ty + th;
		vertex_buffer[vertex_offset + 1].tex_id = element->texture_id;

		// Top-right vertex
		vertex_buffer[vertex_offset + 2].position.x = x1;
		vertex_buffer[vertex_offset + 2].position.y = y1;
		vertex_buffer[vertex_offset + 2].tex_coord.x = tx + tw;
		vertex_buffer[vertex_offset + 2].tex_coord.y = ty;
		vertex_buffer[vertex_offset + 2].tex_id = element->texture_id;

		// Bottom-right vertex
		vertex_buffer[vertex_offset + 3].position.x = x1;
		vertex_buffer[vertex_offset + 3].position.y = y0;
		vertex_buffer[vertex_offset + 3].tex_coord.x = tx + tw;
		vertex_buffer[vertex_offset + 3].tex_coord.y = ty + th;
		vertex_buffer[vertex_offset + 3].tex_id = element->texture_id;
	}

	// Upload buffer data
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, 
		sizeof(ui_vertex_t) * ui_active_2d_elements * 4, 
		vertex_buffer, GL_DYNAMIC_DRAW);
}

void render_ui() {
	if (ui_active_2d_elements) {
		glUseProgram(ui_shader);
		glUniformMatrix4fv(ui_projection_uniform_location, 1, GL_FALSE, ortho);
		glBindVertexArray(ui_vao);
		
		GLuint current_texture = 0;
		
		for (uint8_t i = 0; i < ui_active_2d_elements; i++) {
			const ui_element_t* element = &ui_elements[i];

			// Only bind texture if it's different from the current one
			if (element->texture_id != current_texture) {
				glBindTexture(GL_TEXTURE_2D, element->texture_id);
				current_texture = element->texture_id;
			}

			// Draw this element (4 vertices per element)
			glDrawArrays(GL_TRIANGLE_STRIP, i * 4, 4);
			draw_calls++;
		}
	}

	if (ui_active_3d_elements) {
		glUseProgram(cube_shader);

		// Hotbar blocks
		for (uint8_t i = 0; i < MAX_CUBE_ELEMENTS; i++) {
			draw_cube_element(&cube_elements[i]);
			draw_calls++;
		}
	}
}

void update_ui() {
	ui_center_x = settings.window_width / UI_SCALING;
	ui_center_y = settings.window_height / UI_SCALING;
	clear_ui_elements();
	ui_active_3d_elements = 0;
	
	switch (ui_state) {
		case UI_STATE_RUNNING: // Game view
			uint16_t hotbar_offset = 80 * UI_SCALING;

			// Crosshair
			ui_element_t crosshair = {
				.x = ui_center_x,
				.y = ui_center_y,
				.width = 16,
				.height = 16,
				.tex_x = 240,
				.tex_y = 0,
				.tex_width = 16,
				.tex_height = 16,
				.texture_id = ui_textures
			};
			add_ui_element(&crosshair);

			// Hotbar
			ui_element_t hotbar = {
				.x = ui_center_x,
				.y = 20,
				.width = 182,
				.height = 22,
				.tex_x = 0,
				.tex_y = 0,
				.tex_width = 182,
				.tex_height = 22,
				.texture_id = ui_textures
			};
			add_ui_element(&hotbar);
		
			// Hotbar slot
			ui_element_t hotbar_slot_active = {
				.x = ui_center_x - 182 + 22 + (40 * (hotbar_slot % 9)),
				.y = 20,
				.width = 24,
				.height = 24,
				.tex_x = 0,
				.tex_y = 22,
				.tex_width = 24,
				.tex_height = 24,
				.texture_id = ui_textures
			};
			add_ui_element(&hotbar_slot_active);
		
			// Hotbar blocks
			const cube_element_t base_cube = {
				.pos.y = 10.333f * UI_SCALING,
				.width = 10.05 * UI_SCALING,
				.height = 10.05 * UI_SCALING,
				.depth = 10.05 * UI_SCALING,
				.rotation_x = -30 * (M_PI / 180.0f),
				.rotation_y = 45 * (M_PI / 180.0f),
				.id = 0
			};

			for (uint8_t i = 0; i < MAX_CUBE_ELEMENTS; i++) {
				cube_elements[i] = base_cube;
				cube_elements[i].id = i + 1;
				cube_elements[i].pos.x = screen_center_x + 1 - hotbar_offset + ((20 * UI_SCALING) * i);
				ui_active_3d_elements = i;
			}

			char fps_text[22];
			snprintf(fps_text, sizeof(fps_text), "FPS: %1.2f, %1.3f ms", framerate, frametime);
			draw_text(fps_text, 3, ui_center_y * 2 - 12);
			break;

		case UI_STATE_PAUSED:
			// Resume button
			ui_element_t resume_button = {
				.x = ui_center_x,
				.y = ui_center_y + 25,
				.width = 200,
				.height = 20,
				.tex_x = 0,
				.tex_y = 66,
				.tex_width = 200,
				.tex_height = 20,
				.texture_id = ui_textures
			};
			add_ui_element(&resume_button);

			// Quit button
			ui_element_t quit_button = {
				.x = ui_center_x,
				.y = ui_center_y - 25,
				.width = 200,
				.height = 20,
				.tex_x = 0,
				.tex_y = 66,
				.tex_width = 200,
				.tex_height = 20,
				.texture_id = ui_textures
			};
			add_ui_element(&quit_button);

			char back_text[13];
			snprintf(back_text, sizeof(back_text), "Back to Game");
			draw_text(back_text, ui_center_x - (get_text_length(back_text) / 2), ui_center_y + 25);

			char quit_text[23];
			snprintf(quit_text, sizeof(quit_text), "Save and Quit to Title");
			draw_text(quit_text, ui_center_x - (get_text_length(quit_text) / 2), ui_center_y - 25);
			break;
	}

	update_ui_buffer();

	matrix4_identity(ortho);
	matrix4_translate(ortho, -1.0f, -1.0f, 0.0f);
	matrix4_scale(ortho, UI_SCALING / settings.window_width, UI_SCALING / settings.window_height, 1.0f);
	update_cube_projection();
}

bool check_hit(uint16_t hit_x, uint16_t hit_y, uint8_t element_id) {
	if (element_id >= ui_active_2d_elements) {
		return false;
	}
	
	hit_y = settings.window_height - hit_y;

	const ui_element_t *element = &ui_elements[element_id];

	uint16_t scaled_half_width = (uint16_t)(element->width * UI_SCALING) / 2;
	uint16_t scaled_half_height = (uint16_t)(element->height * UI_SCALING) / 2;
	uint16_t center_x = (uint16_t)(element->x * UI_SCALING) / 2;
	uint16_t center_y = (uint16_t)(element->y * UI_SCALING) / 2;

	return (hit_x >= center_x - scaled_half_width &&
		hit_x <= center_x + scaled_half_width &&
		hit_y >= center_y - scaled_half_height &&
		hit_y <= center_y + scaled_half_height);
}

void cleanup_ui() {
	// Free dynamic arrays
	if (ui_elements) {
		free(ui_elements);
		ui_elements = NULL;
	}
	if (vertex_buffer) {
		free(vertex_buffer);
		vertex_buffer = NULL;
	}
	ui_elements_capacity = 0;
	vertex_buffer_capacity = 0;
	ui_active_2d_elements = 0;
	
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteBuffers(1, &cube_vbo);
	glDeleteBuffers(1, &cube_color_vbo);
	glDeleteBuffers(1, &cube_normal_vbo);
	glDeleteBuffers(1, &cube_tex_id_vbo);
	glDeleteBuffers(1, &cube_ebo);
	glDeleteVertexArrays(1, &cube_vao);
	glDeleteProgram(ui_shader);
	glDeleteProgram(cube_shader);
}