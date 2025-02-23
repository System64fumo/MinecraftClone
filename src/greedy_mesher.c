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

void generate_cross_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	// First diagonal plane
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 1.0f, 0, block_data[block->id][2+0], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 0.0f, 0, block_data[block->id][2+0], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 0.0f, 0, block_data[block->id][2+0], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 1.0f, 0, block_data[block->id][2+0], 1, 1};

	// Back face of first plane
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 0.0f, 2, block_data[block->id][2+2], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 1.0f, 2, block_data[block->id][2+2], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 1.0f, 2, block_data[block->id][2+2], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 0.0f, 2, block_data[block->id][2+2], 1, 1};

	// Second diagonal plane
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 1.0f, 1, block_data[block->id][2+1], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 0.0f, 1, block_data[block->id][2+1], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 0.0f, 1, block_data[block->id][2+1], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 1.0f, 1, block_data[block->id][2+1], 1, 1};

	// Back face of second plane
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y + 1, z + 0.0f, 3, block_data[block->id][2+3], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y + 1, z + 1.0f, 3, block_data[block->id][2+3], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 0.0f, y, z + 1.0f, 3, block_data[block->id][2+3], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 1.0f, y, z + 0.0f, 3, block_data[block->id][2+3], 1, 1};
}

void generate_slab_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	float height = 0.5f;
	
	// Generate vertices for a half-height block
	// Top face
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 5, block_data[block->id][2+5], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 5, block_data[block->id][2+5], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 5, block_data[block->id][2+5], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 5, block_data[block->id][2+5], 1, 1};

	// Bottom face
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 4, block_data[block->id][2+4], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 4, block_data[block->id][2+4], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 4, block_data[block->id][2+4], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 4, block_data[block->id][2+4], 1, 1};

	// Front face
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 0, block_data[block->id][2+0], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 0, block_data[block->id][2+0], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 0, block_data[block->id][2+0], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 0, block_data[block->id][2+0], 1, 1};

	// Back face
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 2, block_data[block->id][2+2], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 2, block_data[block->id][2+2], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 2, block_data[block->id][2+2], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 2, block_data[block->id][2+2], 1, 1};

	// Left face
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z + 1, 1, block_data[block->id][2+1], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x, y + height, z, 1, block_data[block->id][2+1], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x, y, z, 1, block_data[block->id][2+1], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x, y, z + 1, 1, block_data[block->id][2+1], 1, 1};

	// Right face
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z, 3, block_data[block->id][2+3], 1, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y + height, z + 1, 3, block_data[block->id][2+3], 0, 0};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z + 1, 3, block_data[block->id][2+3], 0, 1};
	vertices[(*vertex_count)++] = (Vertex){x + 1, y, z, 3, block_data[block->id][2+3], 1, 1};
}

void generate_vertices(uint8_t face, float x, float y, float z, uint8_t width, uint8_t height, Block* block, Vertex vertices[], uint32_t* vertex_count) {
	uint8_t width_blocks = (face == 1 || face == 3) ? 1 : width;
	uint8_t height_blocks = (face >= 4) ? 1 : height;
	uint8_t depth_blocks = (face == 0 || face == 2) ? 1 : (face >= 4 ? height : width);

	float x1 = x, x2 = x, y1 = y, y2 = y + height, z1 = z, z2 = z;

	if (face == 3 || face == 1) {
		z2 += width;
		if (face == 1) x1 += 1;
	} else {
		x2 += (face == 0 || face == 2 || face >= 4) ? width : 1;
		y2 = y + (face >= 4 ? 1 : height);
		z2 += (face == 0 || face == 2) ? 1 : (face >= 4 ? height : 1);
	}

	uint8_t block_face_data = block_data[block->id][2+face];

	switch (face) {
		case 0: // Front (Z+)
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, block_face_data, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, block_face_data, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, block_face_data, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, block_face_data, width_blocks, height_blocks};
			break;
		case 1: // Left (X-)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, block_face_data, depth_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, block_face_data, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, block_face_data, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, block_face_data, depth_blocks, height_blocks};
			break;
		case 2: // Back (Z-)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, block_face_data, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, block_face_data, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, block_face_data, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, block_face_data, width_blocks, height_blocks};
			break;
		case 3: // Right (X+)
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, block_face_data, depth_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, block_face_data, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, block_face_data, 0.0f, height_blocks};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, block_face_data, depth_blocks, height_blocks};
			break;
		case 4: // Bottom (Y-)
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z1, face, block_face_data, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z1, face, block_face_data, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y1, z2, face, block_face_data, 0.0f, depth_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y1, z2, face, block_face_data, width_blocks, depth_blocks};
			break;
		case 5: // Top (Y+)
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z2, face, block_face_data, width_blocks, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z2, face, block_face_data, 0.0f, 0.0f};
			vertices[(*vertex_count)++] = (Vertex){x2, y2, z1, face, block_face_data, 0.0f, depth_blocks};
			vertices[(*vertex_count)++] = (Vertex){x1, y2, z1, face, block_face_data, width_blocks, depth_blocks};
			break;
	}
}

void generate_indices(uint32_t base_vertex, uint32_t indices[], uint32_t* index_count) {
	indices[(*index_count)++] = base_vertex;
	indices[(*index_count)++] = base_vertex + 1;
	indices[(*index_count)++] = base_vertex + 2;
	indices[(*index_count)++] = base_vertex;
	indices[(*index_count)++] = base_vertex + 2;
	indices[(*index_count)++] = base_vertex + 3;
}
