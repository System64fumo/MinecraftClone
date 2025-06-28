#include "main.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool frustum_culling_enabled = true;
bool occlusion_culling_enabled = true;
float frustum_offset = CHUNK_SIZE * 2;
bool frustum_changed = false;

// BFS queue structure for occlusion culling
typedef struct {
	int x, y, z;
	float distance_sq;
} chunk_queue_node;

static chunk_queue_node bfs_queue[RENDER_DISTANCE * WORLD_HEIGHT * RENDER_DISTANCE];
static int queue_front = 0;
static int queue_rear = 0;
static bool visited[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];

// Queue operations
static void queue_reset() {
	queue_front = 0;
	queue_rear = 0;
}

static bool queue_is_empty() {
	return queue_front == queue_rear;
}

static void queue_push(int x, int y, int z, float distance_sq) {
	if (queue_rear >= RENDER_DISTANCE * WORLD_HEIGHT * RENDER_DISTANCE) return;
	
	bfs_queue[queue_rear].x = x;
	bfs_queue[queue_rear].y = y;
	bfs_queue[queue_rear].z = z;
	bfs_queue[queue_rear].distance_sq = distance_sq;
	queue_rear++;
}

static chunk_queue_node queue_pop() {
	chunk_queue_node node = bfs_queue[queue_front];
	queue_front++;
	return node;
}

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

// Check if there's a clear path between two adjacent chunks
static bool has_clear_connection(int from_x, int from_y, int from_z, int to_x, int to_y, int to_z) {
	// Bounds check
	if (from_x < 0 || from_x >= RENDER_DISTANCE ||
		from_y < 0 || from_y >= WORLD_HEIGHT ||
		from_z < 0 || from_z >= RENDER_DISTANCE ||
		to_x < 0 || to_x >= RENDER_DISTANCE ||
		to_y < 0 || to_y >= WORLD_HEIGHT ||
		to_z < 0 || to_z >= RENDER_DISTANCE) {
		return false;
	}
	
	Chunk* from_chunk = &chunks[from_x][from_y][from_z];
	Chunk* to_chunk = &chunks[to_x][to_y][to_z];
	
	// If either chunk isn't loaded, assume connection exists
	if (!from_chunk->is_loaded || !to_chunk->is_loaded) {
		return true;
	}
	
	// Calculate direction from 'from' to 'to'
	int dx = to_x - from_x;
	int dy = to_y - from_y;
	int dz = to_z - from_z;
	
	// Only adjacent chunks should be checked
	if (abs(dx) + abs(dy) + abs(dz) != 1) {
		return false;
	}
	
	// Determine face direction
	int face_dir = -1;
	if (dx == 1) face_dir = 2;      // East
	else if (dx == -1) face_dir = 3; // West
	else if (dy == 1) face_dir = 4;  // Up
	else if (dy == -1) face_dir = 5; // Down
	else if (dz == 1) face_dir = 0;  // North
	else if (dz == -1) face_dir = 1; // South
	
	if (face_dir == -1) return true;
	
	// Check if the connecting face is solid (blocks view)
	return !is_chunk_face_solid(from_chunk, face_dir);
}

// Calculate squared distance from player position to chunk center
static float calculate_distance_sq(vec3 player_pos, int chunk_x, int chunk_y, int chunk_z) {
	float chunk_center_x = (world_offset_x + chunk_x) * CHUNK_SIZE + CHUNK_SIZE / 2.0f;
	float chunk_center_y = chunk_y * CHUNK_SIZE + CHUNK_SIZE / 2.0f;
	float chunk_center_z = (world_offset_z + chunk_z) * CHUNK_SIZE + CHUNK_SIZE / 2.0f;
	
	float dx = chunk_center_x - player_pos.x;
	float dy = chunk_center_y - player_pos.y;
	float dz = chunk_center_z - player_pos.z;
	
	return dx*dx + dy*dy + dz*dz;
}

// Find the starting chunk (where player is located)
static void find_player_chunk(vec3 player_pos, int* start_x, int* start_y, int* start_z) {
	// Convert world position to chunk coordinates
	int world_chunk_x = (int)floorf(player_pos.x / CHUNK_SIZE);
	int world_chunk_y = (int)floorf(player_pos.y / CHUNK_SIZE);
	int world_chunk_z = (int)floorf(player_pos.z / CHUNK_SIZE);
	
	// Convert to array indices
	*start_x = world_chunk_x - world_offset_x;
	*start_y = world_chunk_y;
	*start_z = world_chunk_z - world_offset_z;
	
	// Clamp to valid range
	*start_x = (*start_x < 0) ? 0 : (*start_x >= RENDER_DISTANCE) ? RENDER_DISTANCE - 1 : *start_x;
	*start_y = (*start_y < 0) ? 0 : (*start_y >= WORLD_HEIGHT) ? WORLD_HEIGHT - 1 : *start_y;
	*start_z = (*start_z < 0) ? 0 : (*start_z >= RENDER_DISTANCE) ? RENDER_DISTANCE - 1 : *start_z;
}

// BFS-based occlusion culling
void perform_bfs_occlusion_culling(vec3 player_pos) {
	memset(visited, false, sizeof(visited));
	
	int start_x, start_y, start_z;
	find_player_chunk(player_pos, &start_x, &start_y, &start_z);
	
	queue_reset();
	float start_dist_sq = calculate_distance_sq(player_pos, start_x, start_y, start_z);
	queue_push(start_x, start_y, start_z, start_dist_sq);
	visited[start_x][start_y][start_z] = true;
	
	// Direction offsets for 26-directional connectivity (including diagonals)
	int dir_offsets[26][3];
	int dir_count = 0;
	
	// Generate all possible directions (-1, 0, 1) for each axis
	for(int dx = -1; dx <= 1; dx++) {
		for(int dy = -1; dy <= 1; dy++) {
			for(int dz = -1; dz <= 1; dz++) {
				if(dx == 0 && dy == 0 && dz == 0) continue;
				dir_offsets[dir_count][0] = dx;
				dir_offsets[dir_count][1] = dy;
				dir_offsets[dir_count][2] = dz;
				dir_count++;
			}
		}
	}
	
	while (!queue_is_empty()) {
		chunk_queue_node current = queue_pop();
		visibility_map[current.x][current.y][current.z] = true;
		
		// Process neighbors in order of distance from player
		chunk_queue_node neighbors[26];
		int neighbor_count = 0;
		
		// Check all 26 adjacent chunks
		for (int dir = 0; dir < dir_count; dir++) {
			int next_x = current.x + dir_offsets[dir][0];
			int next_y = current.y + dir_offsets[dir][1];
			int next_z = current.z + dir_offsets[dir][2];
			
			if (next_x < 0 || next_x >= RENDER_DISTANCE ||
				next_y < 0 || next_y >= WORLD_HEIGHT ||
				next_z < 0 || next_z >= RENDER_DISTANCE ||
				visited[next_x][next_y][next_z]) {
				continue;
			}
			
			// Check line of sight through intermediate chunks
			bool has_line_of_sight = true;
			if (abs(dir_offsets[dir][0]) + abs(dir_offsets[dir][1]) + abs(dir_offsets[dir][2]) > 1) {
				// For diagonal connections, check intermediate chunks
				int intermediate_x = current.x + dir_offsets[dir][0];
				int intermediate_y = current.y;
				int intermediate_z = current.z;
				
				if (!has_clear_connection(current.x, current.y, current.z, intermediate_x, intermediate_y, intermediate_z)) {
					has_line_of_sight = false;
				}
				
				intermediate_x = current.x;
				intermediate_y = current.y + dir_offsets[dir][1];
				intermediate_z = current.z;
				
				if (!has_clear_connection(current.x, current.y, current.z, intermediate_x, intermediate_y, intermediate_z)) {
					has_line_of_sight = false;
				}
				
				intermediate_x = current.x;
				intermediate_y = current.y;
				intermediate_z = current.z + dir_offsets[dir][2];
				
				if (!has_clear_connection(current.x, current.y, current.z, intermediate_x, intermediate_y, intermediate_z)) {
					has_line_of_sight = false;
				}
			} else {
				has_line_of_sight = has_clear_connection(current.x, current.y, current.z, next_x, next_y, next_z);
			}
			
			if (has_line_of_sight) {
				float next_dist_sq = calculate_distance_sq(player_pos, next_x, next_y, next_z);
				neighbors[neighbor_count].x = next_x;
				neighbors[neighbor_count].y = next_y;
				neighbors[neighbor_count].z = next_z;
				neighbors[neighbor_count].distance_sq = next_dist_sq;
				neighbor_count++;
				visited[next_x][next_y][next_z] = true;
			}
		}
		
		// Sort neighbors by distance and add to queue
		for (int i = 0; i < neighbor_count - 1; i++) {
			for (int j = 0; j < neighbor_count - i - 1; j++) {
				if (neighbors[j].distance_sq > neighbors[j + 1].distance_sq) {
					chunk_queue_node temp = neighbors[j];
					neighbors[j] = neighbors[j + 1];
					neighbors[j + 1] = temp;
				}
			}
		}
		
		for (int i = 0; i < neighbor_count; i++) {
			queue_push(neighbors[i].x, neighbors[i].y, neighbors[i].z, neighbors[i].distance_sq);
		}
	}
	
	for (int x = 0; x < RENDER_DISTANCE; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < RENDER_DISTANCE; z++) {
				if (!visited[x][y][z]) {
					visibility_map[x][y][z] = false;
				}
			}
		}
	}
}

bool is_chunk_occluded(vec3 pos, vec3 dir, Chunk* chunk, int chunk_array_x, int chunk_array_y, int chunk_array_z) {
	return !visibility_map[chunk_array_x][chunk_array_y][chunk_array_z];
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

	// Store previous visibility state
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

	// Apply BFS occlusion culling if enabled
	if (occlusion_culling_enabled) {
		perform_bfs_occlusion_culling(pos);
	}

	// Check if visibility changed
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				if (prev_visibility[x][y][z] != visibility_map[x][y][z]) {
					frustum_changed = true;
					break;
				}
			}
			if (frustum_changed) break;
		}
		if (frustum_changed) break;
	}

	if (frustum_changed) {
		mesh_needs_rebuild = true;
	}

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_CULLING, false);
	#endif
}