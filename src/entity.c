#include "main.h"
#include "world.h"
#include "gui.h"
#include <math.h>
#include <stdio.h>

#define GRAVITY 40.0f
#define MAX_FALL_SPEED 78.4f

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
	float pr = pitch * DEG_TO_RAD;
	float yr = (yaw - 90.0f) * DEG_TO_RAD;
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
		entity->vertical_velocity = fmaxf(-MAX_FALL_SPEED, entity->vertical_velocity);
	}

	const float max_step = 0.25f;
	float remaining_time = delta_time;
	int iterations = 0;
	const int max_iterations = 5;
	
	entity->is_grounded = false;
	bool hit_ceiling = false;

	while (remaining_time > 0.0f && iterations < max_iterations) {
		iterations++;
		hit_ceiling = false;

		// Calculate movement for this substep
		float step_time = fminf(remaining_time, max_step / fmaxf(fabsf(entity->vertical_velocity), 0.1f));
		float step_y = entity->vertical_velocity * step_time;

		// Check ground collision (moving down)
		if (step_y < 0) {
			int collision_points_count = 4;
			float half_width = entity->width / 2.0f;
			float ground_check_points[][2] = {
				{entity->pos.x - half_width, entity->pos.z - half_width},
				{entity->pos.x + half_width, entity->pos.z - half_width},
				{entity->pos.x - half_width, entity->pos.z + half_width},
				{entity->pos.x + half_width, entity->pos.z + half_width}
			};

			float min_distance = step_y;
			bool hit_ground = false;

			// Find the highest ground collision point
			for (int i = 0; i < collision_points_count; i++) {
				float test_y = entity->pos.y + step_y;
				int ground_collision = is_block_solid(
					(int)floorf(ground_check_points[i][0]), 
					(int)floorf(test_y), 
					(int)floorf(ground_check_points[i][1])
				);

				if (ground_collision) {
					float collision_y = ceilf(test_y);
					float distance = collision_y - entity->pos.y;
					if (distance > min_distance) {
						min_distance = distance;
						hit_ground = true;
					}
				}
			}

			if (hit_ground) {
				entity->pos.y += min_distance;
				entity->vertical_velocity = 0.0f;
				entity->is_grounded = true;
				remaining_time = 0.0f;
				break;
			}
		}
		
		// Check ceiling collision (moving up)
		if (step_y > 0) {
			int collision_points_count = 4;
			float half_width = entity->width / 2.0f;
			float ceiling_check_points[][2] = {
				{entity->pos.x - half_width, entity->pos.z - half_width},
				{entity->pos.x + half_width, entity->pos.z - half_width},
				{entity->pos.x - half_width, entity->pos.z + half_width},
				{entity->pos.x + half_width, entity->pos.z + half_width}
			};

			float max_distance = step_y;
			hit_ceiling = false;

			// Find the lowest ceiling collision point
			for (int i = 0; i < collision_points_count; i++) {
				float test_y = entity->pos.y + entity->height + step_y;
				int ceiling_collision = is_block_solid(
					(int)floorf(ceiling_check_points[i][0]), 
					(int)floorf(test_y), 
					(int)floorf(ceiling_check_points[i][1])
				);

				if (ceiling_collision) {
					float collision_y = floorf(test_y) - entity->height;
					float distance = collision_y - entity->pos.y;
					if (distance < max_distance) {
						max_distance = distance;
						hit_ceiling = true;
					}
				}
			}

			if (hit_ceiling) {
				entity->pos.y += max_distance;
				entity->vertical_velocity = 0.0f;
				step_y = max_distance;
			}
		}

		if (!entity->is_grounded && !hit_ceiling) {
			entity->pos.y += step_y;
		}

		remaining_time -= step_time;
	}
}

Entity create_entity(uint8_t id) {
	Entity entity = {0};
	switch (id) {
		case 0: // Player
			entity.width = 0.6f;
			entity.height = 1.8f;
			entity.eye_level = 1.625f;
			entity.speed = 5;
			entity.inventory_size = 39;
		break;
		default:
			printf("Unknown entity id: %d\n", id);
		break;
	}

	if (entity.inventory_size != 0) {
		entity.inventory_data = create_array(entity.inventory_size, sizeof(Item));
	}

	return entity;
}