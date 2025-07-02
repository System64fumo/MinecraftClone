#include "main.h"
#include "config.h"

bool are_all_neighbors_loaded(uint8_t x, uint8_t y, uint8_t z) {
	if (x > 0 && !chunks[x-1][y][z].is_loaded) return false;
	if (x < settings.render_distance-1 && !chunks[x+1][y][z].is_loaded) return false;
	if (y > 0 && !chunks[x][y-1][z].is_loaded) return false;
	if (y < WORLD_HEIGHT-1 && !chunks[x][y+1][z].is_loaded) return false;
	if (z > 0 && !chunks[x][y][z-1].is_loaded) return false;
	if (z < settings.render_distance-1 && !chunks[x][y][z+1].is_loaded) return false;
	return true;
}

static inline bool check_bounds(int8_t val, int8_t* ci, int8_t* coord) {
	if (val < 0) {
		*ci -= 1;
		*coord = CHUNK_SIZE - 1;
		return true;
	}
	if (val >= CHUNK_SIZE) {
		*ci += 1;
		*coord = 0;
		return true;
	}
	return false;
}

bool is_face_visible(Chunk* chunk, int8_t x, int8_t y, int8_t z, uint8_t face) {
	int8_t nx = x, ny = y, nz = z;
	int8_t cix = chunk->ci_x, ciy = chunk->ci_y, ciz = chunk->ci_z;

	switch (face) {
		case 0: nz += 1; break; // Front
		case 1: nx += 1; break; // Left
		case 2: nz -= 1; break; // Back
		case 3: nx -= 1; break; // Right
		case 4: ny -= 1; break; // Bottom
		case 5: ny += 1; break; // Top
		default: return false;  // Invalid face
	}

	bool bounds_changed = check_bounds(nx, &cix, &nx) ||
						  check_bounds(ny, &ciy, &ny) ||
						  check_bounds(nz, &ciz, &nz);

	if (bounds_changed && (cix < 0 || cix >= settings.render_distance || 
							ciy < 0 || ciy >= WORLD_HEIGHT || 
							ciz < 0 || ciz >= settings.render_distance)) {
		return true;
	}

	Chunk* neighborChunk = &chunks[cix][ciy][ciz];
	Block currentBlock = chunk->blocks[x][y][z];
	Block neighborBlock = neighborChunk->blocks[nx][ny][nz];

	uint8_t current_type = block_data[currentBlock.id][0];
	bool isCurrentTranslucent = block_data[currentBlock.id][1] == 1;
	bool isNeighborTranslucent = block_data[neighborBlock.id][1] == 1;

	if (current_type == BTYPE_LEAF) {
		if (settings.fancy_graphics) {
			return true;
		} else {
			if (neighborBlock.id == 0) return true;
			if (!isCurrentTranslucent && !isNeighborTranslucent) return false;
			if (isCurrentTranslucent && isNeighborTranslucent) return currentBlock.id != neighborBlock.id;
			return !isCurrentTranslucent;
		}
	}

	if (neighborBlock.id == 0) return true;
	if (!isCurrentTranslucent && !isNeighborTranslucent) return false;
	if (isCurrentTranslucent && isNeighborTranslucent) return currentBlock.id != neighborBlock.id;
	return !isCurrentTranslucent;
}

void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d, uint8_t* x, uint8_t* y, uint8_t* z) {
	switch (face) {
		case 0: *x = u; *y = v; *z = d; break; // Front
		case 1: *x = d; *y = v; *z = u; break; // Left
		case 2: *x = u; *y = v; *z = d; break; // Back
		case 3: *x = d; *y = v; *z = u; break; // Right
		case 4: *x = u; *y = d; *z = v; break; // Bottom
		case 5: *x = u; *y = d; *z = v; break; // Top
	}
}

uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block) {
	uint8_t width = 1;
	uint8_t block_type = block_data[block->id][0];
	bool fancy_leaves = (block_type == BTYPE_LEAF && settings.fancy_graphics);
	
	for (uint8_t du = 1; u + du < CHUNK_SIZE; du++) {
		if (mask[v][u + du]) break;
		uint8_t nx, ny, nz;
		map_coordinates(face, u + du, v, (face == 0 || face == 2) ? z : (face == 1 || face == 3) ? x : y, &nx, &ny, &nz);
		if (nx >= CHUNK_SIZE || ny >= CHUNK_SIZE || nz >= CHUNK_SIZE) break;
		Block* neighbor = &chunk->blocks[nx][ny][nz];
		uint8_t neighbor_type = block_data[neighbor->id][0];
	
		if (neighbor->id != block->id) break;

		if (block_type != BTYPE_REGULAR && block_type != BTYPE_LIQUID && block_type != BTYPE_LEAF) break;
		if (neighbor_type != block_type) break;

		if (!fancy_leaves && !is_face_visible(chunk, nx, ny, nz, face)) break;
		
		width++;
	}
	return width;
}

uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width) {
	uint8_t height = 1;
	uint8_t block_type = block_data[block->id][0];
	bool fancy_leaves = (block_type == BTYPE_LEAF && settings.fancy_graphics);
	
	for (uint8_t dv = 1; v + dv < CHUNK_SIZE; dv++) {
		bool can_extend = true;
		for (uint8_t du = 0; du < width; du++) {
			if (mask[v + dv][u + du]) {
				can_extend = false;
				break;
			}
			uint8_t nx, ny, nz;
			map_coordinates(face, u + du, v + dv, (face == 0 || face == 2) ? z : (face == 1 || face == 3) ? x : y, &nx, &ny, &nz);
			if (nx >= CHUNK_SIZE || ny >= CHUNK_SIZE || nz >= CHUNK_SIZE) {
				can_extend = false;
				break;
			}
			Block* neighbor = &chunk->blocks[nx][ny][nz];
			uint8_t neighbor_type = block_data[neighbor->id][0];

			if (neighbor->id != block->id) {
				can_extend = false;
				break;
			}

			if (block_type != BTYPE_REGULAR && block_type != BTYPE_LIQUID && block_type != BTYPE_LEAF) {
				can_extend = false;
				break;
			}
			if (neighbor_type != block_type) {
				can_extend = false;
				break;
			}
			
			if (!fancy_leaves && !is_face_visible(chunk, nx, ny, nz, face)) {
				can_extend = false;
				break;
			}
		}
		if (!can_extend) break;
		height++;
	}
	return height;
}