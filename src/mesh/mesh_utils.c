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

// Returns a packed light byte (high nibble=sky, low nibble=block) for the face
// of the block at (x,y,z) facing direction 'face', reading from the exposed neighbor.
uint8_t get_face_light(Chunk* chunk, int x, int y, int z, uint8_t face) {
	static const int8_t face_ndx[] = { 0,  1,  0, -1,  0,  0};
	static const int8_t face_ndy[] = { 0,  0,  0,  0, -1,  1};
	static const int8_t face_ndz[] = { 1,  0, -1,  0,  0,  0};

	int nlx = x + face_ndx[face];
	int nly = y + face_ndy[face];
	int nlz = z + face_ndz[face];

	// Own block's light as fallback (for emitters)
	uint8_t own = chunk->blocks[x][y][z].light_level;

	uint8_t neighbor_light = 0;
	if (nlx >= 0 && nlx < CHUNK_SIZE &&
		nly >= 0 && nly < CHUNK_SIZE &&
		nlz >= 0 && nlz < CHUNK_SIZE) {
		neighbor_light = chunk->blocks[nlx][nly][nlz].light_level;
	} else {
		// Cross-chunk lookup
		int8_t cix = chunk->ci_x, ciy = chunk->ci_y, ciz = chunk->ci_z;
		int8_t bx2 = nlx, by2 = nly, bz2 = nlz;
		if (nlx < 0)			{ cix--; bx2 = CHUNK_SIZE - 1; }
		else if (nlx >= CHUNK_SIZE) { cix++; bx2 = 0; }
		if (nly < 0)			{ ciy--; by2 = CHUNK_SIZE - 1; }
		else if (nly >= CHUNK_SIZE) { ciy++; by2 = 0; }
		if (nlz < 0)			{ ciz--; bz2 = CHUNK_SIZE - 1; }
		else if (nlz >= CHUNK_SIZE) { ciz++; bz2 = 0; }

		if (cix >= 0 && cix < settings.render_distance &&
			ciy >= 0 && ciy < WORLD_HEIGHT &&
			ciz >= 0 && ciz < settings.render_distance) {
			Chunk* nc = &chunks[cix][ciy][ciz];
			if (nc->is_loaded)
				neighbor_light = nc->blocks[bx2][by2][bz2].light_level;
			else
				neighbor_light = 0xF0; // unloaded = full sky
		} else {
			neighbor_light = 0xF0; // world edge = full sky
		}
	}

	// Take max of each channel independently
	uint8_t sky	 = SKY_LIGHT(neighbor_light);
	uint8_t blk	 = BLOCK_LIGHT(neighbor_light);
	uint8_t own_sky = SKY_LIGHT(own);
	uint8_t own_blk = BLOCK_LIGHT(own);
	if (own_sky > sky) sky = own_sky;
	if (own_blk > blk) blk = own_blk;
	return (sky << 4) | blk;
}

uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block) {
	uint8_t width = 1;
	uint8_t block_type = block_data[block->id][0];
	bool fancy_leaves = (block_type == BTYPE_LEAF && settings.fancy_graphics);
	uint8_t base_light = get_face_light(chunk, x, y, z, face);

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
		if (get_face_light(chunk, nx, ny, nz, face) != base_light) break;

		width++;
	}
	return width;
}

uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width) {
	uint8_t height = 1;
	uint8_t block_type = block_data[block->id][0];
	bool fancy_leaves = (block_type == BTYPE_LEAF && settings.fancy_graphics);
	uint8_t base_light = get_face_light(chunk, x, y, z, face);

	for (uint8_t dv = 1; v + dv < CHUNK_SIZE; dv++) {
		bool can_extend = true;
		for (uint8_t du = 0; du < width; du++) {
			if (mask[v + dv][u + du]) { can_extend = false; break; }
			uint8_t nx, ny, nz;
			map_coordinates(face, u + du, v + dv, (face == 0 || face == 2) ? z : (face == 1 || face == 3) ? x : y, &nx, &ny, &nz);
			if (nx >= CHUNK_SIZE || ny >= CHUNK_SIZE || nz >= CHUNK_SIZE) { can_extend = false; break; }
			Block* neighbor = &chunk->blocks[nx][ny][nz];
			uint8_t neighbor_type = block_data[neighbor->id][0];

			if (neighbor->id != block->id) { can_extend = false; break; }
			if (block_type != BTYPE_REGULAR && block_type != BTYPE_LIQUID && block_type != BTYPE_LEAF) { can_extend = false; break; }
			if (neighbor_type != block_type) { can_extend = false; break; }
			if (!fancy_leaves && !is_face_visible(chunk, nx, ny, nz, face)) { can_extend = false; break; }
			if (get_face_light(chunk, nx, ny, nz, face) != base_light) { can_extend = false; break; }
		}
		if (!can_extend) break;
		height++;
	}
	return height;
}