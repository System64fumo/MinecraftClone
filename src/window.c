#include "main.h"

float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
bool firstMouse = true;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;

	aspect = (float)screen_width / (float)screen_height;
	screen_center_x = screen_width / 2.0f;
	screen_center_y = screen_height / 2.0f;

	glViewport(0, 0, screen_width, screen_height);
	matrix4_identity(projection);
	matrix4_perspective(projection, fov * 3.14159f / 180.0f, aspect, near, far);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_R:
				load_around_entity(&global_entities[0]);
				break;
		}
	}
}

void processInput(GLFWwindow* window) {
	float moveSpeed = global_entities[0].speed * deltaTime;
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		global_entities[0].x += cosf(yaw) * moveSpeed;
		global_entities[0].z += sinf(yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		global_entities[0].x -= cosf(yaw) * moveSpeed;
		global_entities[0].z -= sinf(yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		global_entities[0].x += sinf(yaw) * moveSpeed;
		global_entities[0].z -= cosf(yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		global_entities[0].x -= sinf(yaw) * moveSpeed;
		global_entities[0].z += cosf(yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		global_entities[0].y += moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		global_entities[0].y -= moveSpeed;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		int world_block_x, world_block_y, world_block_z;
		char face;
		get_targeted_block(&global_entities[0], &world_block_x, &world_block_y, &world_block_z, &face);
		int chunk_x = world_block_x / CHUNK_SIZE;
		int chunk_y = world_block_y / CHUNK_SIZE;
		int chunk_z = world_block_z / CHUNK_SIZE;

		unsigned char block_x = world_block_x % CHUNK_SIZE;
		unsigned char block_y = world_block_y % CHUNK_SIZE;
		unsigned char block_z = world_block_z % CHUNK_SIZE;

		for (int cx = 0; cx < RENDERR_DISTANCE; cx++) {
			for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
				for (int cz = 0; cz < RENDERR_DISTANCE; cz++) {
					Chunk* chunk = &chunks[cx][cy][cz];
					if (chunk_x == chunk->x && chunk_y == chunk->y && chunk_z == chunk->z) {
						chunk->blocks[block_x][block_y][block_z].id = 0;
						chunk->needs_update = true;

						if (block_x == 0)
							chunks[cx - 1][cy][cz].needs_update = true;
						else if (block_x == 15)
							chunks[cx + 1][cy][cz].needs_update = true;

						if (block_y == 0)
							chunks[cx][cy - 1][cz].needs_update = true;
						else if (block_y == 15)
							chunks[cx][cy + 1][cz].needs_update = true;

						if (block_z == 0)
							chunks[cx][cy][cz - 1].needs_update = true;
						else if (block_z == 15)
							chunks[cx][cy][cz + 1].needs_update = true;

						break;
					}
				}
			}
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		printf("TODO: Add build functionality\n");
	}
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
}

void setupMatrices() {
	matrix4_identity(view);
	
	float f[3];
	f[0] = cosf(global_entities[0].yaw * 3.14159f/180.0f) * cosf(global_entities[0].pitch * 3.14159f/180.0f);
	f[1] = sinf(global_entities[0].pitch * 3.14159f/180.0f);
	f[2] = sinf(global_entities[0].yaw * 3.14159f/180.0f) * cosf(global_entities[0].pitch * 3.14159f/180.0f);
	float len = sqrtf(f[0]*f[0] + f[1]*f[1] + f[2]*f[2]);
	f[0] /= len; f[1] /= len; f[2] /= len;

	float s[3] = {
		f[1] * 0 - f[2] * 1,
		f[2] * 0 - f[0] * 0,
		f[0] * 1 - f[1] * 0
	};
	len = sqrtf(s[0]*s[0] + s[1]*s[1] + s[2]*s[2]);
	s[0] /= len; s[1] /= len; s[2] /= len;

	float u[3] = {
		s[1] * f[2] - s[2] * f[1],
		s[2] * f[0] - s[0] * f[2],
		s[0] * f[1] - s[1] * f[0]
	};

	view[0] = s[0]; view[4] = s[1]; view[8] = s[2];
	view[1] = u[0]; view[5] = u[1]; view[9] = u[2];
	view[2] = -f[0]; view[6] = -f[1]; view[10] = -f[2];
	view[12] = -(s[0] * global_entities[0].x + s[1] * global_entities[0].y + s[2] * global_entities[0].z);
	view[13] = -(u[0] * global_entities[0].x + u[1] * global_entities[0].y + u[2] * global_entities[0].z);
	view[14] = (f[0] * global_entities[0].x + f[1] * global_entities[0].y + f[2] * global_entities[0].z);
}
