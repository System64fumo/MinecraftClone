#include "main.h"

void get_targeted_block(Entity* entity, int* out_x, int* out_y, int* out_z, char* out_face) {
	float px = entity->x;
	float py = entity->y;
	float pz = entity->z;

	float pitch_rad = entity->pitch * (M_PI / 180.0f);
	float yaw_rad = (entity->yaw - 90) * (M_PI / 180.0f);

	// Direction vector
	float dx = -sinf(yaw_rad) * cosf(pitch_rad);
	float dy = sinf(pitch_rad);
	float dz = cosf(yaw_rad) * cosf(pitch_rad);

	// Block coordinates
	int block_x = (int)floorf(px);
	int block_y = (int)floorf(py);
	int block_z = (int)floorf(pz);

	// Step direction
	int step_x = (dx > 0) ? 1 : -1;
	int step_y = (dy > 0) ? 1 : -1;
	int step_z = (dz > 0) ? 1 : -1;

	// Delta distance (avoid division by zero)
	float delta_x = (dx == 0) ? INFINITY : fabs(1.0f / dx);
	float delta_y = (dy == 0) ? INFINITY : fabs(1.0f / dy);
	float delta_z = (dz == 0) ? INFINITY : fabs(1.0f / dz);

	// Initial side distances
	float side_x = (dx > 0) ? (block_x + 1 - px) * delta_x : (px - block_x) * delta_x;
	float side_y = (dy > 0) ? (block_y + 1 - py) * delta_y : (py - block_y) * delta_y;
	float side_z = (dz > 0) ? (block_z + 1 - pz) * delta_z : (pz - block_z) * delta_z;

	// Maximum distance
	float max_distance = 5.0f;
	float distance = 0.0f;

	while (distance < max_distance) {
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

			float dx_intersect = px + dx * distance - block_center_x;
			float dy_intersect = py + dy * distance - block_center_y;
			float dz_intersect = pz + dz * distance - block_center_z;

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