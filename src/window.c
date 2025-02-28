#include "main.h"
#include "framebuffer.h"
#include "world.h"

#define IS_WITHIN_RANGE(num, lower, upper) ((num) >= (lower) && (num) <= (upper))
bool frustum_faces[6];

float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
bool firstMouse = true;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;

	aspect = (float)screen_width / (float)screen_height;
	screen_center_x = screen_width / 2;
	screen_center_y = screen_height / 2;

	glViewport(0, 0, screen_width, screen_height);
	matrix4_identity(projection);
	matrix4_perspective(projection, fov * M_PI / 180.0f, aspect, near, far);
	resize_framebuffer(width, height);
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

// Helper function to calculate chunk and block coordinates
void calculate_chunk_and_block(int world_coord, int* chunk_coord, int* block_coord) {
	*chunk_coord = (world_coord < 0) ? ((world_coord + 1) / CHUNK_SIZE - 1) : (world_coord / CHUNK_SIZE);
	*block_coord = ((world_coord % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
}

// Helper function to check if a chunk is within bounds
bool is_chunk_in_bounds(int render_x, int chunk_y, int render_z) {
	return (render_x >= 0 && render_x < RENDER_DISTANCE &&
			chunk_y >= 0 && chunk_y < WORLD_HEIGHT &&
			render_z >= 0 && render_z < RENDER_DISTANCE);
}

// Helper function to update adjacent chunks
void update_adjacent_chunks(int render_x, int chunk_y, int render_z, int block_x, int block_y, int block_z) {
	if (block_x == 0 && render_x > 0)
		chunks[render_x - 1][chunk_y][render_z].needs_update = true;
	else if (block_x == CHUNK_SIZE - 1 && render_x < RENDER_DISTANCE - 1)
		chunks[render_x + 1][chunk_y][render_z].needs_update = true;

	if (block_y == 0 && chunk_y > 0)
		chunks[render_x][chunk_y - 1][render_z].needs_update = true;
	else if (block_y == CHUNK_SIZE - 1 && chunk_y < WORLD_HEIGHT - 1)
		chunks[render_x][chunk_y + 1][render_z].needs_update = true;

	if (block_z == 0 && render_z > 0)
		chunks[render_x][chunk_y][render_z - 1].needs_update = true;
	else if (block_z == CHUNK_SIZE - 1 && render_z < RENDER_DISTANCE - 1)
		chunks[render_x][chunk_y][render_z + 1].needs_update = true;
}

// Main callback function
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_PRESS) return;

	int world_block_x, world_block_y, world_block_z;
	char face;
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].x;
	pos.y = global_entities[0].y;
	pos.z = global_entities[0].z;
	get_targeted_block(pos, dir, &world_block_x, &world_block_y, &world_block_z, &face);

	if (face == 'N') return;

	// Adjust block coordinates based on face for right-click
	if (button == GLFW_MOUSE_BUTTON_RIGHT) {
		switch (face) {
			case 'R': world_block_x--; break;
			case 'L': world_block_x++; break;
			case 'F': world_block_z++; break;
			case 'K': world_block_z--; break;
			case 'T': world_block_y++; break;
			case 'B': world_block_y--; break;
		}
	}

	// Calculate chunk and block coordinates
	int chunk_x, chunk_z, block_x, block_z;
	calculate_chunk_and_block(world_block_x, &chunk_x, &block_x);
	calculate_chunk_and_block(world_block_z, &chunk_z, &block_z);

	int chunk_y = world_block_y / CHUNK_SIZE;
	int block_y = ((world_block_y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

	// Convert to render array indices
	int render_x = chunk_x - last_cx;
	int render_z = chunk_z - last_cz;

	// Check if the chunk is within bounds
	if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
		Chunk* chunk = &chunks[render_x][chunk_y][render_z];
		chunk->blocks[block_x][block_y][block_z].id = (button == GLFW_MOUSE_BUTTON_LEFT) ? 0 : hotbar_slot;
		chunk->needs_update = true;

		// Update adjacent chunks if necessary
		update_adjacent_chunks(render_x, chunk_y, render_z, block_x, block_y, block_z);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	int8_t offset = yoffset;
	hotbar_slot -= offset;
	printf("Slot: %d\n", hotbar_slot);
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

	frustum_faces[0] = IS_WITHIN_RANGE(global_entities[0].yaw, 45, 135);
	frustum_faces[1] = (IS_WITHIN_RANGE(global_entities[0].yaw, 0, 45) || IS_WITHIN_RANGE(global_entities[0].yaw, 315, 360));
	frustum_faces[2] = IS_WITHIN_RANGE(global_entities[0].yaw, 225, 315);
	frustum_faces[3] = IS_WITHIN_RANGE(global_entities[0].yaw, 135, 225);
	frustum_faces[4] = IS_WITHIN_RANGE(global_entities[0].pitch, -90, -45);
	frustum_faces[5] = IS_WITHIN_RANGE(global_entities[0].pitch, 45, 90);
}

void setup_matrices() {
	matrix4_identity(view);

	float pitch = global_entities[0].pitch * (M_PI / 180.0f);
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);

	#if USE_ARM_OPTIMIZED_CODE

	float cos_pitch = cosf(pitch);
	float sin_pitch = sinf(pitch);
	float cos_yaw = cosf(yaw);
	float sin_yaw = sinf(yaw);

	float32x4_t f = {cos_yaw * cos_pitch, sin_pitch, sin_yaw * cos_pitch, 0.0f};
	float32x4_t len_sq = vmulq_f32(f, f);
	float len = sqrtf(vaddvq_f32(len_sq));
	f = vdivq_f32(f, vdupq_n_f32(len));

	float32x4_t s = {-vgetq_lane_f32(f, 2), 0.0f, vgetq_lane_f32(f, 0), 0.0f};
	len_sq = vmulq_f32(s, s);
	len = sqrtf(vaddvq_f32(len_sq));
	s = vdivq_f32(s, vdupq_n_f32(len));

	float32x4_t u = {
		vgetq_lane_f32(s, 1) * vgetq_lane_f32(f, 2) - vgetq_lane_f32(s, 2) * vgetq_lane_f32(f, 1),
		vgetq_lane_f32(s, 2) * vgetq_lane_f32(f, 0) - vgetq_lane_f32(s, 0) * vgetq_lane_f32(f, 2),
		vgetq_lane_f32(s, 0) * vgetq_lane_f32(f, 1) - vgetq_lane_f32(s, 1) * vgetq_lane_f32(f, 0),
		0.0f
	};

	view[0] = vgetq_lane_f32(s, 0); view[4] = vgetq_lane_f32(s, 1); view[8] = vgetq_lane_f32(s, 2);
	view[1] = vgetq_lane_f32(u, 0); view[5] = vgetq_lane_f32(u, 1); view[9] = vgetq_lane_f32(u, 2);
	view[2] = -vgetq_lane_f32(f, 0); view[6] = -vgetq_lane_f32(f, 1); view[10] = -vgetq_lane_f32(f, 2);

	float32x4_t pos = {global_entities[0].x, global_entities[0].y, global_entities[0].z, 0.0f};
	view[12] = -vaddvq_f32(vmulq_f32(s, pos));
	view[13] = -vaddvq_f32(vmulq_f32(u, pos));
	view[14] = vaddvq_f32(vmulq_f32(f, pos));

	#else // Non ARM platforms

	float f[3];
	f[0] = cosf(yaw) * cosf(pitch);
	f[1] = sinf(pitch);
	f[2] = sinf(yaw) * cosf(pitch);
	float len = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	f[0] /= len; f[1] /= len; f[2] /= len;

	float s[3] = {
		f[1] * 0 - f[2] * 1,
		f[2] * 0 - f[0] * 0,
		f[0] * 1 - f[1] * 0
	};
	len = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
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
	#endif
}