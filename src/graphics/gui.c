#include "main.h"
#include "gui.h"
#include <math.h>
#include "config.h"
#include "textures.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

uint8_t ui_state = 0;
uint8_t ui_active_2d_elements = 0;
uint8_t ui_active_3d_elements = 0;
static ui_vertex_t *vertex_buffer = NULL;
static uint16_t vertex_buffer_capacity = 0;
static unsigned int highlight_vao, highlight_vbo, highlight_ebo;
static unsigned int cube_vao, cube_vbo, cube_ebo;
static unsigned int ui_vao, ui_vbo;
static mat4 ortho;
static mat4 cube_projection;
static mat4 cube_view;
static mat4 highlight_matrix;
static mat4 cube_matrix;
static uint16_t ui_center_x, ui_center_y;

ui_element_t* ui_elements = NULL;
static uint8_t ui_elements_capacity = 0;
static ui_batch_t *ui_batches = NULL;
uint8_t ui_batch_count = 0;
static uint8_t ui_batches_capacity = 0;
static cube_element_t cube_elements[MAX_CUBE_ELEMENTS];

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

	uint16_t vertices_needed = new_capacity * 6;
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
	float right = settings.window_width;
	float top = settings.window_height;
	
	matrix4_identity(cube_projection);
	cube_projection[0] = 2.0f / right;
	cube_projection[5] = 2.0f / top;
	cube_projection[10] = 0;
	cube_projection[12] = -right / right;
	cube_projection[13] = -top / top;
	cube_projection[14] = 0;
	
	matrix4_identity(cube_view);
}

void init_highlight() {
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

void init_cube_rendering() {
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);

	glGenBuffers(1, &cube_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
	
	glGenBuffers(1, &cube_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, texture_id));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, size_u));
	glEnableVertexAttribArray(3);
	glBindVertexArray(0);
}

void init_ui_rendering() {
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

void init_ui() {
	init_highlight();
	init_cube_rendering();
	init_ui_rendering();

	if (!resize_ui_elements(16)) {
		printf("Failed to initialize UI elements\n");
		return;
	}

	if (!resize_ui_batches(8)) {
		printf("Failed to initialize UI batches\n");
		return;
	}
}

void draw_block_highlight(vec3 pos) {
	matrix4_identity(highlight_matrix);
	matrix4_translate(highlight_matrix, pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
	matrix4_scale(highlight_matrix, 1.001, 1.001, 1.001);

	glUseProgram(world_shader);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, highlight_matrix);

	glLineWidth(2.0f);
	glBindVertexArray(highlight_vao);
	glDrawElements(GL_LINE_STRIP, sizeof(highlight_indices), GL_UNSIGNED_BYTE, (void*)0);
	draw_calls++;
}

void draw_cube_element(const cube_element_t* cube) {
	matrix4_identity(cube_matrix);

	mat4 temp, rotation;

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

		float x0 = element->x;
		float x1 = element->x + element->width;
		float y0 = element->y;
		float y1 = element->y + element->height;

		uint16_t vertex_offset = i * 6;

		vertex_buffer[vertex_offset + 0] = (ui_vertex_t){{x0, y0}, {tx, ty + th}, element->texture_id};
		vertex_buffer[vertex_offset + 1] = (ui_vertex_t){{x1, y1}, {tx + tw, ty}, element->texture_id};
		vertex_buffer[vertex_offset + 2] = (ui_vertex_t){{x0, y1}, {tx, ty}, element->texture_id};

		vertex_buffer[vertex_offset + 3] = (ui_vertex_t){{x0, y0}, {tx, ty + th}, element->texture_id};
		vertex_buffer[vertex_offset + 4] = (ui_vertex_t){{x1, y0}, {tx + tw, ty + th}, element->texture_id};
		vertex_buffer[vertex_offset + 5] = (ui_vertex_t){{x1, y1}, {tx + tw, ty}, element->texture_id};
	}

	glBindBuffer(GL_ARRAY_BUFFER, ui_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(ui_vertex_t) * ui_active_2d_elements * 6, 
				vertex_buffer, GL_DYNAMIC_DRAW);
}

void draw_item(uint8_t id, vec2 pos) {
	const cube_element_t base_cube = {
		.width = 10 * UI_SCALING,
		.height = 10 * UI_SCALING,
		.depth = 10 * UI_SCALING,
		.rotation_x = -30 * DEG_TO_RAD,
		.rotation_y = 45 * DEG_TO_RAD,
		.id = 0
	};

	// 2D blocks
	if (block_data[id][0] == 2) {
		add_ui_element(&(ui_element_t) {
			.x = pos.x,
			.y = pos.y,
			.width = 16 * UI_SCALING,
			.height = 16 * UI_SCALING,
			.tex_x = 240,
			.tex_y = 0,
			.tex_width = 16,
			.tex_height = 16,
			.texture_id = block_textures
		});
	}

	// Anything else
	else {
		cube_elements[ui_active_3d_elements] = base_cube;
		cube_elements[ui_active_3d_elements].id = id;
		cube_elements[ui_active_3d_elements].pos.x = pos.x + (1 * UI_SCALING);
		cube_elements[ui_active_3d_elements].pos.y = pos.y + (4 * UI_SCALING) - 1;
		ui_active_3d_elements++;
	}
}

void render_ui() {
	// Held block (Has to take priority over 2D elements and other blocks)
	if (ui_active_3d_elements) {
		glUseProgram(world_shader);
		glBindTexture(GL_TEXTURE_2D, block_textures);

		mat4 perspective_proj;
		matrix4_identity(perspective_proj);
		matrix4_perspective(perspective_proj, 45.0f * DEG_TO_RAD, aspect, 0.1f, 100.0f);

		mat4 view;
		matrix4_identity(view);
		matrix4_translate(view, 0, 0, -3.5f);

		glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, perspective_proj);
		glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, view);
		draw_cube_element(&cube_elements[ui_active_3d_elements - 1]);
		draw_calls++;
	}

	if (ui_active_2d_elements) {
		glUseProgram(ui_shader);
		glUniformMatrix4fv(ui_projection_uniform_location, 1, GL_FALSE, ortho);
		glBindVertexArray(ui_vao);
		
		for (uint8_t i = 0; i < ui_batch_count; i++) {
			const ui_batch_t *batch = &ui_batches[i];
			glBindTexture(GL_TEXTURE_2D, batch->texture_id);
			glDrawArrays(GL_TRIANGLES, batch->start_index * 6, batch->count * 6);
			draw_calls++;
		}
	}

	if (ui_active_3d_elements) {
		glUseProgram(world_shader);
		glBindTexture(GL_TEXTURE_2D, block_textures);

		glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, cube_projection);
		glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, cube_view);
		for (uint8_t i = 0; i < ui_active_3d_elements - 1; i++) {
			draw_cube_element(&cube_elements[i]);
			draw_calls++;
		}
	}
}

void update_ui() {
	ui_center_x = settings.window_width / 2;
	ui_center_y = settings.window_height / 2;
	clear_ui_elements();
	ui_active_3d_elements = 0;
	
	switch (ui_state) {
		case UI_STATE_RUNNING: {
			// Crosshair
			add_ui_element(&(ui_element_t) {
				.x = screen_center_x - (8 * UI_SCALING),
				.y = screen_center_y - (8 * UI_SCALING),
				.width = 16 * UI_SCALING,
				.height = 16 * UI_SCALING,
				.tex_x = 240,
				.tex_y = 0,
				.tex_width = 16,
				.tex_height = 16,
				.texture_id = ui_textures
			});

			// Hotbar
			add_ui_element(&(ui_element_t) {
				.x = screen_center_x - ((182 * UI_SCALING) / 2),
				.y = 0,
				.width = 182 * UI_SCALING,
				.height = 22 * UI_SCALING,
				.tex_x = 0,
				.tex_y = 0,
				.tex_width = 182,
				.tex_height = 22,
				.texture_id = ui_textures
			});
		
			// Hotbar slot
			add_ui_element(&(ui_element_t) {
				.x = screen_center_x - ((182 * UI_SCALING) / 2) - (1 * UI_SCALING) + ((20 * (hotbar_slot % 9)) * UI_SCALING),
				.y = 0 - (1 * UI_SCALING),
				.width = 24 * UI_SCALING,
				.height = 24 * UI_SCALING,
				.tex_x = 0,
				.tex_y = 22,
				.tex_width = 24,
				.tex_height = 24,
				.texture_id = ui_textures
			});

			// Hotbar blocks
			for (uint8_t i = 0; i < MAX_CUBE_ELEMENTS - 1; i++) {
				uint8_t block_id = i + 1 + (floor(hotbar_slot / 9) * 9);
				vec2 pos = {
					.x = screen_center_x - ((182 * UI_SCALING) / 2) + ( i * 20 * UI_SCALING) + (3 * UI_SCALING),
					.y = 3 * UI_SCALING,
				};
				draw_item(block_id, pos);
			}

			// Selected block
			if (ui_active_3d_elements < MAX_CUBE_ELEMENTS) {
				cube_elements[ui_active_3d_elements] = (cube_element_t){
					.pos.x = 1.15,
					.pos.y = -1.65,
					.width = 1,
					.height = 1,
					.depth = 1,
					.rotation_x = -10 * DEG_TO_RAD,
					.rotation_y = -30 * DEG_TO_RAD,
					.rotation_z = -2.5 * DEG_TO_RAD,
					.id = hotbar_slot + 1
				};
				ui_active_3d_elements++;
			}

			char fps_text[23];
			snprintf(fps_text, sizeof(fps_text), "FPS: %1.2f, %1.3fms", framerate, frametime);
			draw_text(fps_text, 3, settings.window_height - ((8 + 3) * UI_SCALING));
			break;
		}

		case UI_STATE_PAUSED:
			// Resume button
			add_ui_element(&(ui_element_t) {
				.x = screen_center_x - ((200 * UI_SCALING) / 2),
				.y = screen_center_y + (5 * UI_SCALING),
				.width = 200 * UI_SCALING,
				.height = 20 * UI_SCALING,
				.tex_x = 0,
				.tex_y = 66,
				.tex_width = 200,
				.tex_height = 20,
				.texture_id = ui_textures
			});

			// Quit button
			add_ui_element(&(ui_element_t) {
				.x = screen_center_x - ((200 * UI_SCALING) / 2),
				.y = screen_center_y - ((20 + 5) * UI_SCALING),
				.width = 200 * UI_SCALING,
				.height = 20 * UI_SCALING,
				.tex_x = 0,
				.tex_y = 66,
				.tex_width = 200,
				.tex_height = 20,
				.texture_id = ui_textures
			});

			char back_text[13];
			snprintf(back_text, sizeof(back_text), "Back to Game");
			draw_text(back_text, screen_center_x - (get_text_length(back_text) / 2), screen_center_y + (11 * UI_SCALING));

			char quit_text[23];
			snprintf(quit_text, sizeof(quit_text), "Save and Quit to Title");
			draw_text(quit_text, screen_center_x - (get_text_length(quit_text) / 2), screen_center_y - (19 * UI_SCALING));
			break;
	}

	update_ui_buffer();
	create_batches();

	matrix4_identity(ortho);
	matrix4_ortho(ortho, 0, settings.window_width, 0, settings.window_height, -1, 1);
	update_cube_projection();
}

bool check_hit(uint16_t hit_x, uint16_t hit_y, uint8_t element_id) {
	hit_y = settings.window_height - hit_y;
	ui_element_t *element = &ui_elements[element_id];

	bool in_range_x = IN_RANGE(hit_x, element->x, element->x + element->width);
	bool in_range_y = IN_RANGE(hit_y, element->y, element->y + element->height);

	return in_range_x && in_range_y;
}

void cleanup_ui() {
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
	
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteBuffers(1, &cube_vbo);
	glDeleteBuffers(1, &cube_ebo);
	glDeleteVertexArrays(1, &cube_vao);
	glDeleteProgram(ui_shader);
}