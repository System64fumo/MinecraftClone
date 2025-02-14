#include "main.h"

void get_targeted_block(Entity* entity, int* out_x, int* out_y, int* out_z) {
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
	for(float dist = 0; dist < max_distance; dist += step) {
		float x = px + dx * dist;
		float y = py + dy * dist;
		float z = pz + dz * dist;

		// Convert to block coordinates
		int block_x = (int)floorf(x);
		int block_y = (int)floorf(y);
		int block_z = (int)floorf(z);
		
		// Get chunk coordinates
		int chunk_x = block_x / CHUNK_SIZE;
		int chunk_y = block_y / CHUNK_SIZE;
		int chunk_z = block_z / CHUNK_SIZE;
		
		// Get local block coordinates within chunk
		int local_x = block_x % CHUNK_SIZE;
		int local_y = block_y % CHUNK_SIZE;
		int local_z = block_z % CHUNK_SIZE;

		// Negative blocks don't exist in chunks
		if (local_x < 0)
			continue;
		if (local_y < 0)
			continue;
		if (local_z < 0)
			continue;

		// Check if chunk is loaded and block exists
		for(int cx = 0; cx < RENDERR_DISTANCE; cx++) {
			for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
				for(int cz = 0; cz < RENDERR_DISTANCE; cz++) {
					if(chunks[cx][cy][cz].x == chunk_x && 
					   chunks[cx][cy][cz].y == chunk_y && 
					   chunks[cx][cy][cz].z == chunk_z) {
						if(chunks[cx][cy][cz].blocks[local_x][local_y][local_z].id != 0) {
							*out_x = block_x;
							*out_y = block_y;
							*out_z = block_z;
							return;
						}
					}
				}
			}
		}
	}

	// No block found
	*out_x = -1;
	*out_y = -1;
	*out_z = -1;
}