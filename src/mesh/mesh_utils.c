#include "main.h"
#include "config.h"

bool are_all_neighbors_loaded(uint8_t x, uint8_t y, uint8_t z) {
	if (x > 0                           && !chunks[x-1][y][z].is_loaded) return false;
	if (x < settings.render_distance-1  && !chunks[x+1][y][z].is_loaded) return false;
	if (y > 0                           && !chunks[x][y-1][z].is_loaded) return false;
	if (y < WORLD_HEIGHT-1              && !chunks[x][y+1][z].is_loaded) return false;
	if (z > 0                           && !chunks[x][y][z-1].is_loaded) return false;
	if (z < settings.render_distance-1  && !chunks[x][y][z+1].is_loaded) return false;
	return true;
}

static inline bool check_bounds(int8_t val, int8_t *ci, int8_t *coord) {
	if (val < 0)          { *ci -= 1; *coord = CHUNK_SIZE - 1; return true; }
	if (val >= CHUNK_SIZE){ *ci += 1; *coord = 0;              return true; }
	return false;
}

bool is_face_visible(Chunk *chunk, int8_t x, int8_t y, int8_t z, uint8_t face) {
	int8_t nx = x, ny = y, nz = z;
	int8_t cix = chunk->ci_x, ciy = chunk->ci_y, ciz = chunk->ci_z;

	switch (face) {
		case 0: nz++; break;
		case 1: nx++; break;
		case 2: nz--; break;
		case 3: nx--; break;
		case 4: ny--; break;
		case 5: ny++; break;
		default: return false;
	}

	bool oob = check_bounds(nx, &cix, &nx) |
	           check_bounds(ny, &ciy, &ny) |
	           check_bounds(nz, &ciz, &nz);

	if (oob && (cix < 0 || cix >= settings.render_distance ||
	            ciy < 0 || ciy >= WORLD_HEIGHT ||
	            ciz < 0 || ciz >= settings.render_distance))
		return true;

	Chunk *nb = &chunks[cix][ciy][ciz];
	Block  cb = chunk->blocks[x][y][z];
	Block  nb_blk = nb->blocks[nx][ny][nz];

	uint8_t cur_type  = block_data[cb.id][0];
	bool    cur_trans = block_data[cb.id][1] == 1;
	bool    nb_trans  = block_data[nb_blk.id][1] == 1;

	if (cur_type == BTYPE_LEAF) {
		if (settings.fancy_graphics) return true;
		if (nb_blk.id == 0) return true;
		if (!cur_trans && !nb_trans) return false;
		if (cur_trans && nb_trans) return cb.id != nb_blk.id;
		return !cur_trans;
	}

	if (nb_blk.id == 0) return true;
	if (!cur_trans && !nb_trans) return false;
	uint8_t nb_type = block_data[nb_blk.id][0];
	if (cur_trans && nb_trans) {
		if (cur_type == BTYPE_LIQUID && nb_type == BTYPE_LIQUID) return false;
		return cb.id != nb_blk.id;
	}
	return !cur_trans;
}

void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d,
                     uint8_t *x, uint8_t *y, uint8_t *z) {
	switch (face) {
		case 0: *x = u; *y = v; *z = d; break;
		case 1: *x = d; *y = v; *z = u; break;
		case 2: *x = u; *y = v; *z = d; break;
		case 3: *x = d; *y = v; *z = u; break;
		case 4: *x = u; *y = d; *z = v; break;
		case 5: *x = u; *y = d; *z = v; break;
	}
}

uint8_t get_face_light(Chunk *chunk, int x, int y, int z, uint8_t face) {
	static const int8_t ndx[] = { 0,  1,  0, -1,  0,  0 };
	static const int8_t ndy[] = { 0,  0,  0,  0, -1,  1 };
	static const int8_t ndz[] = { 1,  0, -1,  0,  0,  0 };

	int nlx = x + ndx[face];
	int nly = y + ndy[face];
	int nlz = z + ndz[face];

	uint8_t own = chunk->blocks[x][y][z].light_level;
	uint8_t neighbor_light;

	if (nlx >= 0 && nlx < CHUNK_SIZE &&
	    nly >= 0 && nly < CHUNK_SIZE &&
	    nlz >= 0 && nlz < CHUNK_SIZE) {
		neighbor_light = chunk->blocks[nlx][nly][nlz].light_level;
	} else {
		int8_t cix = chunk->ci_x, ciy = chunk->ci_y, ciz = chunk->ci_z;
		int8_t bx = nlx, by = nly, bz = nlz;
		if (nlx < 0)          { cix--; bx = CHUNK_SIZE - 1; }
		else if (nlx >= CHUNK_SIZE) { cix++; bx = 0; }
		if (nly < 0)          { ciy--; by = CHUNK_SIZE - 1; }
		else if (nly >= CHUNK_SIZE) { ciy++; by = 0; }
		if (nlz < 0)          { ciz--; bz = CHUNK_SIZE - 1; }
		else if (nlz >= CHUNK_SIZE) { ciz++; bz = 0; }

		if (cix >= 0 && cix < settings.render_distance &&
		    ciy >= 0 && ciy < WORLD_HEIGHT &&
		    ciz >= 0 && ciz < settings.render_distance) {
			Chunk *nc = &chunks[cix][ciy][ciz];
			neighbor_light = nc->is_loaded ? nc->blocks[bx][by][bz].light_level : 0xF0;
		} else {
			neighbor_light = 0xF0;
		}
	}

	uint8_t sky = SKY_LIGHT(neighbor_light),  blk = BLOCK_LIGHT(neighbor_light);
	uint8_t osk = SKY_LIGHT(own),             obl = BLOCK_LIGHT(own);
	if (osk > sky) sky = osk;
	if (obl > blk) blk = obl;
	return (sky << 4) | blk;
}

uint8_t find_width(Chunk *chunk, uint8_t face, uint8_t u, uint8_t v,
                   uint8_t x, uint8_t y, uint8_t z,
                   bool mask[CHUNK_SIZE][CHUNK_SIZE], Block *block) {
	uint8_t width      = 1;
	uint8_t block_type = block_data[block->id][0];
	bool    fancy_leaf = (block_type == BTYPE_LEAF && settings.fancy_graphics);
	uint8_t base_light = get_face_light(chunk, x, y, z, face);
	uint8_t d_coord    = (face == 0 || face == 2) ? z : (face == 1 || face == 3) ? x : y;

	for (uint8_t du = 1; u + du < CHUNK_SIZE; du++) {
		if (mask[v][u + du]) break;
		uint8_t nx, ny, nz;
		map_coordinates(face, u + du, v, d_coord, &nx, &ny, &nz);
		if (nx >= CHUNK_SIZE || ny >= CHUNK_SIZE || nz >= CHUNK_SIZE) break;
		Block  *nb      = &chunk->blocks[nx][ny][nz];
		uint8_t nb_type = block_data[nb->id][0];
		if (nb->id != block->id) break;
		if (block_type != BTYPE_REGULAR && block_type != BTYPE_LIQUID && block_type != BTYPE_LEAF) break;
		if (nb_type != block_type) break;
		if (!fancy_leaf && !is_face_visible(chunk, nx, ny, nz, face)) break;
		if (get_face_light(chunk, nx, ny, nz, face) != base_light) break;
		width++;
	}
	return width;
}

uint8_t find_height(Chunk *chunk, uint8_t face, uint8_t u, uint8_t v,
                    uint8_t x, uint8_t y, uint8_t z,
                    bool mask[CHUNK_SIZE][CHUNK_SIZE], Block *block, uint8_t width) {
	uint8_t height     = 1;
	uint8_t block_type = block_data[block->id][0];
	bool    fancy_leaf = (block_type == BTYPE_LEAF && settings.fancy_graphics);
	uint8_t base_light = get_face_light(chunk, x, y, z, face);
	uint8_t d_coord    = (face == 0 || face == 2) ? z : (face == 1 || face == 3) ? x : y;

	for (uint8_t dv = 1; v + dv < CHUNK_SIZE; dv++) {
		bool ok = true;
		for (uint8_t du = 0; du < width && ok; du++) {
			if (mask[v + dv][u + du]) { ok = false; break; }
			uint8_t nx, ny, nz;
			map_coordinates(face, u + du, v + dv, d_coord, &nx, &ny, &nz);
			if (nx >= CHUNK_SIZE || ny >= CHUNK_SIZE || nz >= CHUNK_SIZE) { ok = false; break; }
			Block  *nb      = &chunk->blocks[nx][ny][nz];
			uint8_t nb_type = block_data[nb->id][0];
			if (nb->id != block->id) { ok = false; break; }
			if (block_type != BTYPE_REGULAR && block_type != BTYPE_LIQUID && block_type != BTYPE_LEAF) { ok = false; break; }
			if (nb_type != block_type) { ok = false; break; }
			if (!fancy_leaf && !is_face_visible(chunk, nx, ny, nz, face)) { ok = false; break; }
			if (get_face_light(chunk, nx, ny, nz, face) != base_light) { ok = false; break; }
		}
		if (!ok) break;
		height++;
	}
	return height;
}
