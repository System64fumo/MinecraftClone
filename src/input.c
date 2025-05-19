#include "main.h"
#include "world.h"
#include <math.h>

float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
bool firstMouse = true;

//
// Mouse
//

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	int8_t offset = yoffset;
	hotbar_slot -= offset;
	set_hotbar_slot(hotbar_slot);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;
	lastX = xpos;
	lastY = ypos;

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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_PRESS) return;

	int world_block_x, world_block_y, world_block_z;
	char face;
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	get_targeted_block(global_entities[0], dir, 5.0f, &world_block_x, &world_block_y, &world_block_z, &face);

	if (face == 'N') return;

	// Adjust block coordinates based on face for right-click
	if (button == GLFW_MOUSE_BUTTON_RIGHT || button == GLFW_MOUSE_BUTTON_MIDDLE) {
		switch (face) {
			case 'R': world_block_x--; break;
			case 'L': world_block_x++; break;
			case 'F': world_block_z++; break;
			case 'K': world_block_z--; break;
			case 'T': world_block_y++; break;
			case 'B': world_block_y--; break;
		}
	}

	Block* block = get_block_at(world_block_x, world_block_y, world_block_z);

	int chunk_x, chunk_z, block_x, block_z;
	calculate_chunk_and_block(world_block_x, &chunk_x, &block_x);
	calculate_chunk_and_block(world_block_z, &chunk_z, &block_z);

	int chunk_y = world_block_y / CHUNK_SIZE;
	int block_y = ((world_block_y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

	int render_x = chunk_x - last_cx;
	int render_z = chunk_z - last_cz;

	if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
		Chunk* chunk = &chunks[render_x][chunk_y][render_z];
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			block->id = 0;
		}
		else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
			block->id = hotbar_slot;
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
				glfwSetWindowShouldClose(window, true);
				break;
		}

		// Hotbar
		if (key >= 49 && key <= 57)
			set_hotbar_slot(key - 49);
	}
}

void process_input(GLFWwindow* window) {
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
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		dy += move_speed;
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