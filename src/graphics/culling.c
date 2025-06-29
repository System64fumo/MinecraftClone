#include "main.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool frustum_culling_enabled = true;
float frustum_offset = CHUNK_SIZE * 2;
bool frustum_changed = false;

bool is_chunk_in_frustum(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	float dir_length_sq = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
	
	if (dir_length_sq < 0.001f) {
		return true;
	}

	float inv_dir_length = 1.0f / sqrtf(dir_length_sq);
	vec3 normalized_dir = {
		dir.x * inv_dir_length, 
		dir.y * inv_dir_length, 
		dir.z * inv_dir_length
	};

	vec3 frustum_origin = {
		pos.x - normalized_dir.x * frustum_offset,
		pos.y - normalized_dir.y * frustum_offset,
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

	float origin_to_chunk_length_sq = 
		origin_to_chunk.x * origin_to_chunk.x + 
		origin_to_chunk.y * origin_to_chunk.y + 
		origin_to_chunk.z * origin_to_chunk.z;

	if (origin_to_chunk_length_sq < 0.001f) {
		return true;
	}

	float inv_origin_to_chunk_length = 1.0f / sqrtf(origin_to_chunk_length_sq);
	vec3 normalized_origin_to_chunk = {
		origin_to_chunk.x * inv_origin_to_chunk_length, 
		origin_to_chunk.y * inv_origin_to_chunk_length, 
		origin_to_chunk.z * inv_origin_to_chunk_length
	};

	float dot_product = 
		normalized_dir.x * normalized_origin_to_chunk.x + 
		normalized_dir.y * normalized_origin_to_chunk.y +
		normalized_dir.z * normalized_origin_to_chunk.z;

	return dot_product >= fov_angle;
}

void update_frustum() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_CULLING, false);
	#endif

	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].pos.x;
	pos.y = global_entities[0].pos.y + global_entities[0].eye_level;
	pos.z = global_entities[0].pos.z;

	float fov_angle = cosf(settings.fov * DEG_TO_RAD);

	// Store previous visibility state for change detection
	bool prev_visibility[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];
	memcpy(prev_visibility, visibility_map, sizeof(visibility_map));

	// Apply frustum culling first
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				bool is_visible = true;

				if (frustum_culling_enabled) {
					int chunk_x = world_offset_x + x;
					int chunk_y = y;
					int chunk_z = world_offset_z + z;
					is_visible = is_chunk_in_frustum(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle);
				}

				visibility_map[x][y][z] = is_visible;
			}
		}
	}

	// Check if visibility changed with early exit
	frustum_changed = false;
	for (uint8_t x = 0; x < RENDER_DISTANCE && !frustum_changed; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT && !frustum_changed; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				if (prev_visibility[x][y][z] != visibility_map[x][y][z]) {
					frustum_changed = true;
					break;
				}
			}
		}
	}

	if (frustum_changed) {
		mesh_needs_rebuild = true;
	}

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_CULLING, false);
	#endif
}
