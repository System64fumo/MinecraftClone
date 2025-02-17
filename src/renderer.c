#include "main.h"
#include <stdlib.h>
#include <string.h>

uint32_t max_vertices = 98304; // CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;

typedef struct {
	uint8_t x, y, z;
	uint8_t block_id;
	uint8_t face_id;
	uint8_t tex_x, tex_y;
} Vertex;

static bool isFaceVisible(Chunk* chunk, int8_t x, int8_t y, int8_t z, uint8_t face) {
	int8_t nx = x, ny = y, nz = z;
	uint8_t cix = chunk->ci_x, ciy = chunk->ci_y, ciz = chunk->ci_z;

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
	if (nx < 0) {
		cix -= 1; nx = CHUNK_SIZE - 1;
	} else if (nx >= CHUNK_SIZE) {
		cix += 1; nx = 0;
	}

	if (ny < 0) {
		ciy -= 1; ny = CHUNK_SIZE - 1;
	} else if (ny >= CHUNK_SIZE) {
		ciy += 1; ny = 0;
	}

	if (nz < 0) {
		ciz -= 1; nz = CHUNK_SIZE - 1;
	} else if (nz >= CHUNK_SIZE) {
		ciz += 1; nz = 0;
	}

	// Ensure chunk indices are within bounds
	if (cix < 0 || cix >= RENDERR_DISTANCE || 
		ciy < 0 || ciy >= WORLD_HEIGHT || 
		ciz < 0 || ciz >= RENDERR_DISTANCE) {
		return true; // Assume out-of-bounds chunks are air
	}

	// Get the neighboring chunk
	Chunk* neighborChunk = &chunks[cix][ciy][ciz];

	// Check if adjacent block is translucent
	return neighborChunk->blocks[nx][ny][nz].id == 0;
}

void pre_process_chunk(Chunk* chunk) {
	bool mask[CHUNK_SIZE][CHUNK_SIZE] = {0};

	// Process each face direction
	for (uint8_t face = 0; face < 6; face++) {
		Vertex vertices[max_vertices];
		uint32_t indices[max_vertices];
		uint16_t vertex_count = 0;
		uint16_t index_count = 0;
		// Sweep through each layer
		for (uint8_t d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			// Try to find rectangles in this slice
			for (uint8_t v = 0; v < CHUNK_SIZE; v++) {
				for (uint8_t u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					// Map coordinates based on face direction
					uint8_t x = 0, y = 0, z = 0;
					switch (face) {
						case 0: x = u; y = v; z = d; break; // Front face (Z+)
						case 1: x = d; y = v; z = u; break; // Left face (X-)
						case 2: x = u; y = v; z = d; break; // Back face (Z-)
						case 3: x = d; y = v; z = u; break; // Right face (X+)
						case 4: x = u; y = d; z = v; break; // Bottom face (Y-)
						case 5: x = u; y = d; z = v; break; // Top face (Y+)
					}

					Block* block = &chunk->blocks[x][y][z];
					if (block->id == 0 || !isFaceVisible(chunk, x, y, z, face)) {
						continue;
					}

					// Find width of rectangle (extend in u direction)
					uint8_t width = 1;
					while (u + width < CHUNK_SIZE) {
						int next_x = x, next_y = y, next_z = z;
						switch (face) {
							case 0: case 2: next_x = x + width; break;  // Front/Back
							case 1: case 3: next_z = z + width; break;  // Left/Right
							case 4: case 5: next_x = x + width; break;  // Top/Bottom
						}

						if (mask[v][u + width] ||
							chunk->blocks[next_x][next_y][next_z].id != block->id ||
							!isFaceVisible(chunk, next_x, next_y, next_z, face)) {
							break;
						}
						width++;
					}

					// Find height of rectangle (extend in v direction)
					uint8_t height = 1;
					bool can_extend = true;
					while (v + height < CHUNK_SIZE && can_extend) {
						for (uint8_t dx = 0; dx < width; dx++) {
							uint8_t next_x = x, next_y = y, next_z = z;
							switch (face) {
								case 0: case 2:  // Front/Back
									next_x = x + dx;
									next_y = y + height;
									break;
								case 1: case 3:  // Left/Right
									next_z = z + dx;
									next_y = y + height;
									break;
								case 4: case 5:  // Top/Bottom
									next_x = x + dx;
									next_z = z + height;
									break;
							}

							if (mask[v + height][u + dx] ||
								chunk->blocks[next_x][next_y][next_z].id != block->id ||
								!isFaceVisible(chunk, next_x, next_y, next_z, face)) {
								can_extend = false;
								break;
							}
						}
						if (can_extend) height++;
					}

					// Mark all blocks in the rectangle as processed
					for (uint8_t dy = 0; dy < height; dy++) {
						for (uint8_t dx = 0; dx < width; dx++) {
							mask[v + dy][u + dx] = true;
						}
					}

					// Generate vertices for the merged rectangle
					uint16_t base_vertex = vertex_count;
					uint8_t x1 = x, x2 = x, y1 = y, y2 = y + height, z1 = z, z2 = z;

					if (face == 3 || face == 1) {
						z2 += width;
						if (face == 1)
							x1 += 1;
					}
					else {
						x2 += (face == 0 || face == 2 || face >= 4) ? width : 1;
						y2 = y + (face >= 4 ? 1 : height);
						z2 += (face == 0 || face == 2) ? 1 : (face >= 4 ? height : 1);
					}

					// In the face generation section of pre_process_chunk:
					uint8_t width_blocks = (face == 1 || face == 3) ? 1.0f : width;
					uint8_t height_blocks = (face >= 4) ? 1.0f : height;
					uint8_t depth_blocks = (face == 0 || face == 2) ? 1.0f : (face >= 4 ? height : width);

					switch (face) {
						case 0: // Front (Z+)
							vertices[vertex_count++] = (Vertex){x2, y2, z2, block_textures[block->id][face], face, width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y2, z2, block_textures[block->id][face], face, 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y1, z2, block_textures[block->id][face], face, 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x2, y1, z2, block_textures[block->id][face], face, width_blocks, height_blocks};
							break;
						case 1: // Left (X-)
							vertices[vertex_count++] = (Vertex){x1, y2, z1, block_textures[block->id][face], face, depth_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y2, z2, block_textures[block->id][face], face, 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y1, z2, block_textures[block->id][face], face, 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x1, y1, z1, block_textures[block->id][face], face, depth_blocks, height_blocks};
							break;
						case 2: // Back (Z-)
							vertices[vertex_count++] = (Vertex){x1, y2, z1, block_textures[block->id][face], face, width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z1, block_textures[block->id][face], face, 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z1, block_textures[block->id][face], face, 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x1, y1, z1, block_textures[block->id][face], face, width_blocks, height_blocks};
							break;
						case 3: // Right (X+)
							vertices[vertex_count++] = (Vertex){x2, y2, z2, block_textures[block->id][face], face, depth_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z1, block_textures[block->id][face], face, 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z1, block_textures[block->id][face], face, 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x2, y1, z2, block_textures[block->id][face], face, depth_blocks, height_blocks};
							break;
						case 4: // Bottom (Y-)
							vertices[vertex_count++] = (Vertex){x1, y1, z1, block_textures[block->id][face], face, width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z1, block_textures[block->id][face], face, 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z2, block_textures[block->id][face], face, 0.0f, depth_blocks};
							vertices[vertex_count++] = (Vertex){x1, y1, z2, block_textures[block->id][face], face, width_blocks, depth_blocks};
							break;
						case 5: // Top (Y+)
							vertices[vertex_count++] = (Vertex){x1, y2, z2, block_textures[block->id][face], face, width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z2, block_textures[block->id][face], face, 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z1, block_textures[block->id][face], face, 0.0f, depth_blocks};
							vertices[vertex_count++] = (Vertex){x1, y2, z1, block_textures[block->id][face], face, width_blocks, depth_blocks};
							break;
					}

					// Add indices for the merged rectangle
					indices[index_count++] = base_vertex;
					indices[index_count++] = base_vertex + 1;
					indices[index_count++] = base_vertex + 2;
					indices[index_count++] = base_vertex;
					indices[index_count++] = base_vertex + 2;
					indices[index_count++] = base_vertex + 3;
				}
			}
		}

		Face* current_face = &chunk->faces[face];
        current_face->vertex_count = vertex_count;
        current_face->index_count = index_count;
		// Delete existing buffers if they exist
		if (current_face->VBO) {
			glDeleteBuffers(1, &current_face->VBO);
			current_face->VBO = 0;
		}
		if (current_face->EBO) {
			glDeleteBuffers(1, &current_face->EBO);
			current_face->EBO = 0;
		}
		if (current_face->VAO) {
			glDeleteVertexArrays(1, &current_face->VAO);
			current_face->VAO = 0;
		}

		// Only create new buffers if there are vertices to process
		if (vertex_count > 0) {
			glGenVertexArrays(1, &current_face->VAO);
			glGenBuffers(1, &current_face->VBO);
			glGenBuffers(1, &current_face->EBO);

			glBindVertexArray(current_face->VAO);

			glBindBuffer(GL_ARRAY_BUFFER, current_face->VBO);
			glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, current_face->EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint64_t), indices, GL_STATIC_DRAW);

			glVertexAttribPointer(0, 3, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(0);

			glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, block_id));
			glEnableVertexAttribArray(1);

			glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
			glEnableVertexAttribArray(2);

			glVertexAttribPointer(3, 2, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_x));
			glEnableVertexAttribArray(3);

			glBindVertexArray(0);
		}
	}
	chunk->needs_update = false;
}

void render_chunks() {
	GLint modelUniformLocation = glGetUniformLocation(shaderProgram, "model");
	static bool baking = false;
	for (uint8_t x = 0; x < RENDERR_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDERR_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];

				if (chunk->needs_update) {
					#ifdef DEBUG
					if (!baking) {
						profiler_start(PROFILER_ID_BAKE);
						baking = true;
					}
					#endif
					pre_process_chunk(chunk);
					chunk->needs_update = false;
				}
			}
		}
	}
	#ifdef DEBUG
	if (baking) {
		profiler_stop(PROFILER_ID_BAKE);
		baking = false;
	}
	#endif

	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER);
	#endif
	for (uint8_t x = 0; x < RENDERR_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDERR_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
					
				matrix4_identity(model);
				matrix4_translate(model,
					chunk->x * CHUNK_SIZE, 
					chunk->y * CHUNK_SIZE, 
					chunk->z * CHUNK_SIZE);

				for (uint8_t face = 0; face < 6; face++) {
					if (frustum_faces[face])
						continue;

					Face* current_face = &chunk->faces[face];
					if (current_face->vertex_count == 0) continue;

					glBindVertexArray(current_face->VAO);
					glUniformMatrix4fv(modelUniformLocation, 1, GL_FALSE, model);
					glDrawElements(GL_TRIANGLES, current_face->index_count, GL_UNSIGNED_INT, 0);
				}
			}
		}
	}
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER);
	#endif
}
