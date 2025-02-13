#include "main.h"
#include <stdlib.h>

typedef struct {
	float x, y, z;
	unsigned char block_id;
	unsigned char face_id;
} Vertex;

static const float FACE_VERTICES[6][4][3] = {
	{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}}, // Front face (Z+)
	{{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, // Right face (X+)
	{{0.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}}, // Back face (Z-)
	{{0.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}, // Left face (X-)
	{{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}, // Bottom face (Y-)
	{{0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}} // Top face (Y+)
};

Vec3 faceNormals[6] = {
	{ 1.0f,  0.0f,  0.0f }, // +X Right
	{-1.0f,  0.0f,  0.0f }, // -X Left
	{ 0.0f,  1.0f,  0.0f }, // +Y Top
	{ 0.0f, -1.0f,  0.0f }, // -Y Bottom
	{ 0.0f,  0.0f,  1.0f }, // +Z Back
	{ 0.0f,  0.0f, -1.0f }  // -Z Front
};

static bool isFaceVisible(Chunk* chunk, int x, int y, int z, char face) {
	int nx = x, ny = y, nz = z; // Neighbor block coordinates

	switch (face) {
		case 0: nz += 1; break; // Front
		case 1: nx += 1; break; // Left
		case 2: nz -= 1; break; // Back
		case 3: nx -= 1; break; // Right
		case 4: ny -= 1; break; // Bottom
		case 5: ny += 1; break; // Top
		default: return false;  // Invalid face
	}

	// If neighbor is outside chunk bounds, assume it's air (visible face)
	if (nx < 0 || nx >= 16 || ny < 0 || ny >= 16 || nz < 0 || nz >= 16) {
		return true;
	}

	// Check if adjacent block is air
	return chunk->blocks[nx][ny][nz].id == 0;
}

void pre_process_chunk(Chunk* chunk) {
	if (chunk->VAO == 0) {
		glGenVertexArrays(1, &chunk->VAO);
		glGenBuffers(1, &chunk->VBO);
		glGenBuffers(1, &chunk->EBO);
	}

	const size_t max_vertices = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;
	const size_t max_indices = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 6;
	
	Vertex* vertices = malloc(max_vertices * sizeof(Vertex));
	unsigned int* indices = malloc(max_indices * sizeof(unsigned int));
	
	unsigned int vertex_count = 0;
	unsigned int index_count = 0;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				Block* block = &chunk->blocks[x][y][z];
				if (block->id == 0) continue;  // Skip air blocks

				for (int face = 0; face < 6; face++) {
					// Check if this face needs to be rendered
					if (!isFaceVisible(chunk, x, y, z, face)) {
						continue;
					}

					unsigned int base_vertex = vertex_count;
					
					// Add vertices for this face
					for (int v = 0; v < 4; v++) {
						vertices[vertex_count].x = x + FACE_VERTICES[face][v][0];
						vertices[vertex_count].y = y + FACE_VERTICES[face][v][1];
						vertices[vertex_count].z = z + FACE_VERTICES[face][v][2];
						vertices[vertex_count].block_id = block->id;
						vertices[vertex_count].face_id = face;  // Pass the face ID
						vertex_count++;
					}

					// Add indices for the face triangles (CCW winding)
					indices[index_count++] = base_vertex;     // First triangle
					indices[index_count++] = base_vertex + 1;
					indices[index_count++] = base_vertex + 2;
					
					indices[index_count++] = base_vertex;     // Second triangle
					indices[index_count++] = base_vertex + 2;
					indices[index_count++] = base_vertex + 3;
				}
			}
		}
	}

	chunk->vertex_count = vertex_count;
	chunk->index_count = index_count;

	if (vertex_count > 0) {
		glBindVertexArray(chunk->VAO);
		
		glBindBuffer(GL_ARRAY_BUFFER, chunk->VBO);
		glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), 
					vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunk->EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(unsigned int), 
					indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribIPointer(1, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), 
							  (void*)offsetof(Vertex, block_id));
		glEnableVertexAttribArray(1);

		glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE, sizeof(Vertex), 
							  (void*)offsetof(Vertex, face_id)); // Pass faceID to shader
		glEnableVertexAttribArray(2);
	}

	free(vertices);
	free(indices);
}

void render_chunk(Chunk* chunk, unsigned int shaderProgram, float* model) {
	if (chunk->vertex_count == 0) return;

	glBindVertexArray(chunk->VAO);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, model);
	glDrawElements(GL_TRIANGLES, chunk->index_count, GL_UNSIGNED_INT, 0);
}

void render_chunks() {
	for (unsigned char x = 0; x < RENDERR_DISTANCE; x++) {
		for (unsigned char y = 0; y < WORLD_HEIGHT; y++) {
			for (unsigned char z = 0; z < RENDERR_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				
				if (chunk->needs_update) {
					pre_process_chunk(chunk);
					chunk->needs_update = false;
				}

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
}
