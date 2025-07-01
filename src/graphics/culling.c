#include "main.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

float frustum_offset = CHUNK_SIZE * 1;
bool frustum_changed = false;

#define MAX_RENDER_DISTANCE_SQ ((settings.render_distance * CHUNK_SIZE * 0.8f) * (settings.render_distance * CHUNK_SIZE * 0.8f))
#define CLOSE_RENDER_DISTANCE_SQ ((CHUNK_SIZE * 1.0f) * (CHUNK_SIZE * 1.0f))

static inline float vec3_distance_sq(vec3 a, vec3 b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	float dz = a.z - b.z;
	return dx * dx + dy * dy + dz * dz;
}

static inline float vec3_dot(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

bool is_chunk_in_frustum(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	vec3 chunk_center = {
		chunk_x * CHUNK_SIZE + CHUNK_SIZE * 0.5f,
		chunk_y * CHUNK_SIZE + CHUNK_SIZE * 0.5f,
		chunk_z * CHUNK_SIZE + CHUNK_SIZE * 0.5f
	};

	float distance_sq = vec3_distance_sq(pos, chunk_center);
	if (distance_sq > MAX_RENDER_DISTANCE_SQ) {
		return false;
	}

	if (distance_sq < CLOSE_RENDER_DISTANCE_SQ) {
		return true;
	}

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

	vec3 to_chunk = {
		chunk_center.x - pos.x,
		chunk_center.y - pos.y,
		chunk_center.z - pos.z
	};

	float to_chunk_length = sqrtf(distance_sq);
	if (to_chunk_length < 0.001f) {
		return true;
	}

	vec3 normalized_to_chunk = {
		to_chunk.x / to_chunk_length,
		to_chunk.y / to_chunk_length,
		to_chunk.z / to_chunk_length
	};

	float dot_product = vec3_dot(normalized_dir, normalized_to_chunk);

	float chunk_angular_size = (CHUNK_SIZE * 0.866f) / to_chunk_length;
	float adjusted_fov_angle = fov_angle - chunk_angular_size;

	return dot_product >= adjusted_fov_angle;
}

uint8_t get_visible_faces(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	if (!is_chunk_in_frustum(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle)) {
		return 0;
	}

	uint8_t visible_faces = ALL_FACES;

	float chunk_min_x = chunk_x * CHUNK_SIZE;
	float chunk_max_x = chunk_min_x + CHUNK_SIZE;
	float chunk_min_y = chunk_y * CHUNK_SIZE;
	float chunk_max_y = chunk_min_y + CHUNK_SIZE;
	float chunk_min_z = chunk_z * CHUNK_SIZE;
	float chunk_max_z = chunk_min_z + CHUNK_SIZE;

	float threshold = 0.25f;

	// X-axis faces
	if (pos.x > chunk_max_x + threshold) {
		visible_faces &= ~FACE_RIGHT;
	}
	if (pos.x < chunk_min_x - threshold) {
		visible_faces &= ~FACE_LEFT;
	}

	// Y-axis faces
	if (pos.y > chunk_max_y + threshold) {
		visible_faces &= ~FACE_TOP;
	}
	if (pos.y < chunk_min_y - threshold) {
		visible_faces &= ~FACE_BOTTOM;
	}

	// Z-axis faces
	if (pos.z > chunk_max_z + threshold) {
		visible_faces &= ~FACE_FRONT;
	}
	if (pos.z < chunk_min_z - threshold) {
		visible_faces &= ~FACE_BACK;
	}

	vec3 normalized_dir = dir;
	float dir_length = sqrtf(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	if (dir_length > 0.001f) {
		normalized_dir.x /= dir_length;
		normalized_dir.y /= dir_length;
		normalized_dir.z /= dir_length;

		float dir_threshold = 0.85f;

		if (normalized_dir.x > dir_threshold) visible_faces &= ~FACE_LEFT;
		if (normalized_dir.x < -dir_threshold) visible_faces &= ~FACE_RIGHT;
		if (normalized_dir.y > dir_threshold) visible_faces &= ~FACE_BOTTOM;
		if (normalized_dir.y < -dir_threshold) visible_faces &= ~FACE_TOP;
		if (normalized_dir.z > dir_threshold) visible_faces &= ~FACE_BACK;
		if (normalized_dir.z < -dir_threshold) visible_faces &= ~FACE_FRONT;
	}

	return visible_faces;
}

void update_frustum() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_CULLING, false);
	#endif

	static vec3 cached_dir;
	static vec3 cached_pos;
	static float cached_fov_angle;
	static int cached_pitch = -9999, cached_yaw = -9999;
	static float cached_fov = -1.0f;
	static float cached_width = -1.0f, cached_height = -1.0f;

	int pitch = global_entities[0].pitch;
	int yaw = global_entities[0].yaw;
	float fov = settings.fov;
	float width = (float)settings.window_width;
	float height = (float)settings.window_height;

	if (pitch != cached_pitch || yaw != cached_yaw || fov != cached_fov || width != cached_width || height != cached_height) {
		cached_dir = get_direction(pitch, yaw);
		cached_pos.x = global_entities[0].pos.x;
		cached_pos.y = global_entities[0].pos.y + global_entities[0].eye_level;
		cached_pos.z = global_entities[0].pos.z;
		cached_fov_angle = cosf(fov * DEG_TO_RAD);
		cached_pitch = pitch;
		cached_yaw = yaw;
		cached_fov = fov;
		cached_width = width;
		cached_height = height;
	}

	vec3 dir = cached_dir;
	vec3 pos = cached_pos;
	float fov_angle = cached_fov_angle;

	static uint8_t prev_visibility[32][WORLD_HEIGHT][32];
	static bool first_run = true;
	
	if (!first_run) {
		memcpy(prev_visibility, visibility_map, sizeof(prev_visibility));
	}

	frustum_changed = false;

	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				uint8_t visible_faces = ALL_FACES;

				if (settings.frustum_culling) {
					int chunk_x = world_offset_x + x;
					int chunk_y = y;
					int chunk_z = world_offset_z + z;
					
					visible_faces = get_visible_faces(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle);
				}

				visibility_map[x][y][z] = visible_faces;

				if (!first_run && !frustum_changed && prev_visibility[x][y][z] != visible_faces) {
					frustum_changed = true;
				}
			}
		}
	}

	if (first_run) {
		frustum_changed = true;
		first_run = false;
	}

	memcpy(prev_visibility, visibility_map, sizeof(prev_visibility));

	if (frustum_changed) {
		mesh_needs_rebuild = true;
	}

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_CULLING, false);
	#endif
}