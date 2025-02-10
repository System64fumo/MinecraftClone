#include "main.h"

// Raycast function to find block player is looking at
bool raycast(Entity* player, float max_distance, int* out_x, int* out_y, int* out_z, float* out_distance) {
	float px = player->x;
	float py = player->y;
	float pz = player->z;
	
	// Direction vector based on player rotation
	const float PI = 3.14159265358979323846f;
	float dx = sin(player->yaw * PI / 180.0f) * cos(player->pitch * PI / 180.0f);
	float dy = -sin(player->pitch * PI / 180.0f);
	float dz = -cos(player->yaw * PI / 180.0f) * cos(player->pitch * PI / 180.0f);
	
	// Ray step size
	float step = 0.1f;
	float distance = 0.0f;

	while (distance < max_distance) {
		// Update position along ray
		px += dx * step;
		py += dy * step;
		pz += dz * step;
		
		// Get chunk coordinates
		int chunk_x = (int)(floor(px / CHUNK_SIZE)) % RENDERR_DISTANCE;
		int chunk_y = (int)(floor(py / CHUNK_SIZE)) % WORLD_HEIGHT;
		int chunk_z = (int)(floor(pz / CHUNK_SIZE)) % RENDERR_DISTANCE;

		// Get block coordinates within chunk
		int block_x = ((int)floor(px)) % CHUNK_SIZE;
		int block_y = ((int)floor(py)) % CHUNK_SIZE;
		int block_z = ((int)floor(pz)) % CHUNK_SIZE;
		
		// Ensure we're within valid chunk bounds
		if (chunk_x >= 0 && chunk_x < RENDERR_DISTANCE &&
			chunk_y >= 0 && chunk_y < WORLD_HEIGHT &&
			chunk_z >= 0 && chunk_z < RENDERR_DISTANCE) {
			
			// Fix negative modulo
			if (block_x < 0) block_x += CHUNK_SIZE;
			if (block_y < 0) block_y += CHUNK_SIZE;
			if (block_z < 0) block_z += CHUNK_SIZE;

			// Check if block exists
			if (chunks[chunk_x][chunk_y][chunk_z].blocks[block_x][block_y][block_z].id != 0) {
				*out_x = (int)floor(px);
				*out_y = (int)floor(py);
				*out_z = (int)floor(pz);
				return true;
			}
		}
		
		distance += step;
	}
	
	return false;
}

void process_keyboard_movement(const Uint8* key_state, Entity* player, float delta_time) {
	const float PI = 3.14159265358979323846f;
	if (key_state[SDL_SCANCODE_W]) {
		player->z -= cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->x -= -sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_S]) {
		player->z += cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->x += -sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_A]) {
		player->x -= cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->z -= sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_D]) {
		player->x += cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->z += sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_SPACE]) {
		player->y += player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT]) {
		player->y -= player->speed * delta_time;
	}
}