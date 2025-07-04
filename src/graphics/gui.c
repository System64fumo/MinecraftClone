#include "main.h"
#include "gui.h"
#include "shaders.h"
#include "config.h"
#include "textures.h"
#include "views.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

uint8_t ui_state = 0;
uint8_t ui_active_2d_elements = 0;
uint8_t ui_active_3d_elements = 0;
static ui_vertex_t *vertex_buffer = NULL;
static uint16_t vertex_buffer_capacity = 0;
static unsigned int cube_vao, cube_vbo, cube_ebo;
static unsigned int ui_vao, ui_vbo;
static mat4 ortho, cube_projection, cube_view, cube_matrix;
static mat4 highlight_matrix;
static uint16_t ui_center_x, ui_center_y;

ui_element_t* ui_elements = NULL;
cube_element_t* cube_elements = NULL;
static uint8_t ui_elements_capacity = 0;
static ui_batch_t *ui_batches = NULL;
uint8_t ui_batch_count = 0;
static uint8_t ui_batches_capacity = 0;
static uint8_t cube_elements_capacity = 0;

static uint8_t cached_highlight_block_id = 0;
static Mesh cached_highlight_faces[6] = {0};

static bool resize_ui_elements(uint8_t new_capacity) {
	if (new_capacity == 0) return false;
	
	ui_element_t *new_elements = realloc(ui_elements, sizeof(ui_element_t) * new_capacity);
	if (!new_elements) {
		return false;
	}
	ui_elements = new_elements;
	ui_elements_capacity = new_capacity;

	uint16_t vertices_needed = new_capacity * 6;
	ui_vertex_t *new_vertex_buffer = realloc(vertex_buffer, sizeof(ui_vertex_t) * vertices_needed);
	if (!new_vertex_buffer) {
		return false;
	}
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

static bool resize_cube_elements(uint8_t new_capacity) {
	cube_element_t *new_elements = realloc(cube_elements, sizeof(cube_element_t) * new_capacity);
	if (!new_elements) return false;
	cube_elements = new_elements;
	cube_elements_capacity = new_capacity;
	return true;
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

bool add_cube_element(const cube_element_t *element) {
	if (ui_active_3d_elements >= cube_elements_capacity) {
		uint8_t new_capacity = cube_elements_capacity == 0 ? 16 : cube_elements_capacity * 2;
		if (!resize_cube_elements(new_capacity)) return false;
	}
	cube_elements[ui_active_3d_elements++] = *element;
	return true;
}

void remove_cube_element(uint8_t index) {
	if (index >= ui_active_3d_elements) return;
	memmove(&cube_elements[index], &cube_elements[index + 1],
		   (ui_active_3d_elements - index - 1) * sizeof(cube_element_t));
	ui_active_3d_elements--;
}

void create_batches() {
	ui_batch_count = 0;
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

void init_cube_rendering() {
	glGenVertexArrays(1, &cube_vao);
	glBindVertexArray(cube_vao);

	glGenBuffers(1, &cube_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
	
	glGenBuffers(1, &cube_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_SHORT, sizeof(Vertex), (void*)offsetof(Vertex, packed_data));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_UNSIGNED_INT, sizeof(Vertex), (void*)offsetof(Vertex, packed_size));
	glEnableVertexAttribArray(2);
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
	init_cube_rendering();
	init_ui_rendering();

	if (!resize_ui_elements(16)) return;
	if (!resize_ui_batches(8)) return;
}

void free_cached_highlight() {
	for (uint8_t face = 0; face < 6; face++) {
		if (cached_highlight_faces[face].vertices) free(cached_highlight_faces[face].vertices);
		if (cached_highlight_faces[face].indices) free(cached_highlight_faces[face].indices);
		cached_highlight_faces[face].vertices = NULL;
		cached_highlight_faces[face].indices = NULL;
		cached_highlight_faces[face].vertex_count = 0;
		cached_highlight_faces[face].index_count = 0;
	}
	cached_highlight_block_id = 0xFF;
}

void draw_block_highlight(vec3 pos, uint8_t block_id) {
	if (block_id != cached_highlight_block_id) {
		free_cached_highlight();
		generate_single_block_mesh(0, 0, 0, block_id, cached_highlight_faces);
		cached_highlight_block_id = block_id;
	}

	matrix4_identity(highlight_matrix);
	matrix4_translate(highlight_matrix, pos.x, pos.y, pos.z);
	matrix4_scale(highlight_matrix, 1.005f, 1.005f, 1.005f);

	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, highlight_matrix);
	glUniform1i(highlight_uniform_location, 1);
	glBindVertexArray(cube_vao);

	uint8_t block_type = block_data[block_id][0];
	size_t total_line_vertices = 0;

	if (block_type == BTYPE_CROSS) {
		total_line_vertices = 16;
	} else {
		for (uint8_t face = 0; face < 6; face++) {
			if (cached_highlight_faces[face].vertex_count == 0) continue;
			total_line_vertices += cached_highlight_faces[face].vertex_count * 2;
		}
	}

	if (total_line_vertices > 0) {
		Vertex* line_vertices = malloc(total_line_vertices * sizeof(Vertex));
		size_t vertex_offset = 0;
		if (block_type == BTYPE_CROSS) {
			for (uint8_t face = 0; face < 4; face++) {
				if (cached_highlight_faces[face].vertex_count == 0) continue;
				for (uint8_t i = 0; i < 4; i++) {
					uint8_t next_i = (i + 1) % 4;
					memcpy(&line_vertices[vertex_offset++], &cached_highlight_faces[face].vertices[i], sizeof(Vertex));
					memcpy(&line_vertices[vertex_offset++], &cached_highlight_faces[face].vertices[next_i], sizeof(Vertex));
				}
				uint8_t next_face = (face + 1) % 4;
				if (cached_highlight_faces[next_face].vertex_count > 0) {
					for (uint8_t i = 0; i < 4; i++) {
						uint8_t next_i = (i + 1) % 4;
						memcpy(&line_vertices[vertex_offset++], &cached_highlight_faces[next_face].vertices[i], sizeof(Vertex));
						memcpy(&line_vertices[vertex_offset++], &cached_highlight_faces[next_face].vertices[next_i], sizeof(Vertex));
					}
				}
				break;
			}
		} else {
			for (uint8_t face = 0; face < 6; face++) {
				if (cached_highlight_faces[face].vertex_count == 0) continue;
				for (uint8_t i = 0; i < 4; i++) {
					uint8_t next_i = (i + 1) % 4;
					memcpy(&line_vertices[vertex_offset++], &cached_highlight_faces[face].vertices[i], sizeof(Vertex));
					memcpy(&line_vertices[vertex_offset++], &cached_highlight_faces[face].vertices[next_i], sizeof(Vertex));
				}
			}
		}
		glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
		glBufferData(GL_ARRAY_BUFFER, total_line_vertices * sizeof(Vertex), line_vertices, GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, total_line_vertices);
		draw_calls++;
		free(line_vertices);
	}
	glUniform1i(highlight_uniform_location, 0);
}

void draw_cube_element(const cube_element_t* cube) {
	matrix4_identity(cube_matrix);

	mat4 temp, rotation;

	matrix4_rotate_z(rotation, cube->rotation_z);
	memcpy(temp, cube_matrix, sizeof(float) * 16);
	matrix4_multiply(cube_matrix, temp, rotation);

	matrix4_rotate_y(rotation, cube->rotation_y);
	memcpy(temp, cube_matrix, sizeof(float) * 16);
	matrix4_multiply(cube_matrix, temp, rotation);

	matrix4_rotate_x(rotation, cube->rotation_x);
	memcpy(temp, cube_matrix, sizeof(float) * 16);
	matrix4_multiply(cube_matrix, temp, rotation);

	matrix4_translate(cube_matrix, cube->pos.x, cube->pos.y, 0);
	matrix4_scale(cube_matrix, cube->width, cube->height, cube->depth);

	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, cube_matrix);

	Mesh faces[6] = {0};
	generate_single_block_mesh(0, 0, 0, cube->id, faces);

	glBindVertexArray(cube_vao);
	glBindBuffer(GL_ARRAY_BUFFER, cube_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_ebo);

	size_t total_vertices = 0;
	size_t total_indices = 0;

	for (uint8_t face = 0; face < 6; face++) {
		if (faces[face].vertex_count == 0) continue;
		total_vertices += faces[face].vertex_count;
		total_indices += faces[face].index_count;
	}

	Vertex* all_vertices = malloc(total_vertices * sizeof(Vertex));
	uint32_t* all_indices = malloc(total_indices * sizeof(uint32_t));

	size_t vertex_offset = 0;
	size_t index_offset = 0;
	uint32_t base_index = 0;
	
	for (uint8_t face = 0; face < 6; face++) {
		if (faces[face].vertex_count == 0) continue;
		
		memcpy(all_vertices + vertex_offset, faces[face].vertices, faces[face].vertex_count * sizeof(Vertex));

		for (size_t i = 0; i < faces[face].index_count; i++) {
			all_indices[index_offset + i] = faces[face].indices[i] + base_index;
		}
		
		vertex_offset += faces[face].vertex_count;
		index_offset += faces[face].index_count;
		base_index += faces[face].vertex_count;

		free(faces[face].vertices);
		free(faces[face].indices);
	}

	glBufferData(GL_ARRAY_BUFFER, total_vertices * sizeof(Vertex), all_vertices, GL_DYNAMIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_indices * sizeof(uint32_t), all_indices, GL_DYNAMIC_DRAW);
	glDrawElements(GL_TRIANGLES, total_indices, GL_UNSIGNED_INT, 0);
	draw_calls++;

	free(all_vertices);
	free(all_indices);
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
		.width = 10 * settings.gui_scale,
		.height = 10 * settings.gui_scale,
		.depth = 10 * settings.gui_scale,
		.rotation_x = -30 * DEG_TO_RAD,
		.rotation_y = 45 * DEG_TO_RAD,
		.id = 0
	};

	// 2D blocks
	if (block_data[id][0] == BTYPE_CROSS) {
		uint8_t textureIndex = block_data[id][2] - 1u;
		uint8_t xOffset = textureIndex % 16;
		uint8_t yOffset = textureIndex / 16;

		add_ui_element(&(ui_element_t) {
			.x = pos.x,
			.y = pos.y,
			.width = 16 * settings.gui_scale,
			.height = 16 * settings.gui_scale,
			.tex_x = xOffset * 16,
			.tex_y = yOffset * 16,
			.tex_width = 16,
			.tex_height = 16,
			.texture_id = block_textures
		});
	}

	// Anything else
	else {
		cube_element_t cube = base_cube;
		cube.id = id;
		cube.pos.x = pos.x + (1 * settings.gui_scale);
		cube.pos.y = pos.y + (4 * settings.gui_scale) - 1;
		add_cube_element(&cube);
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

	if (ui_active_3d_elements > 1) {
		glUseProgram(world_shader);
		glBindTexture(GL_TEXTURE_2D, block_textures);
		glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, cube_projection);
		glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, cube_view);
		for (uint8_t i = 0; i < ui_active_3d_elements - 1; i++) {
			draw_cube_element(&cube_elements[i]);
		}
	}
}

void update_ui() {
	ui_center_x = settings.window_width / 2;
	ui_center_y = settings.window_height / 2;
	ui_active_2d_elements = 0;
	ui_active_3d_elements = 0;
	
	switch (ui_state) {
		case UI_STATE_RUNNING: {
			view_game_init();
			break;
		}

		case UI_STATE_PAUSED:
			view_pause_init();
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
	free(cube_elements);
	free_cached_highlight();
	ui_elements = NULL;
	vertex_buffer = NULL;
	ui_batches = NULL;
	cube_elements = NULL;
	ui_elements_capacity = 0;
	vertex_buffer_capacity = 0;
	ui_batches_capacity = 0;
	cube_elements_capacity = 0;
	ui_active_2d_elements = 0;
	ui_batch_count = 0;
	glDeleteVertexArrays(1, &ui_vao);
	glDeleteBuffers(1, &ui_vbo);
	glDeleteBuffers(1, &cube_vbo);
	glDeleteBuffers(1, &cube_ebo);
	glDeleteVertexArrays(1, &cube_vao);
	glDeleteProgram(ui_shader);
}