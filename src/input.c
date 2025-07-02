#include "main.h"
#include "world.h"
#include "gui.h"
#include "shaders.h"
#include "config.h"
#include <math.h>
#include <stdio.h>

float last_cursor_x = 0;
float last_cursor_y = 0;

static double last_break_time = 0;
static double last_place_time = 0;
const double block_action_interval = 0.5;

int last_pitch, last_yaw;
int last_player_pos_x, last_player_pos_y, last_player_pos_z;

//
// Mouse
//

void set_hotbar_slot(uint8_t slot) {
	hotbar_slot = slot;
	update_ui();
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (ui_state != UI_STATE_RUNNING)
		return;

	int8_t offset = yoffset;
	hotbar_slot -= offset;
	set_hotbar_slot(hotbar_slot);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	float xoffset = xpos - last_cursor_x;
	float yoffset = last_cursor_y - ypos;
	last_cursor_x = xpos;
	last_cursor_y = ypos;

	if (ui_state == UI_STATE_PAUSED) {
		// TODO: This could probably be done a little better..
		if (check_hit(xpos, ypos, 0)) {
			ui_elements[0].tex_y = 86;
		}
		else if (check_hit(xpos, ypos, 1)) {
			ui_elements[1].tex_y = 86;
		}
		else {
			ui_elements[0].tex_y = 66;
			ui_elements[1].tex_y = 66;
		}
		update_ui_buffer();
	}
	else if (ui_state == UI_STATE_RUNNING) {
		float sensitivity = 0.1f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;
	
		global_entities[0].yaw += xoffset;
		global_entities[0].pitch += yoffset;
	
		if (global_entities[0].pitch > 89.0f) {
			global_entities[0].pitch = 89.0f;
		}
		if (global_entities[0].pitch < -89.0f) {
			global_entities[0].pitch = -89.0f;
		}
	
		// Wrap yaw
		if (global_entities[0].yaw >= 360.0f) {
			global_entities[0].yaw -= 360.0f;
		}
		if (global_entities[0].yaw < 0.0f) {
			global_entities[0].yaw += 360.0f;
		}

		int pitch = floorl(global_entities[0].pitch);
		int yaw = floorl(global_entities[0].yaw);
		if (pitch != last_pitch) {
			last_pitch = pitch;
			frustum_changed = true;
		}
		if (yaw != last_yaw) {
			last_yaw = yaw;
			frustum_changed = true;
		}
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (ui_state == UI_STATE_PAUSED) {
		if (action == GLFW_PRESS) return;

		// Resume button
		if (check_hit(last_cursor_x, last_cursor_y, 0)) {
			ui_state = UI_STATE_RUNNING;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			update_ui();
		}

		// Quit button
		else if (check_hit(last_cursor_x, last_cursor_y, 1)) {
			glfwSetWindowShouldClose(window, true);
		}
	}
	else if (ui_state == UI_STATE_RUNNING) {
		if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
			last_break_time = 0;
			last_place_time = 0;
			return;
		}
		else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
			vec3 block_pos;
			char face;
			vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
			get_targeted_block(global_entities[0], dir, 5.0f, &block_pos, &face);
				
			if (face == 'N') return;
				
			Block* block = get_block_at(chunks, block_pos.x, block_pos.y, block_pos.z);
		}
	}
}

//
// Keyboard
//

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_L:
				load_around_entity(&global_entities[0]);
				break;
			case GLFW_KEY_F2:
				mesh_mode = !mesh_mode;
				break;
			case GLFW_KEY_LEFT_CONTROL:
				global_entities[0].sprinting = true;
				break;
			case GLFW_KEY_R:
				load_shaders();
				break;
			case GLFW_KEY_T:
				unsigned char* light_texture = generate_light_texture();
				printf("Saved slice /tmp/slice.webp\n");
				save_light_slice(light_texture, 5, "/tmp/slice.webp");
				break;
			case GLFW_KEY_ESCAPE:
				if (ui_state == UI_STATE_RUNNING) {
					ui_state = UI_STATE_PAUSED;
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
				else {
					ui_state = UI_STATE_RUNNING;
					glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}
				update_ui();
				break;
		}

		// Hotbar
		if (key >= 49 && key <= 57)
			set_hotbar_slot(key - 49);
	}
}

bool is_valid_block_position(float x, float y, float z) {
	// Check world height bounds
	if (y < 0 || y >= WORLD_HEIGHT * CHUNK_SIZE) {
		return false;
	}
	
	// Check if we're trying to place blocks too far away
	int chunk_x = (int)floorf(x / CHUNK_SIZE);
	int chunk_z = (int)floorf(z / CHUNK_SIZE);
	int render_x = chunk_x - world_offset_x;
	int render_z = chunk_z - world_offset_z;
	
	return (render_x >= 0 && render_x < settings.render_distance && 
			render_z >= 0 && render_z < settings.render_distance);
}

void process_input(GLFWwindow* window, Chunk*** chunks) {
	if (ui_state != UI_STATE_RUNNING)
		return;

	// Handle continuous block breaking/placing
	double current_time = glfwGetTime();
	
	// Left mouse button - break blocks
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (current_time - last_break_time >= block_action_interval) {
			vec3 block_pos;
			char face;
			vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
			get_targeted_block(global_entities[0], dir, 5.0f, &block_pos, &face);
			
			if (face != 'N' && is_valid_block_position(block_pos.x, block_pos.y, block_pos.z)) {
				Block* block = get_block_at(chunks, block_pos.x, block_pos.y, block_pos.z);
				// TODO: Add error handling
				if (block == NULL)
					return;

				int chunk_x, chunk_z, block_x, block_z;
				calculate_chunk_and_block(block_pos.x, &chunk_x, &block_x);
				calculate_chunk_and_block(block_pos.z, &chunk_z, &block_z);
				
				int chunk_y = block_pos.y / CHUNK_SIZE;
				int block_y = (((int)block_pos.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
				
				int render_x = chunk_x - world_offset_x;
				int render_z = chunk_z - world_offset_z;
				
				if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
					Chunk* chunk = &chunks[render_x][chunk_y][render_z];
					block->id = 0;
					chunk->needs_update = true;
					update_adjacent_chunks(chunks, render_x, chunk_y, render_z, block_x, block_y, block_z);
				}
			}
			last_break_time = current_time;
		}
	}
	
	// Right mouse button - place blocks
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (current_time - last_place_time >= block_action_interval) {
			vec3 block_pos;
			char face;
			vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
			get_targeted_block(global_entities[0], dir, 5.0f, &block_pos, &face);
			
			if (face != 'N') {
				// Adjust block coordinates based on face for placement
				switch (face) {
					case 'R': block_pos.x--; break;
					case 'L': block_pos.x++; break;
					case 'F': block_pos.z++; break;
					case 'K': block_pos.z--; break;
					case 'T': block_pos.y++; break;
					case 'B': block_pos.y--; break;
				}
				
				// Check if the new position is valid
				if (is_valid_block_position(block_pos.x, block_pos.y, block_pos.z)) {
					// Check if placing here would intersect with any entity
					AABB block_aabb = {
						.min_x = floorf(block_pos.x),
						.max_x = floorf(block_pos.x) + 1.0f,
						.min_y = floorf(block_pos.y),
						.max_y = floorf(block_pos.y) + 1.0f,
						.min_z = floorf(block_pos.z),
						.max_z = floorf(block_pos.z) + 1.0f
					};
					
					AABB player_aabb = get_entity_aabb(
						global_entities[0].pos.x,
						global_entities[0].pos.y,
						global_entities[0].pos.z,
						global_entities[0].width,
						global_entities[0].height
					);
					
					if (!aabb_intersect(block_aabb, player_aabb)) {
						Block* block = get_block_at(chunks, block_pos.x, block_pos.y, block_pos.z);

						// TODO: Add error handling
						if (block == NULL)
							return;

						int chunk_x, chunk_z, block_x, block_z;
						calculate_chunk_and_block(block_pos.x, &chunk_x, &block_x);
						calculate_chunk_and_block(block_pos.z, &chunk_z, &block_z);
						
						int chunk_y = block_pos.y / CHUNK_SIZE;
						int block_y = (((int)block_pos.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
						
						int render_x = chunk_x - world_offset_x;
						int render_z = chunk_z - world_offset_z;
						
						if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
							Chunk* chunk = &chunks[render_x][chunk_y][render_z];
							block->id = hotbar_slot + 1;
							block->light_level = 0;
							chunk->needs_update = true;
							update_adjacent_chunks(chunks, render_x, chunk_y, render_z, block_x, block_y, block_z);
						}
					}
				}
			}
			last_place_time = current_time;
		}
	}

	int player_cx = (int)(global_entities[0].pos.x / CHUNK_SIZE);
	int player_cy = (int)(global_entities[0].pos.y / CHUNK_SIZE);
	int player_cz = (int)(global_entities[0].pos.z / CHUNK_SIZE);

	int center_cx = world_offset_x + (settings.render_distance / 2);
	int center_cz = world_offset_z + (settings.render_distance / 2);

	int relative_cx = player_cx - (center_cx - (settings.render_distance / 2));
	int relative_cz = player_cz - (center_cz - (settings.render_distance / 2));

	bool within_height = player_cy > 0 && player_cy < WORLD_HEIGHT;
	if (within_height) {
		if (relative_cx < 0 || relative_cx >= settings.render_distance || relative_cz < 0 || relative_cz >= settings.render_distance) {
			return;
		}

		Chunk* chunk = &chunks[relative_cx][player_cy][relative_cz];

		if (!chunk->is_loaded || chunk->needs_update) {
			return;
		}
	}

	float move_speed = global_entities[0].speed;
	if (global_entities[0].sprinting == true) {
		move_speed *= 1.3;
		settings.fov = settings.fov_desired * 1.1;
	}
	else {
		settings.fov = settings.fov_desired;
	}
	move_speed *= delta_time;

	float yaw = global_entities[0].yaw * DEG_TO_RAD;
	float dx = 0.0f, dy = 0.0f, dz = 0.0f;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		dx += cosf(yaw) * move_speed;
		dz += sinf(yaw) * move_speed;
	}
	else if (glfwGetKey(window, GLFW_KEY_W) == GLFW_RELEASE) {
		global_entities[0].sprinting = false;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		dx -= cosf(yaw) * move_speed;
		dz -= sinf(yaw) * move_speed;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		dx += sinf(yaw) * move_speed;
		dz -= cosf(yaw) * move_speed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		dx -= sinf(yaw) * move_speed;
		dz += cosf(yaw) * move_speed;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && global_entities[0].is_grounded) {
		global_entities[0].vertical_velocity = 10.0f;
		global_entities[0].is_grounded = false;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		dy -= move_speed;
	}

	move_entity_with_collision(&global_entities[0], dx, dy, dz);
	update_entity_physics(&global_entities[0], delta_time);

	int x = floor(global_entities[0].pos.x);
	int y = floor(global_entities[0].pos.y);
	int z = floor(global_entities[0].pos.z);
	if (x != last_player_pos_x || y != last_player_pos_y || z != last_player_pos_z) {
		last_player_pos_x = x;
		last_player_pos_y = y;
		last_player_pos_z = z;
		frustum_changed = true;
	}
}