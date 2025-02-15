#include "main.h"

void get_targeted_block(Entity* entity, int* out_x, int* out_y, int* out_z, char* out_face) {
	float px = entity->x;
	float py = entity->y;
	float pz = entity->z;

	float pitch_rad = entity->pitch * M_PI / 180.0f;
	float yaw_rad = (entity->yaw - 90) * M_PI / 180.0f;

	// Direction vector
	float dx = -sinf(yaw_rad) * cosf(pitch_rad);
	float dy = sinf(pitch_rad);
	float dz = cosf(yaw_rad) * cosf(pitch_rad);

	// Ray step size (smaller = more precise but slower)
	float step = 0.1f;
	float max_distance = 5.0f;

	// Step along ray
	for (float dist = 0; dist < max_distance; dist += step) {
		float x = px + dx * dist;
		float y = py + dy * dist;
		float z = pz + dz * dist;

		// Convert to block coordinates
		int block_x = (int)floorf(x);
		int block_y = (int)floorf(y);
		int block_z = (int)floorf(z);

		if (block_y < 0)
			continue;

		// Handle negative coordinates properly
		int chunk_x = (block_x < 0) ? ((block_x + 1) / CHUNK_SIZE - 1) : (block_x / CHUNK_SIZE);
		int chunk_z = (block_z < 0) ? ((block_z + 1) / CHUNK_SIZE - 1) : (block_z / CHUNK_SIZE);
		int chunk_y = block_y / CHUNK_SIZE;

		// Convert to local coordinates (always positive 0-15)
		int local_x = ((block_x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
		int local_y = ((block_y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
		int local_z = ((block_z % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;

		// Convert chunk world coords to render distance array indices
		int render_x = chunk_x - (last_cx);
		int render_z = chunk_z - (last_cz);

		if (render_x < 0 || render_x >= RENDERR_DISTANCE ||
			chunk_y < 0 || chunk_y >= WORLD_HEIGHT ||
			render_z < 0 || render_z >= RENDERR_DISTANCE) continue;

		if (chunks[render_x][chunk_y][render_z].blocks[local_x][local_y][local_z].id != 0) {
			*out_x = block_x;
			*out_y = block_y;
			*out_z = block_z;

			float block_center_x = block_x + 0.5f;
			float block_center_y = block_y + 0.5f;
			float block_center_z = block_z + 0.5f;

			float dx_intersect = x - block_center_x;
			float dy_intersect = y - block_center_y;
			float dz_intersect = z - block_center_z;

			if (fabs(dx_intersect) > fabs(dy_intersect) && fabs(dx_intersect) > fabs(dz_intersect)) {
				*out_face = (dx_intersect > 0) ? 'L' : 'R';
			}
			else if (fabs(dy_intersect) > fabs(dx_intersect) && fabs(dy_intersect) > fabs(dz_intersect)) {
				*out_face = (dy_intersect > 0) ? 'T' : 'B';
			}
			else {
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