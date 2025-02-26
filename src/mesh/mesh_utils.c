#include "main.h"

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

	// Check if neighbor is out of chunk bounds
	// Check bounds
	bool bounds_changed = check_bounds(nx, &cix, &nx) ||
						  check_bounds(ny, &ciy, &ny) ||
						  check_bounds(nz, &ciz, &nz);

	if (bounds_changed && (cix < 0 || cix >= RENDER_DISTANCE || 
						   ciy < 0 || ciy >= WORLD_HEIGHT || 
						   ciz < 0 || ciz >= RENDER_DISTANCE)) {
		return true;
	}

	Chunk* neighborChunk = &chunks[cix][ciy][ciz];
	Block currentBlock = chunk->blocks[x][y][z];
	Block neighborBlock = neighborChunk->blocks[nx][ny][nz];

	bool isCurrentTranslucent = block_data[currentBlock.id][1] == 1;
	bool isNeighborTranslucent = block_data[neighborBlock.id][1] == 1;

	if (neighborBlock.id == 0) return true;
	if (!isCurrentTranslucent && !isNeighborTranslucent) return false;
	if (isCurrentTranslucent && isNeighborTranslucent) return currentBlock.id != neighborBlock.id;
	return !isCurrentTranslucent;
}

void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d, uint8_t* x, uint8_t* y, uint8_t* z) {
	if (face == 1 || face == 3) { *x = d; *y = v; *z = u; }  // Left/Right
	else if (face >= 4) { *x = u; *y = d; *z = v; }  // Top/Bottom
	else { *x = u; *y = v; *z = d; }  // Front/Back
}

uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block) {
	uint8_t width = 1;
	while (u + width < CHUNK_SIZE) {
		uint8_t next_x = x, next_z = z;

		// Determine the next coordinates based on the face
		if (face == 0 || face == 2 || face >= 4) next_x = x + width;  // Front/Back/Top/Bottom
		if (face == 1 || face == 3) next_z = z + width;			   // Left/Right

		// Check if the next block is valid
		if (mask[v][u + width] ||
			chunk->blocks[next_x][y][next_z].id != block->id ||
			!is_face_visible(chunk, next_x, y, next_z, face)) {
			break;
		}
		width++;
	}
	return width;
}

uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width) {
	uint8_t height = 1;
	while (v + height < CHUNK_SIZE) {
		bool can_extend = true;

		for (uint8_t dx = 0; dx < width; dx++) {
			uint8_t next_x = x, next_y = y, next_z = z;

			// Determine the next coordinates based on the face
			if (face == 0 || face == 2) {  // Front/Back
				next_x = x + dx;
				next_y = y + height;
			} else if (face == 1 || face == 3) {  // Left/Right
				next_z = z + dx;
				next_y = y + height;
			} else if (face >= 4) {  // Top/Bottom
				next_x = x + dx;
				next_z = z + height;
			}

			// Check if the next block is valid
			if (mask[v + height][u + dx] ||
				chunk->blocks[next_x][next_y][next_z].id != block->id ||
				!is_face_visible(chunk, next_x, next_y, next_z, face)) {
				can_extend = false;
				break;
			}
		}

		if (!can_extend) break;
		height++;
	}
	return height;
}