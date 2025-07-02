#include "main.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool frustum_changed = false;

#define MAX_RENDER_DISTANCE_SQ ((settings.render_distance * CHUNK_SIZE * 0.8f) * (settings.render_distance * CHUNK_SIZE * 0.8f))
#define CLOSE_RENDER_DISTANCE_SQ ((CHUNK_SIZE * 1.0f) * (CHUNK_SIZE * 1.0f))
#define MIN_OCCLUSION_DISTANCE_SQ ((CHUNK_SIZE * 2.5f) * (CHUNK_SIZE * 2.5f))
#define MAX_STEP_SIZE 1.0f
#define MIN_STEP_SIZE 0.25f
#define VISIBILITY_THRESHOLD 0.05f 
#define EDGE_VISIBILITY_THRESHOLD 0.01f
#define MAX_TEST_POINTS 24
#define SAMPLE_BLOCK_THRESHOLD 4

static inline vec3 vec3_sub(vec3 a, vec3 b) { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline float vec3_distance_sq(vec3 a, vec3 b) { vec3 d = vec3_sub(a, b); return d.x*d.x + d.y*d.y + d.z*d.z; }
static inline float vec3_dot(vec3 a, vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline float vec3_length(vec3 v) { return sqrtf(v.x*v.x + v.y*v.y + v.z*v.z); }
static inline vec3 vec3_normalize(vec3 v) { 
	float len = vec3_length(v); 
	return len < 0.001f ? (vec3){0} : (vec3){v.x/len, v.y/len, v.z/len}; 
}

static inline vec3 get_chunk_center(int x, int y, int z) {
	return (vec3){
		x * CHUNK_SIZE + CHUNK_SIZE * 0.5f,
		y * CHUNK_SIZE + CHUNK_SIZE * 0.5f,
		z * CHUNK_SIZE + CHUNK_SIZE * 0.5f
	};
}

static bool is_ray_obstructed(vec3 start, vec3 end) {
	vec3 direction = vec3_sub(end, start);
	float total_distance = vec3_length(direction);
	
	if (total_distance < 0.001f) return false;
	
	direction = vec3_normalize(direction);
	float step_size = fminf(MAX_STEP_SIZE, fmaxf(MIN_STEP_SIZE, total_distance * 0.01f));
	int max_steps = (int)(total_distance / step_size) + 1;
	
	int consecutive_air_blocks = 0;
	int solid_block_encounters = 0;
	
	for (int i = 1; i < max_steps; i++) {
		float t = (float)i * step_size;
		if (t >= total_distance * 0.95f) break;
		
		vec3 test_pos = {
			start.x + direction.x * t,
			start.y + direction.y * t,
			start.z + direction.z * t
		};

		Block* block = get_block_at(chunks, test_pos.x, test_pos.y, test_pos.z);
		if (!block) return false;
		
		if (block_data[block->id][1] == 0) {
			if (++solid_block_encounters >= 2) return true;
			consecutive_air_blocks = 0;
		} else if (++consecutive_air_blocks > 5) {
			solid_block_encounters = 0;
		}
	}
	
	return false;
}

static bool chunk_has_surface_blocks(int chunk_x, int chunk_y, int chunk_z) {
	if (chunk_x < world_offset_x || chunk_x >= world_offset_x + settings.render_distance ||
		chunk_z < world_offset_z || chunk_z >= world_offset_z + settings.render_distance) {
		return false;
	}
	
	int cx = chunk_x - world_offset_x;
	int cz = chunk_z - world_offset_z;
	
	if (chunk_y < 0 || chunk_y >= WORLD_HEIGHT || !chunks || !chunks[cx] || 
		!chunks[cx][chunk_y] || !chunks[cx][chunk_y][cz].is_loaded) {
		return false;
	}
	
	Chunk* chunk = &chunks[cx][chunk_y][cz];
	const int sample_positions[][3] = {
		{0,0,0}, {CHUNK_SIZE-1,0,0}, {0,0,CHUNK_SIZE-1}, {CHUNK_SIZE-1,0,CHUNK_SIZE-1},
		{0,CHUNK_SIZE-1,0}, {CHUNK_SIZE-1,CHUNK_SIZE-1,0}, {0,CHUNK_SIZE-1,CHUNK_SIZE-1}, {CHUNK_SIZE-1,CHUNK_SIZE-1,CHUNK_SIZE-1},
		{CHUNK_SIZE/2,CHUNK_SIZE/2,CHUNK_SIZE/2}, {0,CHUNK_SIZE/2,CHUNK_SIZE/2}, {CHUNK_SIZE-1,CHUNK_SIZE/2,CHUNK_SIZE/2},
		{CHUNK_SIZE/2,0,CHUNK_SIZE/2}, {CHUNK_SIZE/2,CHUNK_SIZE-1,CHUNK_SIZE/2}, {CHUNK_SIZE/2,CHUNK_SIZE/2,0}, {CHUNK_SIZE/2,CHUNK_SIZE/2,CHUNK_SIZE-1}
	};
	
	int non_air_blocks = 0;
	for (int i = 0; i < sizeof(sample_positions)/sizeof(sample_positions[0]); i++) {
		if (chunk->blocks[sample_positions[i][0]][sample_positions[i][1]][sample_positions[i][2]].id > 0) {
			non_air_blocks++;
		}
	}
	
	return non_air_blocks >= SAMPLE_BLOCK_THRESHOLD;
}

static int generate_test_points(int chunk_x, int chunk_y, int chunk_z, vec3 viewer_pos, vec3 test_points[MAX_TEST_POINTS]) {
	vec3 chunk_center = get_chunk_center(chunk_x, chunk_y, chunk_z);
	float distance = sqrtf(vec3_distance_sq(viewer_pos, chunk_center));
	int num_points = 0;
	
	test_points[num_points++] = chunk_center;
	
	float chunk_min_x = chunk_x * CHUNK_SIZE;
	float chunk_min_y = chunk_y * CHUNK_SIZE;
	float chunk_min_z = chunk_z * CHUNK_SIZE;
	float chunk_max_x = chunk_min_x + CHUNK_SIZE;
	float chunk_max_y = chunk_min_y + CHUNK_SIZE;
	float chunk_max_z = chunk_min_z + CHUNK_SIZE;
	
	if (distance < CHUNK_SIZE * 4) {
		const vec3 corners[] = {
			{chunk_min_x, chunk_min_y, chunk_min_z}, {chunk_max_x, chunk_min_y, chunk_min_z},
			{chunk_min_x, chunk_max_y, chunk_min_z}, {chunk_min_x, chunk_min_y, chunk_max_z},
			{chunk_max_x, chunk_max_y, chunk_min_z}, {chunk_max_x, chunk_min_y, chunk_max_z},
			{chunk_min_x, chunk_max_y, chunk_max_z}, {chunk_max_x, chunk_max_y, chunk_max_z}
		};
		
		const vec3 face_centers[] = {
			{chunk_center.x, chunk_center.y, chunk_min_z}, {chunk_center.x, chunk_center.y, chunk_max_z},
			{chunk_min_x, chunk_center.y, chunk_center.z}, {chunk_max_x, chunk_center.y, chunk_center.z},
			{chunk_center.x, chunk_min_y, chunk_center.z}, {chunk_center.x, chunk_max_y, chunk_center.z}
		};
		
		const vec3 edge_points[] = {
			{chunk_min_x, chunk_min_y, chunk_center.z}, {chunk_max_x, chunk_min_y, chunk_center.z},
			{chunk_min_x, chunk_max_y, chunk_center.z}, {chunk_max_x, chunk_max_y, chunk_center.z},
			{chunk_min_x, chunk_center.y, chunk_min_z}, {chunk_max_x, chunk_center.y, chunk_min_z},
			{chunk_min_x, chunk_center.y, chunk_max_z}, {chunk_max_x, chunk_center.y, chunk_max_z},
			{chunk_center.x, chunk_min_y, chunk_min_z}, {chunk_center.x, chunk_max_y, chunk_min_z},
			{chunk_center.x, chunk_min_y, chunk_max_z}, {chunk_center.x, chunk_max_y, chunk_max_z}
		};
		
		for (int i = 0; i < 8 && num_points < MAX_TEST_POINTS; i++) test_points[num_points++] = corners[i];
		for (int i = 0; i < 6 && num_points < MAX_TEST_POINTS; i++) test_points[num_points++] = face_centers[i];
		for (int i = 0; i < 12 && num_points < MAX_TEST_POINTS; i++) test_points[num_points++] = edge_points[i];
	} else if (distance < CHUNK_SIZE * 8) {
		const vec3 points[] = {
			{chunk_min_x, chunk_min_y, chunk_min_z}, {chunk_max_x, chunk_min_y, chunk_min_z},
			{chunk_min_x, chunk_max_y, chunk_min_z}, {chunk_min_x, chunk_min_y, chunk_max_z},
			{chunk_max_x, chunk_max_y, chunk_min_z}, {chunk_max_x, chunk_min_y, chunk_max_z},
			{chunk_min_x, chunk_max_y, chunk_max_z}, {chunk_max_x, chunk_max_y, chunk_max_z},
			{chunk_min_x, chunk_center.y, chunk_center.z}, {chunk_max_x, chunk_center.y, chunk_center.z},
			{chunk_center.x, chunk_min_y, chunk_center.z}, {chunk_center.x, chunk_max_y, chunk_center.z}
		};
		
		for (int i = 0; i < 12 && num_points < MAX_TEST_POINTS; i++) test_points[num_points++] = points[i];
	} else {
		vec3 to_chunk = vec3_normalize(vec3_sub(chunk_center, viewer_pos));
		float x_pos = to_chunk.x > 0 ? chunk_max_x : chunk_min_x;
		float y_pos = to_chunk.y > 0 ? chunk_max_y : chunk_min_y;
		float z_pos = to_chunk.z > 0 ? chunk_max_z : chunk_min_z;
		
		const vec3 points[] = {
			{x_pos, y_pos, z_pos},
			{x_pos, chunk_center.y, chunk_center.z},
			{chunk_center.x, y_pos, chunk_center.z},
			{chunk_center.x, chunk_center.y, z_pos}
		};
		
		for (int i = 0; i < 4 && num_points < MAX_TEST_POINTS; i++) test_points[num_points++] = points[i];
	}
	
	return num_points;
}

static bool is_chunk_occluded(vec3 viewer_pos, int chunk_x, int chunk_y, int chunk_z) {
	vec3 chunk_center = get_chunk_center(chunk_x, chunk_y, chunk_z);
	float distance_sq = vec3_distance_sq(viewer_pos, chunk_center);
	
	if (distance_sq < MIN_OCCLUSION_DISTANCE_SQ || distance_sq > MAX_RENDER_DISTANCE_SQ) {
		return false;
	}
	
	bool has_surface_blocks = chunk_has_surface_blocks(chunk_x, chunk_y, chunk_z);
	float effective_threshold = has_surface_blocks ? EDGE_VISIBILITY_THRESHOLD : VISIBILITY_THRESHOLD;
	
	vec3 test_points[MAX_TEST_POINTS];
	int num_test_points = generate_test_points(chunk_x, chunk_y, chunk_z, viewer_pos, test_points);
	
	int visible_points = 0;
	for (int i = 0; i < num_test_points; i++) {
		if (!is_ray_obstructed(viewer_pos, test_points[i])) {
			visible_points++;
			if ((float)visible_points / (i+1) > effective_threshold && i >= 2) return false;
		}
		if ((float)(visible_points + (num_test_points-i-1)) / num_test_points < effective_threshold && i >= 5) return true;
	}
	
	return (float)visible_points / num_test_points < effective_threshold;
}

bool is_chunk_in_frustum(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	vec3 chunk_center = get_chunk_center(chunk_x, chunk_y, chunk_z);
	float distance_sq = vec3_distance_sq(pos, chunk_center);
	
	if (distance_sq > MAX_RENDER_DISTANCE_SQ) return false;
	if (distance_sq < CLOSE_RENDER_DISTANCE_SQ) return true;
	
	float dir_length = vec3_length(dir);
	if (dir_length < 0.001f) return true;
	
	vec3 normalized_dir = vec3_normalize(dir);
	vec3 to_chunk = vec3_normalize(vec3_sub(chunk_center, pos));
	
	float dot_product = vec3_dot(normalized_dir, to_chunk);
	float chunk_angular_size = (CHUNK_SIZE * 0.866f) / sqrtf(distance_sq);
	
	if (dot_product < (fov_angle - chunk_angular_size)) return false;

	if (settings.occlusion_culling) {
		pthread_mutex_lock(&chunks_mutex);
		bool occluded = is_chunk_occluded(pos, chunk_x, chunk_y, chunk_z);
		pthread_mutex_unlock(&chunks_mutex);
		
		return !occluded;
	}
	
	return true;
}

uint8_t get_visible_faces(vec3 pos, vec3 dir, int chunk_x, int chunk_y, int chunk_z, float fov_angle) {
	if (!is_chunk_in_frustum(pos, dir, chunk_x, chunk_y, chunk_z, fov_angle)) return 0;

	uint8_t visible_faces = ALL_FACES;

	if (settings.face_culling) {
		vec3 chunk_min = {chunk_x * CHUNK_SIZE, chunk_y * CHUNK_SIZE, chunk_z * CHUNK_SIZE};
		vec3 chunk_max = {chunk_min.x + CHUNK_SIZE, chunk_min.y + CHUNK_SIZE, chunk_min.z + CHUNK_SIZE};

		if (pos.x > chunk_max.x + 0.25f) visible_faces &= ~FACE_RIGHT;
		if (pos.x < chunk_min.x - 0.25f) visible_faces &= ~FACE_LEFT;
		if (pos.y > chunk_max.y + 0.25f) visible_faces &= ~FACE_TOP;
		if (pos.y < chunk_min.y - 0.25f) visible_faces &= ~FACE_BOTTOM;
		if (pos.z > chunk_max.z + 0.25f) visible_faces &= ~FACE_FRONT;
		if (pos.z < chunk_min.z - 0.25f) visible_faces &= ~FACE_BACK;

		float dir_length = vec3_length(dir);
		if (dir_length > 0.001f) {
			vec3 normalized_dir = vec3_normalize(dir);
			const float dir_threshold = 0.85f;
			
			if (normalized_dir.x > dir_threshold) visible_faces &= ~FACE_LEFT;
			if (normalized_dir.x < -dir_threshold) visible_faces &= ~FACE_RIGHT;
			if (normalized_dir.y > dir_threshold) visible_faces &= ~FACE_BOTTOM;
			if (normalized_dir.y < -dir_threshold) visible_faces &= ~FACE_TOP;
			if (normalized_dir.z > dir_threshold) visible_faces &= ~FACE_BACK;
			if (normalized_dir.z < -dir_threshold) visible_faces &= ~FACE_FRONT;
		}
	}

	return visible_faces;
}

void update_frustum() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_CULLING, false);
	#endif

	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos = (vec3){
		global_entities[0].pos.x,
		global_entities[0].pos.y + global_entities[0].eye_level,
		global_entities[0].pos.z
	};
	float fov_angle = cosf(settings.fov * DEG_TO_RAD);


	static uint8_t prev_visibility[32][WORLD_HEIGHT][32];
	static bool first_run = true;
	
	if (!first_run) memcpy(prev_visibility, visibility_map, sizeof(prev_visibility));
	
	frustum_changed = false;

	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				uint8_t visible_faces = settings.frustum_culling ? 
					get_visible_faces(pos, dir, world_offset_x + x, y, world_offset_z + z, fov_angle) : 
					ALL_FACES;
				
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
	if (frustum_changed) mesh_needs_rebuild = true;

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_CULLING, false);
	#endif
}