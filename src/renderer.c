#include "main.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	float x, y, z;
	unsigned char block_id;
	unsigned char face_id;
	unsigned char rotation;
	float tex_x, tex_y;
} Vertex;

void draw_block_highlight(float x, float y, float z) {
	static const float vertices_template[] = {
		// Front face (Z+)
		-0.505f, -0.505f, 0.505f,
		0.505f, -0.505f, 0.505f,
		0.505f, 0.505f, 0.505f,
		-0.505f, 0.505f, 0.505f,
		-0.505f, -0.505f, 0.505f,

		// Back face (Z-)
		-0.505f, -0.505f, -0.505f,
		-0.505f, 0.505f, -0.505f,
		0.505f, 0.505f, -0.505f,
		0.505f, -0.505f, -0.505f,
		-0.505f, -0.505f, -0.505f,

		// Left face (X-)
		-0.505f, -0.505f, 0.505f,
		-0.505f, -0.505f, -0.505f,
		-0.505f, 0.505f, -0.505f,
		-0.505f, 0.505f, 0.505f,
		-0.505f, -0.505f, 0.505f,

		// Right face (X+)
		0.505f, -0.505f, 0.505f,
		0.505f, 0.505f, 0.505f,
		0.505f, 0.505f, -0.505f,
		0.505f, -0.505f, -0.505f,
		0.505f, -0.505f, 0.505f,

		// Bottom face (Y-)
		-0.505f, -0.505f, 0.505f,
		0.505f, -0.505f, 0.505f,
		0.505f, -0.505f, -0.505f,
		-0.505f, -0.505f, -0.505f,
		-0.505f, -0.505f, 0.505f,

		// Top face (Y+)
		-0.505f, 0.505f, 0.505f,
		-0.505f, 0.505f, -0.505f,
		0.505f, 0.505f, -0.505f,
		0.505f, 0.505f, 0.505f,
		-0.505f, 0.505f, 0.505f, 
	};

	static unsigned int vbo = 0, vao = 0;
	if (vbo == 0) {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_template), vertices_template, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}

	float translationMatrix[16];
	matrix4_identity(translationMatrix);
	matrix4_translate(translationMatrix, x - 0.5f, y - 0.5f, z - 0.5f);

	glUseProgram(shaderProgram);

	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, translationMatrix);

	glLineWidth(2f);

	glBindVertexArray(vao);

	for (int i = 0; i < 6; i++) {
		glDrawArrays(GL_LINE_LOOP, i * 5, 5);
	}

	glBindVertexArray(0);
	glUseProgram(0);
}

static bool isFaceVisible(Chunk* chunk, int x, int y, int z, char face) {
	int nx = x, ny = y, nz = z;
	int cix = chunk->ci_x, ciy = chunk->ci_y, ciz = chunk->ci_z;

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

	// Check if adjacent block is air
	return neighborChunk->blocks[nx][ny][nz].id == 0;
}

void pre_process_chunk(Chunk* chunk) {
	const size_t max_vertices = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;
	const size_t max_indices = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6;
	
	// Use stack allocation for small arrays to avoid malloc overhead
	Vertex vertices[max_vertices];
	unsigned int indices[max_indices];

	unsigned int vertex_count = 0;
	unsigned int index_count = 0;

	bool mask[CHUNK_SIZE][CHUNK_SIZE] = {0};

	// Process each face direction
	for (int face = 0; face < 6; face++) {
		memset(mask, 0, sizeof(mask));

		// Sweep through each layer
		for (int d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			// Try to find rectangles in this slice
			for (int v = 0; v < CHUNK_SIZE; v++) {
				for (int u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					// Map coordinates based on face direction
					int x = 0, y = 0, z = 0;
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
					int width = 1;
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
					int height = 1;
					bool can_extend = true;
					while (v + height < CHUNK_SIZE && can_extend) {
						for (int dx = 0; dx < width; dx++) {
							int next_x = x, next_y = y, next_z = z;
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
					for (int dy = 0; dy < height; dy++) {
						for (int dx = 0; dx < width; dx++) {
							mask[v + dy][u + dx] = true;
						}
					}

					// Generate vertices for the merged rectangle
					unsigned int base_vertex = vertex_count;
					float x1 = x, x2 = x, y1 = y, y2 = y + height, z1 = z, z2 = z;

					if (face == 3) {
						z2 += width;
					} 
					else if (face == 1) {
						x1 += 1;
						z2 += width;
					} 
					else {
						x2 += (face == 0 || face == 2 || face >= 4 ? width : 1);
						y2 = y + (face >= 4 ? 1 : height);
						z2 += (face == 0 || face == 2 ? 1 : (face >= 4 ? height : 1));
					}

					// In the face generation section of pre_process_chunk:
					float width_blocks = (face == 1 || face == 3) ? 1.0f : width;
					float height_blocks = (face >= 4) ? 1.0f : height;
					float depth_blocks = (face == 0 || face == 2) ? 1.0f : (face >= 4 ? height : width);


					switch (face) {
						case 0: // Front (Z+)
							vertices[vertex_count++] = (Vertex){x2, y2, z2, block_textures[block->id][face], face, block->rotations[face], width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y2, z2, block_textures[block->id][face], face, block->rotations[face], 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y1, z2, block_textures[block->id][face], face, block->rotations[face], 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x2, y1, z2, block_textures[block->id][face], face, block->rotations[face], width_blocks, height_blocks};
							break;
						case 1: // Left (X-)
							vertices[vertex_count++] = (Vertex){x1, y2, z1, block_textures[block->id][face], face, block->rotations[face], depth_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y2, z2, block_textures[block->id][face], face, block->rotations[face], 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x1, y1, z2, block_textures[block->id][face], face, block->rotations[face], 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x1, y1, z1, block_textures[block->id][face], face, block->rotations[face], depth_blocks, height_blocks};
							break;
						case 2: // Back (Z-)
							vertices[vertex_count++] = (Vertex){x1, y2, z1, block_textures[block->id][face], face, block->rotations[face], width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z1, block_textures[block->id][face], face, block->rotations[face], 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z1, block_textures[block->id][face], face, block->rotations[face], 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x1, y1, z1, block_textures[block->id][face], face, block->rotations[face], width_blocks, height_blocks};
							break;
						case 3: // Right (X+)
							vertices[vertex_count++] = (Vertex){x2, y2, z2, block_textures[block->id][face], face, block->rotations[face], depth_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z1, block_textures[block->id][face], face, block->rotations[face], 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z1, block_textures[block->id][face], face, block->rotations[face], 0.0f, height_blocks};
							vertices[vertex_count++] = (Vertex){x2, y1, z2, block_textures[block->id][face], face, block->rotations[face], depth_blocks, height_blocks};
							break;
						case 4: // Bottom (Y-)
							vertices[vertex_count++] = (Vertex){x1, y1, z1, block_textures[block->id][face], face, block->rotations[face], width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z1, block_textures[block->id][face], face, block->rotations[face], 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y1, z2, block_textures[block->id][face], face, block->rotations[face], 0.0f, depth_blocks};
							vertices[vertex_count++] = (Vertex){x1, y1, z2, block_textures[block->id][face], face, block->rotations[face], width_blocks, depth_blocks};
							break;
						case 5: // Top (Y+)
							vertices[vertex_count++] = (Vertex){x1, y2, z2, block_textures[block->id][face], face, block->rotations[face], width_blocks, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z2, block_textures[block->id][face], face, block->rotations[face], 0.0f, 0.0f};
							vertices[vertex_count++] = (Vertex){x2, y2, z1, block_textures[block->id][face], face, block->rotations[face], 0.0f, depth_blocks};
							vertices[vertex_count++] = (Vertex){x1, y2, z1, block_textures[block->id][face], face, block->rotations[face], width_blocks, depth_blocks};
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
	}

	chunk->vertex_count = vertex_count;
	chunk->index_count = index_count;

	// Delete existing buffers if they exist
	if (chunk->VBO) {
		glDeleteBuffers(1, &chunk->VBO);
		chunk->VBO = 0;
	}
	if (chunk->EBO) {
		glDeleteBuffers(1, &chunk->EBO);
		chunk->EBO = 0;
	}
	if (chunk->VAO) {
		glDeleteVertexArrays(1, &chunk->VAO);
		chunk->VAO = 0;
	}

	// Only create new buffers if there are vertices to process
	if (vertex_count > 0) {
		glGenVertexArrays(1, &chunk->VAO);
		glGenBuffers(1, &chunk->VBO);
		glGenBuffers(1, &chunk->EBO);

		glBindVertexArray(chunk->VAO);

		glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, block_id));
		glEnableVertexAttribArray(1);

		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, face_id));
		glEnableVertexAttribArray(2);

		glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), (void*)offsetof(Vertex, rotation));
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tex_x));
		glEnableVertexAttribArray(4);

		glBindVertexArray(0);
		chunk->needs_update = false;
	}
}

void render_chunk(Chunk* chunk, unsigned int shaderProgram, float* model) {
	if (chunk->vertex_count == 0) return;

	glBindVertexArray(chunk->VAO);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
	glDrawElements(GL_TRIANGLES, chunk->index_count, GL_UNSIGNED_INT, 0);
}

void render_chunks() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_BAKE);
	#endif
	for (unsigned char x = 0; x < RENDERR_DISTANCE; x++) {
		for (unsigned char y = 0; y < WORLD_HEIGHT; y++) {
			for (unsigned char z = 0; z < RENDERR_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];

				if (chunk->needs_update) {
					pre_process_chunk(chunk);
					chunk->needs_update = false;
				}
			}
		}
	}
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_BAKE);
	#endif

	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER);
	#endif
	for (unsigned char x = 0; x < RENDERR_DISTANCE; x++) {
		for (unsigned char y = 0; y < WORLD_HEIGHT; y++) {
			for (unsigned char z = 0; z < RENDERR_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				matrix4_identity(model);
				matrix4_translate(model, 
					chunk->x * CHUNK_SIZE, 
					chunk->y * CHUNK_SIZE, 
					chunk->z * CHUNK_SIZE
				);
				render_chunk(chunk, shaderProgram, model);
			}
		}
	}
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER);
	#endif
}
