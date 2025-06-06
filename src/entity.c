#include "main.h"
#include "world.h"
#include "gui.h"
#include <math.h>
#include <stdio.h>

#define GRAVITY 40.0f
#define MAX_FALL_SPEED 78.4f

typedef struct {
	float min_x, max_x;
	float min_y, max_y;
	float min_z, max_z;
} AABB;

AABB get_entity_aabb(float x, float y, float z, float width, float height) {
	float half_width = width / 2.0f;
	return (AABB) {
		.min_x = x - half_width,
		.max_x = x + half_width,
		.min_y = y,
		.max_y = y + height,
		.min_z = z - half_width,
		.max_z = z + half_width
	};
}

bool aabb_intersect(AABB a, AABB b) {
	return (a.min_x < b.max_x && a.max_x > b.min_x) &&
			(a.min_y < b.max_y && a.max_y > b.min_y) &&
			(a.min_z < b.max_z && a.max_z > b.min_z);
}

vec3 get_direction(float pitch, float yaw) {
	float pr = pitch * (M_PI / 180.0f);
	float yr = (yaw - 90.0f) * (M_PI / 180.0f);
	return (vec3) {
		.x = -sinf(yr) * cosf(pr),
		.y = sinf(pr),
		.z = cosf(yr) * cosf(pr)
	};
}

void get_targeted_block(Entity entity, vec3 direction, float reach, vec3* pos_out, char* out_face) {
	vec3 position = entity.pos;
	position.y += entity.eye_level;

	// Block coordinates
	int block_x = (int)floorf(position.x);
	int block_y = (int)floorf(position.y);
	int block_z = (int)floorf(position.z);

	// Step direction
	int step_x = (direction.x > 0) ? 1 : -1;
	int step_y = (direction.y > 0) ? 1 : -1;
	int step_z = (direction.z > 0) ? 1 : -1;

	// Delta distance (avoid division by zero)
	float delta_x = (direction.x == 0) ? INFINITY : fabs(1.0f / direction.x);
	float delta_y = (direction.y == 0) ? INFINITY : fabs(1.0f / direction.y);
	float delta_z = (direction.z == 0) ? INFINITY : fabs(1.0f / direction.z);

	// Initial side distances
	float side_x = (direction.x > 0) ? (block_x + 1 - position.x) * delta_x : (position.x - block_x) * delta_x;
	float side_y = (direction.y > 0) ? (block_y + 1 - position.y) * delta_y : (position.y - block_y) * delta_y;
	float side_z = (direction.z > 0) ? (block_z + 1 - position.z) * delta_z : (position.z - block_z) * delta_z;

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
			pos_out->x = block_x;
			pos_out->y = block_y;
			pos_out->z = block_z;

			// Determine which face was hit
			float block_center_x = block_x + 0.5f;
			float block_center_y = block_y + 0.5f;
			float block_center_z = block_z + 0.5f;

			float dx_intersect = position.x + direction.x * distance - block_center_x;
			float dy_intersect = position.y + direction.y * distance - block_center_y;
			float dz_intersect = position.z + direction.z * distance - block_center_z;

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
	pos_out->x = -1;
	pos_out->y = -1;
	pos_out->z = -1;
	*out_face = 'N';
}

bool check_entity_collision(float x, float y, float z, float width, float height) {
	AABB entity_aabb = get_entity_aabb(x, y, z, width, height);
	
	// Calculate the range of blocks we need to check
	int min_x = (int)floorf(entity_aabb.min_x);
	int max_x = (int)floorf(entity_aabb.max_x);
	int min_y = (int)floorf(entity_aabb.min_y);
	int max_y = (int)floorf(entity_aabb.max_y);
	int min_z = (int)floorf(entity_aabb.min_z);
	int max_z = (int)floorf(entity_aabb.max_z);
	
	// Check all blocks in the entity's AABB volume
	for (int bx = min_x; bx <= max_x; bx++) {
		for (int by = min_y; by <= max_y; by++) {
			for (int bz = min_z; bz <= max_z; bz++) {
				if (by < 0 || by >= WORLD_HEIGHT * CHUNK_SIZE) continue;
				
				if (is_block_solid(bx, by, bz)) {
					AABB block_aabb = {
						.min_x = bx,
						.max_x = bx + 1.0f,
						.min_y = by,
						.max_y = by + 1.0f,
						.min_z = bz,
						.max_z = bz + 1.0f
					};
					
					if (aabb_intersect(entity_aabb, block_aabb)) {
						return false; // Collision detected
					}
				}
			}
		}
	}

	return true; // No collision
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

	// Check ground collision
	for (int i = 0; i < collision_points_count; i++) {
		int ground_collision = is_block_solid(
			(int)floorf(ground_check_points[i][0]), 
			(int)floorf(new_y), 
			(int)floorf(ground_check_points[i][1])
		);

		if (ground_collision) {
			new_y = ceilf(new_y);
			entity->vertical_velocity = 0.0f;
			entity->is_grounded = true;
			break;
		}
	}

	// Check ceiling collision
	if (entity->vertical_velocity > 0) {
		for (int i = 0; i < collision_points_count; i++) {
			int ceiling_collision = is_block_solid(
				(int)floorf(ground_check_points[i][0]), 
				(int)floorf(new_y + entity->height), 
				(int)floorf(ground_check_points[i][1])
			);

			if (ceiling_collision) {
				new_y = floorf(new_y + entity->height) - entity->height;
				entity->vertical_velocity = 0.0f;
				break;
			}
		}
	}

	entity->pos.y = new_y;
}

void set_hotbar_slot(uint8_t slot) {
	printf("Selected block: %d\n", hotbar_slot + 1);
	hotbar_slot = slot;
	update_ui();
}

Entity create_entity(uint8_t id) {
	Entity entity = {0};
	switch (id) {
		case 0: // Player
			entity.width = 0.6f;
			entity.height = 1.8f;
			entity.eye_level = 1.625f;
			entity.speed = 5;
			entity.inventory_size = 10;
		break;
		default:
			printf("Unknown entity id: %d\n", id);
		break;
	}

	if (entity.inventory_size != 0) {
		entity.inventory_data = create_array(entity.inventory_size, sizeof(uint8_t));
	}

	return entity;
}