#include "main.h"
#include "world.h"
#include "gui.h"
#include <math.h>

#define GRAVITY 40.0f
#define MAX_FALL_SPEED 78.4f

vec3 get_direction(float pitch, float yaw) {
	float pr = pitch * (M_PI / 180.0f);
	float yr = (yaw - 90.0f) * (M_PI / 180.0f);
	return (vec3) {
		.x = -sinf(yr) * cosf(pr),
		.y = sinf(pr),
		.z = cosf(yr) * cosf(pr)
	};
}

void update_frustum() {
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].pos.x;
	pos.y = global_entities[0].pos.y + global_entities[0].eye_level;
	pos.z = global_entities[0].pos.z;
	update_chunks_visibility(pos, dir);
}

void get_targeted_block(Entity entity, vec3 direction, float reach, int* out_x, int* out_y, int* out_z, char* out_face) {
	// Block coordinates
	int block_x = (int)floorf(entity.pos.x);
	int block_y = (int)floorf(entity.pos.y + entity.eye_level);
	int block_z = (int)floorf(entity.pos.z);

	// Step direction
	int step_x = (direction.x > 0) ? 1 : -1;
	int step_y = (direction.y > 0) ? 1 : -1;
	int step_z = (direction.z > 0) ? 1 : -1;

	// Delta distance (avoid division by zero)
	float delta_x = (direction.x == 0) ? INFINITY : fabs(1.0f / direction.x);
	float delta_y = (direction.y == 0) ? INFINITY : fabs(1.0f / direction.y);
	float delta_z = (direction.z == 0) ? INFINITY : fabs(1.0f / direction.z);

	// Initial side distances
	float side_x = (direction.x > 0) ? (block_x + 1 - entity.pos.x) * delta_x : (entity.pos.x - block_x) * delta_x;
	float side_y = (direction.y > 0) ? (block_y + 1 - entity.pos.y + entity.eye_level) * delta_y : (entity.pos.y + entity.eye_level - block_y) * delta_y;
	float side_z = (direction.z > 0) ? (block_z + 1 - entity.pos.z) * delta_z : (entity.pos.z - block_z) * delta_z;

	float distance = 0.0f;

	while (distance < reach) {
		// Determine which axis to step on
		if (side_x < side_y && side_x < side_z) {
			block_x += step_x;
			distance = side_x;
			side_x += delta_x;
		} else if (side_y < side_z) {
			block_y += step_y;
			distance = side_y;
			side_y += delta_y;
		} else {
			block_z += step_z;
			distance = side_z;
			side_z += delta_z;
		}

		// Skip if out of bounds
		if (block_y < 0 || block_y >= WORLD_HEIGHT * CHUNK_SIZE) continue;

		Block* block = get_block_at(block_x, block_y, block_z);
		if (block == NULL) continue;
		uint8_t intersecting_block_id = block->id;

		if (intersecting_block_id != 0 && intersecting_block_id != 8 && intersecting_block_id != 9) {
			*out_x = block_x;
			*out_y = block_y;
			*out_z = block_z;

			// Determine which face was hit
			float block_center_x = block_x + 0.5f;
			float block_center_y = block_y + 0.5f;
			float block_center_z = block_z + 0.5f;

			float dx_intersect = entity.pos.x + direction.x * distance - block_center_x;
			float dy_intersect = entity.pos.y + entity.eye_level + direction.y * distance - block_center_y;
			float dz_intersect = entity.pos.z + direction.z * distance - block_center_z;

			if (fabs(dx_intersect) > fabs(dy_intersect) && fabs(dx_intersect) > fabs(dz_intersect)) {
				*out_face = (dx_intersect > 0) ? 'L' : 'R';
			} else if (fabs(dy_intersect) > fabs(dx_intersect) && fabs(dy_intersect) > fabs(dz_intersect)) {
				*out_face = (dy_intersect > 0) ? 'T' : 'B';
			} else {
				*out_face = (dz_intersect > 0) ? 'F' : 'K';
			}
			return;
		}
	}

	// No block found
	*out_x = -1;
	*out_y = -1;
	*out_z = -1;
	*out_face = 'N';
}

bool check_entity_collision(float x, float y, float z, float width, float height) {
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
			return false;  // Collision detected
		}
	}

	return true;  // No collision
}

void update_entity_physics(Entity* entity, float delta_time) {
	if (!entity->is_grounded) {
		entity->vertical_velocity -= GRAVITY * delta_time;
		entity->vertical_velocity = fmaxf(
			-MAX_FALL_SPEED, 
			entity->vertical_velocity
		);
	}

	float new_y = entity->pos.y + entity->vertical_velocity * delta_time;

	int collision_points_count = 4;
	float half_width = entity->width / 2.0f;
	float ground_check_points[][2] = {
		{entity->pos.x - half_width, entity->pos.z - half_width},
		{entity->pos.x + half_width, entity->pos.z - half_width},
		{entity->pos.x - half_width, entity->pos.z + half_width},
		{entity->pos.x + half_width, entity->pos.z + half_width}
	};

	entity->is_grounded = false;

	for (int i = 0; i < collision_points_count; i++) {
		int ground_collision = is_block_solid(
			(int)floorf(ground_check_points[i][0]), 
			(int)floorf(new_y), 
			(int)floorf(ground_check_points[i][1])
		);

		if (ground_collision) {
			new_y = ceilf(new_y);
			entity->vertical_velocity = 0.0f;
			entity->is_grounded = 1;
			break;
		}
	}

	entity->pos.y = new_y;
}

void set_hotbar_slot(uint8_t slot) {
	printf("Slot: %d\n", hotbar_slot);
	hotbar_slot = slot;
	update_ui();
}