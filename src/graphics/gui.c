#include "main.h"
#include "gui.h"
#include <math.h>
#include "config.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


uint8_t ui_state = 0;
uint8_t ui_active_2d_elements = 0;
uint8_t ui_active_3d_elements = 0;
static ui_vertex_t *vertex_buffer = NULL;
static uint16_t vertex_buffer_capacity = 0;
static unsigned int highlight_vbo = 0, highlight_vao = 0;
static unsigned int ui_vao, ui_vbo;
static float ortho[16];
static float cube_projection[16];
static float cube_view[16];
static float highlight_matrix[16];
static float cube_matrix[16];

ui_element_t* ui_elements = NULL;
static uint8_t ui_elements_capacity = 0;

static ui_batch_t *ui_batches = NULL;
uint8_t ui_batch_count = 0;
static uint8_t ui_batches_capacity = 0;

static cube_element_t cube_elements[MAX_CUBE_ELEMENTS];
static GLuint highlight_vao, highlight_vbo, highlight_ebo;
static GLuint cube_vao, cube_vbo, cube_ebo;
static uint16_t ui_center_x, ui_center_y;

static const float highlight_vertices[] = {
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

static const uint8_t highlight_indices[] = {
	// Front face (Z+)
	0, 1, 2, 3, 0,
	// Back face (Z-)
	4, 5, 6, 7, 4,
	// Left face (X-)
	8, 9, 10, 11, 8,
	// Right face (X+)
	12, 13, 14, 15, 12,
	// Bottom face (Y-)
	16, 17, 18, 19, 16,
	// Top face (Y+)
	20, 21, 22, 23, 20
};

static bool resize_ui_elements(uint8_t new_capacity) {
	ui_element_t *new_elements = realloc(ui_elements, sizeof(ui_element_t) * new_capacity);
	if (!new_elements) return false;
	
	ui_elements = new_elements;
	ui_elements_capacity = new_capacity;

	uint16_t vertices_needed = new_capacity * 4;
	ui_vertex_t *new_vertex_buffer = realloc(vertex_buffer, sizeof(ui_vertex_t) * vertices_needed);
	if (!new_vertex_buffer) return false;
	
	vertex_buffer = new_vertex_buffer;
	vertex_buffer_capacity = vertices_needed;
	return true;
}

static bool resize_ui_batches(uint8_t new_capacity) {
	ui_batch_t *new_batches = realloc(ui_batches, sizeof(ui_batch_t) * new_capacity);
	if (!new_batches) return false;
	
	ui_batches = new_batches;
	ui_batches_capacity = new_capacity;
	return true;
}

void clear_ui_elements() {
	ui_active_2d_elements = 0;
}

void clear_ui_batches() {
	ui_batch_count = 0;
}

bool add_ui_element(const ui_element_t *element) {
	if (ui_active_2d_elements >= ui_elements_capacity) {
		uint8_t new_capacity = ui_elements_capacity == 0 ? 16 : ui_elements_capacity * 2;
		if (!resize_ui_elements(new_capacity)) return false;
	}
	
	ui_elements[ui_active_2d_elements++] = *element;
	return true;
}

void remove_ui_element(uint8_t index) {
	if (index >= ui_active_2d_elements) return;
	
	memmove(&ui_elements[index], &ui_elements[index + 1], 
		   (ui_active_2d_elements - index - 1) * sizeof(ui_element_t));
	ui_active_2d_elements--;
}

void create_batches() {
	clear_ui_batches();
	if (ui_active_2d_elements == 0) return;

	if (ui_batches_capacity == 0 && !resize_ui_batches(8)) {
		return;
	}

	GLuint current_texture = ui_elements[0].texture_id;
	ui_batches[0] = (ui_batch_t){current_texture, 0, 1};
	ui_batch_count = 1;
	
	for (uint8_t i = 1; i < ui_active_2d_elements; i++) {
		if (ui_elements[i].texture_id == current_texture) {
			ui_batches[ui_batch_count-1].count++;
		} else {
			if (ui_batch_count >= ui_batches_capacity) {
				uint8_t new_capacity = ui_batches_capacity * 2;
				if (!resize_ui_batches(new_capacity)) break;
			}
			
			current_texture = ui_elements[i].texture_id;
			ui_batches[ui_batch_count++] = (ui_batch_t){current_texture, i, 1};
		}
	}
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
	
	glGenBuffers(1, &cube_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);

	// Set up vertex attributes
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);
	update_cube_projection();
}

void init_block_highlight() {
	glGenVertexArrays(1, &highlight_vao);
	glGenBuffers(1, &highlight_vbo);
	glGenBuffers(1, &highlight_ebo);

	glBindVertexArray(highlight_vao);
	glBindBuffer(GL_ARRAY_BUFFER, highlight_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(highlight_vertices), highlight_vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, highlight_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(highlight_indices), highlight_indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);
}

void init_ui() {
	init_block_highlight();
	init_cube_rendering();

	if (!resize_ui_elements(16)) {
		printf("Failed to initialize UI elements\n");
		return;
	}

	if (!resize_ui_batches(8)) {
		printf("Failed to initialize UI batches\n");
		return;
	}

	glGenVertexArrays(1, &ui_vao);
	glGenBuffers(1, &ui_vbo);
	glBindVertexArray(ui_vao);
	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ui_vertex_t), (void*)offsetof(ui_vertex_t, position));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(ui_vertex_t), (void*)offsetof(ui_vertex_t, tex_coord));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(ui_vertex_t), (void*)offsetof(ui_vertex_t, tex_id));
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
	glDrawElements(GL_LINE_STRIP, sizeof(highlight_indices), GL_UNSIGNED_BYTE, (void*)0);
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

	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, cube_projection);
	glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, cube_view);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, cube_matrix);

	FaceMesh faces[6] = {0};
	generate_single_block_mesh(0, 0, 0, cube->id, faces);

	glBindVertexArray(cube_vao);

	for (uint8_t face = 0; face < 6; face++) {
		if (faces[face].vertex_count == 0) continue;

		glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
		glBufferData(GL_ARRAY_BUFFER, faces[face].vertex_count * sizeof(Vertex), faces[face].vertices, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, faces[face].index_count * sizeof(uint32_t), faces[face].indices, GL_DYNAMIC_DRAW);

		glDrawElements(GL_TRIANGLES, faces[face].index_count, GL_UNSIGNED_INT, 0);

		free(faces[face].vertices);
		free(faces[face].indices);
	}
}

void update_ui_buffer() {
	if (ui_active_2d_elements == 0) return;
	
	for (uint8_t i = 0; i < ui_active_2d_elements; i++) {
		const ui_element_t* element = &ui_elements[i];
		float tx = element->tex_x / 256.0f;
		float ty = element->tex_y / 256.0f;
		float tw = element->tex_width / 256.0f;
		float th = element->tex_height / 256.0f;

		float x0 = floor(element->x - element->width) + 0.375f;
		float x1 = floor(element->x + element->width) + 0.375f;
		float y0 = floor(element->y - element->height) + 0.375f;
		float y1 = floor(element->y + element->height) + 0.375f;

		uint16_t vertex_offset = i * 4;

		// Set vertex data
		vertex_buffer[vertex_offset + 0] = (ui_vertex_t){{x0, y1}, {tx, ty}, element->texture_id};
		vertex_buffer[vertex_offset + 1] = (ui_vertex_t){{x0, y0}, {tx, ty + th}, element->texture_id};
		vertex_buffer[vertex_offset + 2] = (ui_vertex_t){{x1, y1}, {tx + tw, ty}, element->texture_id};
		vertex_buffer[vertex_offset + 3] = (ui_vertex_t){{x1, y0}, {tx + tw, ty + th}, element->texture_id};
	}

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ui_vertex_t) * ui_active_2d_elements * 4, 
				vertex_buffer, GL_DYNAMIC_DRAW);
}

void render_ui() {
	if (ui_active_2d_elements) {
		glUseProgram(ui_shader);
		glUniformMatrix4fv(ui_projection_uniform_location, 1, GL_FALSE, ortho);
		glBindVertexArray(ui_vao);
		
		for (uint8_t i = 0; i < ui_batch_count; i++) {
			const ui_batch_t *batch = &ui_batches[i];
			glBindTexture(GL_TEXTURE_2D, batch->texture_id);
			glDrawArrays(GL_TRIANGLE_STRIP, batch->start_index * 4, batch->count * 4);
			draw_calls++;
		}
	}

	if (ui_active_3d_elements) {
		glUseProgram(shaderProgram);
		glBindTexture(GL_TEXTURE_2D, block_textures);
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
		case UI_STATE_RUNNING: {
			uint16_t hotbar_offset = 80 * UI_SCALING;

			// Crosshair
			add_ui_element(&(ui_element_t) {
				.x = ui_center_x,
				.y = ui_center_y,
				.width = 16,
				.height = 16,
				.tex_x = 240,
				.tex_y = 0,
				.tex_width = 16,
				.tex_height = 16,
				.texture_id = ui_textures
			});

			// Hotbar
			add_ui_element(&(ui_element_t) {
				.x = ui_center_x,
				.y = 20,
				.width = 182,
				.height = 22,
				.tex_x = 0,
				.tex_y = 0,
				.tex_width = 182,
				.tex_height = 22,
				.texture_id = ui_textures
			});
		
			// Hotbar slot
			add_ui_element(&(ui_element_t) {
				.x = ui_center_x - 182 + 22 + (40 * (hotbar_slot % 9)),
				.y = 20,
				.width = 24,
				.height = 24,
				.tex_x = 0,
				.tex_y = 22,
				.tex_width = 24,
				.tex_height = 24,
				.texture_id = ui_textures
			});
		
			// Hotbar blocks
			const cube_element_t base_cube = {
				.pos.y = 6 * UI_SCALING,
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
				cube_elements[i].pos.x = screen_center_x + 1 - hotbar_offset + ((20 * UI_SCALING) * i) - (7 * UI_SCALING);
				ui_active_3d_elements = i;
			}

			char fps_text[23];
			snprintf(fps_text, sizeof(fps_text), "FPS: %1.2f, %1.3fms", framerate, frametime);
			draw_text(fps_text, 3, ui_center_y * 2 - 12);
			break;
		}

		case UI_STATE_PAUSED:
			// Resume button
			add_ui_element(&(ui_element_t) {
				.x = ui_center_x,
				.y = ui_center_y + 25,
				.width = 200,
				.height = 20,
				.tex_x = 0,
				.tex_y = 66,
				.tex_width = 200,
				.tex_height = 20,
				.texture_id = ui_textures
			});

			// Quit button
			add_ui_element(&(ui_element_t) {
				.x = ui_center_x,
				.y = ui_center_y - 25,
				.width = 200,
				.height = 20,
				.tex_x = 0,
				.tex_y = 66,
				.tex_width = 200,
				.tex_height = 20,
				.texture_id = ui_textures
			});

			char back_text[13];
			snprintf(back_text, sizeof(back_text), "Back to Game");
			draw_text(back_text, ui_center_x - (get_text_length(back_text) / 2), ui_center_y + 25);

			char quit_text[23];
			snprintf(quit_text, sizeof(quit_text), "Save and Quit to Title");
			draw_text(quit_text, ui_center_x - (get_text_length(quit_text) / 2), ui_center_y - 25);
			break;
	}

	update_ui_buffer();
	create_batches();

	matrix4_identity(ortho);
	matrix4_translate(ortho, -1.0f, -1.0f, 0.0f);
	matrix4_scale(ortho, UI_SCALING / settings.window_width, UI_SCALING / settings.window_height, 1.0f);
	update_cube_projection();
}

bool check_hit(uint16_t hit_x, uint16_t hit_y, uint8_t element_id) {
	if (element_id >= ui_active_2d_elements) return false;
	
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
	free(ui_elements);
	free(vertex_buffer);
	free(ui_batches);
	
	ui_elements = NULL;
	vertex_buffer = NULL;
	ui_batches = NULL;
	
	ui_elements_capacity = 0;
	vertex_buffer_capacity = 0;
	ui_batches_capacity = 0;
	ui_active_2d_elements = 0;
	ui_batch_count = 0;
	
	// Delete OpenGL resources
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteBuffers(1, &cube_vbo);
	glDeleteBuffers(1, &cube_ebo);
	glDeleteVertexArrays(1, &cube_vao);
	glDeleteProgram(ui_shader);
}