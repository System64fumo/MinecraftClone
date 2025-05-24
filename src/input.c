#include "main.h"
#include "world.h"
#include "gui.h"
#include <math.h>

float lastX = 0;
float lastY = 0;

//
// Mouse
//

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	if (ui_state != UI_STATE_RUNNING)
		return;

	int8_t offset = yoffset;
	hotbar_slot -= offset;
	set_hotbar_slot(hotbar_slot);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

	if (ui_state == UI_STATE_PAUSED) {
		// TODO: This could probably be done a little better..
		if (check_hit(lastX, lastY, 0)) {
			ui_elements[0].tex_y = 86;
			update_ui_buffer();
		}
		else if (check_hit(lastX, lastY, 1)) {
			ui_elements[1].tex_y = 86;
			update_ui_buffer();
		}
		else {
			ui_elements[0].tex_y = 66;
			ui_elements[1].tex_y = 66;
			update_ui_buffer();
		}
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
	
		update_frustum();
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (ui_state == UI_STATE_PAUSED) {
		if (action == GLFW_PRESS) return;
		//printf("Click at: %.fx%.f\n", lastX, lastY);
		// TODO: Align hitboxes with buttons, Currently hardcoded for 1920x1080..

		// Resume button
		if (check_hit(lastX, lastY, 0)) {
			ui_state = UI_STATE_RUNNING;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			update_ui();
		}

		// Quit button
		else if (check_hit(lastX, lastY, 1)) {
			glfwSetWindowShouldClose(window, true);
		}
	}
	else if (ui_state == UI_STATE_RUNNING) {
		if (action != GLFW_PRESS) return;
		vec3 block_pos;
		char face;
		vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
		get_targeted_block(global_entities[0], dir, 5.0f, &block_pos, &face);
	
		if (face == 'N') return;
	
		// Adjust block coordinates based on face for right-click
		if (button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
			switch (face) {
				case 'R': block_pos.x--; break;
				case 'L': block_pos.x++; break;
				case 'F': block_pos.z++; break;
				case 'K': block_pos.z--; break;
				case 'T': block_pos.y++; break;
				case 'B': block_pos.y--; break;
			}
		}
	
		Block* block = get_block_at(block_pos.x, block_pos.y, block_pos.z);
	
		int chunk_x, chunk_z, block_x, block_z;
		calculate_chunk_and_block(block_pos.x, &chunk_x, &block_x);
		calculate_chunk_and_block(block_pos.z, &chunk_z, &block_z);
	
		int chunk_y = block_pos.y / CHUNK_SIZE;
		int block_y = (((int)block_pos.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	
		int render_x = chunk_x - last_cx;
		int render_z = chunk_z - last_cz;
	
		if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
			Chunk* chunk = &chunks[render_x][chunk_y][render_z];
			if (button == GLFW_MOUSE_BUTTON_LEFT) {
				block->id = 0;
			}
			else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
				block->id = hotbar_slot + 1;
				block->light_data = 0;
			}
			else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
				printf("Adjacent Light data: %d\n", block->light_data);
			}
			else {
				return;
			}
			chunk->needs_update = true;
	
			update_adjacent_chunks(render_x, chunk_y, render_z, block_x, block_y, block_z);
		}
	}
}

//
// Keyboard
//

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_R:
				load_around_entity(&global_entities[0]);
				break;
			case GLFW_KEY_F2:
				mesh_mode = !mesh_mode;
				break;
			case GLFW_KEY_ESCAPE:
				glfwSetCursorPos(window, screen_center_x, screen_center_y); // This doesn't seem to work?
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

void process_input(GLFWwindow* window) {
	if (ui_state != UI_STATE_RUNNING)
		return;

	int player_cx = (int)(global_entities[0].pos.x / CHUNK_SIZE);
	int player_cy = (int)(global_entities[0].pos.y / CHUNK_SIZE);
	int player_cz = (int)(global_entities[0].pos.z / CHUNK_SIZE);

	int center_cx = last_cx + (RENDER_DISTANCE / 2);
	int center_cz = last_cz + (RENDER_DISTANCE / 2);

	int relative_cx = player_cx - (center_cx - (RENDER_DISTANCE / 2));
	int relative_cz = player_cz - (center_cz - (RENDER_DISTANCE / 2));

	if (relative_cx < 0 || relative_cx >= RENDER_DISTANCE || relative_cz < 0 || relative_cz >= RENDER_DISTANCE || player_cy < 0 || player_cy >= WORLD_HEIGHT) {
		return;
	}

	Chunk* chunk = &chunks[relative_cx][player_cy][relative_cz];

	if (!chunk->is_loaded || chunk->needs_update) {
		return;
	}

	float move_speed = global_entities[0].speed * delta_time;
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);
	float dx = 0.0f, dy = 0.0f, dz = 0.0f;
	bool frustum_needs_update = false;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		dx += cosf(yaw) * move_speed;
		dz += sinf(yaw) * move_speed;
		frustum_needs_update = true;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		dx -= cosf(yaw) * move_speed;
		dz -= sinf(yaw) * move_speed;
		frustum_needs_update = true;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		dx += sinf(yaw) * move_speed;
		dz -= cosf(yaw) * move_speed;
		frustum_needs_update = true;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		dx -= sinf(yaw) * move_speed;
		dz += cosf(yaw) * move_speed;
		frustum_needs_update = true;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && global_entities[0].is_grounded) {
		global_entities[0].vertical_velocity = 10.0f;
		global_entities[0].is_grounded = false;
		frustum_needs_update = true;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		dy -= move_speed;
		frustum_needs_update = true;
	}
	if (frustum_needs_update) {
		update_frustum();
	}

	float new_x = global_entities[0].pos.x + dx;
	float new_y = global_entities[0].pos.y + dy;
	float new_z = global_entities[0].pos.z + dz;

	bool can_move_x = check_entity_collision(
		new_x, 
		global_entities[0].pos.y, 
		global_entities[0].pos.z, 
		global_entities[0].width, 
		global_entities[0].height
	);

	bool can_move_y = check_entity_collision(
		global_entities[0].pos.x, 
		new_y, 
		global_entities[0].pos.z, 
		global_entities[0].width, 
		global_entities[0].height
	);

	bool can_move_z = check_entity_collision(
		global_entities[0].pos.x, 
		global_entities[0].pos.y, 
		new_z, 
		global_entities[0].width, 
		global_entities[0].height
	);

	if (can_move_x) global_entities[0].pos.x = new_x;
	if (can_move_y) global_entities[0].pos.y = new_y;
	if (can_move_z) global_entities[0].pos.z = new_z;

	update_entity_physics(&global_entities[0], delta_time);
}