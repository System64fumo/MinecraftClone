#include "main.h"
#include "framebuffer.h"
#include "world.h"
#include <math.h>

#define IS_WITHIN_RANGE(num, lower, upper) ((num) >= (lower) && (num) <= (upper))
bool frustum_faces[6];

float lastX = 1280.0f / 2.0f;
float lastY = 720.0f / 2.0f;
bool firstMouse = true;

#define GRAVITY 40.0f
#define MAX_FALL_SPEED 78.4f

typedef struct {
	int is_grounded;
	float vertical_velocity;
} physics_state;

physics_state player_physics = {0, 0.0f};

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
	update_ui();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_R:
				load_around_entity(&global_entities[0]);
				break;
			case GLFW_KEY_F2:
				mesh_mode = !mesh_mode;
				break;
		}
	}
}

int check_entity_collision(float x, float y, float z, float width, float height) {
	float half_width = width / 2.0f;

	float check_points[][3] = {
		{x - half_width, y, z - half_width},
		{x + half_width, y, z - half_width},
		{x - half_width, y, z + half_width},
		{x + half_width, y, z + half_width},
		{x - half_width, y + height, z - half_width},
		{x + half_width, y + height, z - half_width},
		{x - half_width, y + height, z + half_width},
		{x + half_width, y + height, z + half_width}
	};

	// Check each point for collision
	for (int i = 0; i < 8; i++) {
		if (is_block_solid(
			(int)floorf(check_points[i][0]), 
			(int)floorf(check_points[i][1]), 
			(int)floorf(check_points[i][2])
		)) {
			return 0;  // Collision detected
		}
	}

	return 1;  // No collision
}

void update_player_physics(Entity* player, float delta_time) {
	if (!player_physics.is_grounded) {
		player_physics.vertical_velocity -= GRAVITY * delta_time;
		player_physics.vertical_velocity = fmaxf(
			-MAX_FALL_SPEED, 
			player_physics.vertical_velocity
		);
	}

	float new_y = player->y + player_physics.vertical_velocity * delta_time;

	int collision_points_count = 4;
	float half_width = player->width / 2.0f;
	float ground_check_points[][2] = {
		{player->x - half_width, player->z - half_width},
		{player->x + half_width, player->z - half_width},
		{player->x - half_width, player->z + half_width},
		{player->x + half_width, player->z + half_width}
	};

	player_physics.is_grounded = 0;

	for (int i = 0; i < collision_points_count; i++) {
		int ground_collision = is_block_solid(
			(int)floorf(ground_check_points[i][0]), 
			(int)floorf(new_y), 
			(int)floorf(ground_check_points[i][1])
		);

		if (ground_collision) {
			new_y = ceilf(new_y);
			player_physics.vertical_velocity = 0.0f;
			player_physics.is_grounded = 1;
			break;
		}
	}

	player->y = new_y;
}

void process_input(GLFWwindow* window) {
	float move_speed = global_entities[0].speed * delta_time;
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);

	float dx = 0.0f, dy = 0.0f, dz = 0.0f;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		dx += cosf(yaw) * move_speed;
		dz += sinf(yaw) * move_speed;
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
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		dy += move_speed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		dy -= move_speed;
	}

	float new_x = global_entities[0].x + dx;
	float new_y = global_entities[0].y + dy;
	float new_z = global_entities[0].z + dz;

	int can_move_x = check_entity_collision(
		new_x, 
		global_entities[0].y, 
		global_entities[0].z, 
		global_entities[0].width, 
		global_entities[0].height
	);

	int can_move_y = check_entity_collision(
		global_entities[0].x, 
		new_y, 
		global_entities[0].z, 
		global_entities[0].width, 
		global_entities[0].height
	);

	int can_move_z = check_entity_collision(
		global_entities[0].x, 
		global_entities[0].y, 
		new_z, 
		global_entities[0].width, 
		global_entities[0].height
	);

	if (can_move_x) global_entities[0].x = new_x;
	if (can_move_y) global_entities[0].y = new_y;
	if (can_move_z) global_entities[0].z = new_z;
	update_player_physics(&global_entities[0], delta_time);
}

// Main callback function
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action != GLFW_PRESS) return;

	int world_block_x, world_block_y, world_block_z;
	char face;
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].x;
	pos.y = global_entities[0].y + global_entities[0].eye_level;
	pos.z = global_entities[0].z;
	get_targeted_block(pos, dir, 5.0f, &world_block_x, &world_block_y, &world_block_z, &face);

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
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			chunk->blocks[block_x][block_y][block_z].id = 0;
		}
		else {
			chunk->blocks[block_x][block_y][block_z].id = hotbar_slot;
			chunk->blocks[block_x][block_y][block_z].light_data = 0;
		}
		chunk->needs_update = true;

		// Update adjacent chunks if necessary
		update_adjacent_chunks(render_x, chunk_y, render_z, block_x, block_y, block_z);
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	int8_t offset = yoffset;
	hotbar_slot -= offset;
	printf("Slot: %d\n", hotbar_slot);
	update_ui();
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

	float32x4_t pos = {global_entities[0].x, global_entities[0].y + global_entities[0].eye_level, global_entities[0].z, 0.0f};
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
	view[12] = -(s[0] * global_entities[0].x + s[1] * global_entities[0].y + global_entities[0].eye_level + s[2] * global_entities[0].z);
	view[13] = -(u[0] * global_entities[0].x + u[1] * global_entities[0].y + global_entities[0].eye_level + u[2] * global_entities[0].z);
	view[14] = (f[0] * global_entities[0].x + f[1] * global_entities[0].y + global_entities[0].eye_level + f[2] * global_entities[0].z);
	#endif
}