#include "main.h"
#include "world.h"

vec3 get_direction(float pitch, float yaw) {
	vec3 dir;
	float pitch_rad = pitch * (M_PI / 180.0f);
	float yaw_rad = (yaw - 90) * (M_PI / 180.0f);

	dir.x = -sinf(yaw_rad) * cosf(pitch_rad);
	dir.y = sinf(pitch_rad);
	dir.z = cosf(yaw_rad) * cosf(pitch_rad);
	return dir;
}

void get_targeted_block(vec3 position, vec3 direction, float reach, int* out_x, int* out_y, int* out_z, char* out_face) {
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

		// Calculate chunk and local coordinates
		int chunk_x = block_x >> 4; // Equivalent to block_x / CHUNK_SIZE
		int chunk_z = block_z >> 4; // Equivalent to block_z / CHUNK_SIZE
		int chunk_y = block_y >> 4; // Equivalent to block_y / CHUNK_SIZE

		uint8_t local_x = block_x & 0xF; // Equivalent to block_x % CHUNK_SIZE
		uint8_t local_y = block_y & 0xF; // Equivalent to block_y % CHUNK_SIZE
		uint8_t local_z = block_z & 0xF; // Equivalent to block_z % CHUNK_SIZE

		// Convert chunk world coords to render distance array indices
		int render_x = chunk_x - last_cx;
		int render_z = chunk_z - last_cz;

		// Skip if out of render distance
		if (render_x < 0 || render_x >= RENDER_DISTANCE ||
			chunk_y < 0 || chunk_y >= WORLD_HEIGHT ||
			render_z < 0 || render_z >= RENDER_DISTANCE) continue;

		// Check block ID
		uint8_t intersecting_block_id = chunks[render_x][chunk_y][render_z].blocks[local_x][local_y][local_z].id;
		if (intersecting_block_id != 0 && intersecting_block_id != 8 && intersecting_block_id != 9) {
			*out_x = block_x;
			*out_y = block_y;
			*out_z = block_z;

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
	*out_x = -1;
	*out_y = -1;
	*out_z = -1;
	*out_face = 'N';
}

// TODO: Move this elsewhere
Block* get_block_at(int world_block_x, int world_block_y, int world_block_z) {
	int chunk_x, chunk_z, block_x, block_z;
	calculate_chunk_and_block(world_block_x, &chunk_x, &block_x);
	calculate_chunk_and_block(world_block_z, &chunk_z, &block_z);

	int chunk_y = world_block_y / CHUNK_SIZE;
	int block_y = ((world_block_y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

	int render_x = chunk_x - last_cx;
	int render_z = chunk_z - last_cz;

	if (is_chunk_in_bounds(render_x, chunk_y, render_z)) {
		Chunk* chunk = &chunks[render_x][chunk_y][render_z];
		Block* block = &chunk->blocks[block_x][block_y][block_z];
		return block;
	}

	return NULL;
}

bool is_block_solid(int world_block_x, int world_block_y, int world_block_z) {
	Block* block = get_block_at(world_block_x, world_block_y, world_block_z);
	if (block != NULL)
		return block->id != 0;
	else
		return 0;
}