#include "main.h"
#include "world.h"
#include "config.h"
#include <math.h>

bool frustum_culling_enabled = true;
bool occlusion_culling_enabled = false;
float frustum_offset = CHUNK_SIZE * 2;
bool frustum_changed = false;

bool is_chunk_face_solid(Chunk* chunk, int direction) {
	if (!chunk || !chunk->is_loaded) return false;
	
	int solid_blocks = 0;
	int total_blocks = CHUNK_SIZE * CHUNK_SIZE;
	float threshold = 0.85f;
	
	switch(direction) {
		case 0: // North face (Z+)
			for(int x = 0; x < CHUNK_SIZE; x++) {
				for(int y = 0; y < CHUNK_SIZE; y++) {
					if(block_data[chunk->blocks[x][y][CHUNK_SIZE-1].id][1] == 0) { // Opaque block
						solid_blocks++;
					}
				}
			}
			break;
			
		case 1: // South face (Z-)
			for(int x = 0; x < CHUNK_SIZE; x++) {
				for(int y = 0; y < CHUNK_SIZE; y++) {
					if(block_data[chunk->blocks[x][y][0].id][1] == 0) { // Opaque block
						solid_blocks++;
					}
				}
			}
			break;
			
		case 2: // East face (X+)
			for(int z = 0; z < CHUNK_SIZE; z++) {
				for(int y = 0; y < CHUNK_SIZE; y++) {
					if(block_data[chunk->blocks[CHUNK_SIZE-1][y][z].id][1] == 0) { // Opaque block
						solid_blocks++;
					}
				}
			}
			break;
			
		case 3: // West face (X-)
			for(int z = 0; z < CHUNK_SIZE; z++) {
				for(int y = 0; y < CHUNK_SIZE; y++) {
					if(block_data[chunk->blocks[0][y][z].id][1] == 0) { // Opaque block
						solid_blocks++;
					}
				}
			}
			break;
			
		case 4: // Up face (Y+)
			for(int x = 0; x < CHUNK_SIZE; x++) {
				for(int z = 0; z < CHUNK_SIZE; z++) {
					if(block_data[chunk->blocks[x][CHUNK_SIZE-1][z].id][1] == 0) { // Opaque block
						solid_blocks++;
					}
				}
			}
			break;
			
		case 5: // Down face (Y-)
			for(int x = 0; x < CHUNK_SIZE; x++) {
				for(int z = 0; z < CHUNK_SIZE; z++) {
					if(block_data[chunk->blocks[x][0][z].id][1] == 0) { // Opaque block
						solid_blocks++;
					}
				}
			}
			break;
	}
	
	return (float)solid_blocks / total_blocks >= threshold;
}

bool is_chunk_in_frustum(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	float dir_length = sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	
	if (dir_length < 0.001f) {
		return true;
	}

	vec3 normalized_dir = {dir.x / dir_length, dir.y / dir_length, dir.z / dir_length};

	vec3 frustum_origin = {
		pos.x - normalized_dir.x * frustum_offset,
		pos.y - (CHUNK_SIZE * 4) - normalized_dir.y * frustum_offset,
		pos.z - normalized_dir.z * frustum_offset
	};

	vec3 chunk_pos = {
		chunk_x * CHUNK_SIZE + CHUNK_SIZE / 2,
		chunk_y * CHUNK_SIZE + CHUNK_SIZE / 2,
		chunk_z * CHUNK_SIZE + CHUNK_SIZE / 2
	};

	vec3 origin_to_chunk = {
		chunk_pos.x - frustum_origin.x,
		chunk_pos.y - frustum_origin.y,
		chunk_pos.z - frustum_origin.z
	};

	float origin_to_chunk_length = sqrt(
		origin_to_chunk.x * origin_to_chunk.x + 
		origin_to_chunk.y * origin_to_chunk.y + 
		origin_to_chunk.z * origin_to_chunk.z
	);

	if (origin_to_chunk_length < 0.001f) {
		return true;
	}

	vec3 normalized_origin_to_chunk = {
		origin_to_chunk.x / origin_to_chunk_length, 
		origin_to_chunk.y / origin_to_chunk_length, 
		origin_to_chunk.z / origin_to_chunk_length
	};

	float dot_product = 
		normalized_dir.x * normalized_origin_to_chunk.x + 
		normalized_dir.y * normalized_origin_to_chunk.y +
		normalized_dir.z * normalized_origin_to_chunk.z;

	return dot_product >= fov_angle;
}

Chunk* get_adjacent_chunk(int x, int y, int z, int direction) {
	// Convert direction to offset
	int offset_x = 0, offset_y = 0, offset_z = 0;
	
	switch (direction) {
		case 0: offset_z = 1; break; // North (Z+)
		case 1: offset_z = -1; break; // South (Z-)
		case 2: offset_x = 1; break; // East (X+)
		case 3: offset_x = -1; break; // West (X-)
		case 4: offset_y = 1; break; // Up (Y+)
		case 5: offset_y = -1; break; // Down (Y-)
	}
	
	// Calculate adjacent chunk coordinates
	int adj_x = x + offset_x;
	int adj_y = y + offset_y;
	int adj_z = z + offset_z;
	
	// Check if coordinates are within bounds
	if (adj_x < 0 || adj_x >= RENDER_DISTANCE ||
		adj_y < 0 || adj_y >= WORLD_HEIGHT ||
		adj_z < 0 || adj_z >= RENDER_DISTANCE) {
		return NULL;
	}
	
	// Return the adjacent chunk
	return &chunks[adj_x][adj_y][adj_z];
}

bool is_chunk_occluded(vec3 pos, vec3 dir, Chunk* chunk, int chunk_array_x, int chunk_array_y, int chunk_array_z) {
	if (!chunk || !chunk->is_loaded) return false;
	
	// Get player position relative to chunk
	float rel_x = pos.x - (chunk->x * CHUNK_SIZE);
	float rel_y = pos.y - (chunk->y * CHUNK_SIZE);
	float rel_z = pos.z - (chunk->z * CHUNK_SIZE);
	
	// Calculate view direction components
	float dir_length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	if (dir_length < 0.001f) return false;
	
	vec3 norm_dir = {
		dir.x / dir_length,
		dir.y / dir_length,
		dir.z / dir_length
	};
	
	// Check if player is inside this chunk
	bool inside_chunk = 
		rel_x >= 0 && rel_x < CHUNK_SIZE &&
		rel_y >= 0 && rel_y < CHUNK_SIZE &&
		rel_z >= 0 && rel_z < CHUNK_SIZE;
	
	if (inside_chunk) return false; // Never occlude the chunk the player is in
	
	// Define potential occluders based on player position relative to chunk
	bool potential_occluders[6] = {false}; // North, South, East, West, Up, Down
	
	// Check which faces could potentially occlude this chunk
	if (norm_dir.z < -0.1f && rel_z >= CHUNK_SIZE) potential_occluders[0] = true; // North face
	if (norm_dir.z > 0.1f && rel_z < 0) potential_occluders[1] = true; // South face
	if (norm_dir.x < -0.1f && rel_x >= CHUNK_SIZE) potential_occluders[2] = true; // East face
	if (norm_dir.x > 0.1f && rel_x < 0) potential_occluders[3] = true; // West face
	if (norm_dir.y < -0.1f && rel_y >= CHUNK_SIZE) potential_occluders[4] = true; // Up face
	if (norm_dir.y > 0.1f && rel_y < 0) potential_occluders[5] = true; // Down face

	float chunk_center_x = chunk->x * CHUNK_SIZE + CHUNK_SIZE / 2.0f;
	float chunk_center_y = chunk->y * CHUNK_SIZE + CHUNK_SIZE / 2.0f;
	float chunk_center_z = chunk->z * CHUNK_SIZE + CHUNK_SIZE / 2.0f;

	float dx = chunk_center_x - pos.x;
	float dy = chunk_center_y - pos.y;
	float dz = chunk_center_z - pos.z;

	float dist_sq = dx*dx + dy*dy + dz*dz;

	if (dist_sq < CHUNK_SIZE * CHUNK_SIZE * 4) {
		return false;
	}

	for (int dir = 0; dir < 6; dir++) {
		if (!potential_occluders[dir]) continue;

		Chunk* adjacent = get_adjacent_chunk(chunk_array_x, chunk_array_y, chunk_array_z, dir);

		if (adjacent && adjacent->is_loaded) {
			int opposite_face = dir;
			if (dir % 2 == 0) opposite_face = dir + 1;
			else opposite_face = dir - 1;
			
			if (is_chunk_face_solid(adjacent, opposite_face)) {
				return true;
			}
		}
	}

	return false;
}

void update_frustum() {
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].pos.x;
	pos.y = global_entities[0].pos.y + global_entities[0].eye_level;
	pos.z = global_entities[0].pos.z;

	float fov_angle = cosf(settings.fov * M_PI / 180.0f);

	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				bool was_visible = chunk->is_visible;
				bool is_visible = true;

				if (frustum_culling_enabled) {
					int chunk_x = last_cx + x;
					int chunk_y = last_cy + y;
					int chunk_z = last_cz + z;
					is_visible = is_chunk_in_frustum(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle);
				}

				if (is_visible && occlusion_culling_enabled && chunk->is_loaded) {
					is_visible = !is_chunk_occluded(pos, dir, chunk, x, y, z);
				}

				if (was_visible != is_visible) {
					chunk->is_visible = is_visible;
					frustum_changed = true;
				}
			}
		}
	}

	if (frustum_changed) {
		mesh_needs_rebuild = true;
	}
}