#include "main.h"

bool should_render_face(Chunk* chunk, unsigned char x, unsigned char y, unsigned char z, unsigned char face) {
	if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
		return false;
	}

	switch(face) {
		case 0: // Front (Z-)
			if (z == 0) {
				return chunk->neighbors[1] ? chunk->neighbors[1]->blocks[x][y][CHUNK_SIZE-1].id == 0 : true;
			}
			return chunk->blocks[x][y][z-1].id == 0;
		case 1: // Back (Z+)
			if (z == CHUNK_SIZE-1) {
				return chunk->neighbors[0] ? chunk->neighbors[0]->blocks[x][y][0].id == 0 : true;
			}
			return chunk->blocks[x][y][z+1].id == 0;
		case 2: // Left (X-)
			if (x == 0) {
				return chunk->neighbors[3] ? chunk->neighbors[3]->blocks[CHUNK_SIZE-1][y][z].id == 0 : true;
			}
			return chunk->blocks[x-1][y][z].id == 0;
		case 3: // Right (X+)
			if (x == CHUNK_SIZE-1) {
				return chunk->neighbors[2] ? chunk->neighbors[2]->blocks[0][y][z].id == 0 : true;
			}
			return chunk->blocks[x+1][y][z].id == 0;
		case 4: // Top (Y+)
			if (y == CHUNK_SIZE-1) {
				return chunk->neighbors[4] ? chunk->neighbors[4]->blocks[x][0][z].id == 0 : true;
			}
			return chunk->blocks[x][y+1][z].id == 0;
		case 5: // Bottom (Y-)
			if (y == 0) {
				return chunk->neighbors[5] ? chunk->neighbors[5]->blocks[x][CHUNK_SIZE-1][z].id == 0 : true;
			}
			return chunk->blocks[x][y-1][z].id == 0;
	}
	return false;
}

void bake_chunk(Chunk* chunk) {
	if (!chunk->vertices) {
		chunk->vertices = (float*)malloc(MAX_VERTICES * 3 * sizeof(float));	// xyz per vertex
	}
	if (!chunk->colors) {
		chunk->colors = (float*)malloc(MAX_VERTICES * 3 * sizeof(float));	// rgb per vertex
	}

	int vertex_count = 0;

	for(int x = 0; x < CHUNK_SIZE; x++) {
		for(int y = 0; y < CHUNK_SIZE; y++) {
			for(int z = 0; z < CHUNK_SIZE; z++) {
				if(chunk->blocks[x][y][z].id != 0) {
					float wx = chunk->x + x;
					float wy = chunk->y + y;
					float wz = chunk->z + z;

					// Set color based on block ID
					float r, g, b;
					switch(chunk->blocks[x][y][z].id) {
						case 1: // Dirt
							r = 0.6f; g = 0.4f; b = 0.2f;
							break;
						case 2: // Grass
							r = 0.2f; g = 0.8f; b = 0.2f;
							break;
						case 3: // Stone
							r = 0.5f; g = 0.5f; b = 0.5f;
							break;
						default:
							r = 1.0f; g = 1.0f; b = 1.0f;
					}

					// Front face (Z-)
					if(should_render_face(chunk, x, y, z, 0)) {
						for(int i = 0; i < 4; i++) {
							chunk->colors[vertex_count*3] = r;
							chunk->colors[vertex_count*3+1] = g;
							chunk->colors[vertex_count*3+2] = b;
							chunk->vertices[vertex_count*3] = wx + (i == 1 || i == 2);
							chunk->vertices[vertex_count*3+1] = wy + (i == 2 || i == 3);
							chunk->vertices[vertex_count*3+2] = wz;
							vertex_count++;
						}
					}

					// Back face (Z+)
					if(should_render_face(chunk, x, y, z, 1)) {
						for(int i = 0; i < 4; i++) {
							chunk->colors[vertex_count*3] = r;
							chunk->colors[vertex_count*3+1] = g;
							chunk->colors[vertex_count*3+2] = b;
							chunk->vertices[vertex_count*3] = wx + (i == 3 || i == 2);
							chunk->vertices[vertex_count*3+1] = wy + (i == 1 || i == 2);
							chunk->vertices[vertex_count*3+2] = wz + 1;
							vertex_count++;
						}
					}

					// Left face (X-)
					if(should_render_face(chunk, x, y, z, 2)) {
						for(int i = 0; i < 4; i++) {
							chunk->colors[vertex_count*3] = r * 0.8f;
							chunk->colors[vertex_count*3+1] = g * 0.8f;
							chunk->colors[vertex_count*3+2] = b * 0.8f;
							chunk->vertices[vertex_count*3] = wx;
							chunk->vertices[vertex_count*3+1] = wy + (i == 1 || i == 2);
							chunk->vertices[vertex_count*3+2] = wz + (i == 2 || i == 3);
							vertex_count++;
						}
					}

					// Right face (X+)
					if(should_render_face(chunk, x, y, z, 3)) {
						for(int i = 0; i < 4; i++) {
							chunk->colors[vertex_count*3] = r * 0.8f;
							chunk->colors[vertex_count*3+1] = g * 0.8f;
							chunk->colors[vertex_count*3+2] = b * 0.8f;
							chunk->vertices[vertex_count*3] = wx + 1;
							chunk->vertices[vertex_count*3+1] = wy + (i == 2 || i == 3);
							chunk->vertices[vertex_count*3+2] = wz + (i == 1 || i == 2);
							vertex_count++;
						}
					}

					// Top face (Y+)
					if(should_render_face(chunk, x, y, z, 4)) {
						for(int i = 0; i < 4; i++) {
							chunk->colors[vertex_count*3] = r * 1.2f;
							chunk->colors[vertex_count*3+1] = g * 1.2f;
							chunk->colors[vertex_count*3+2] = b * 1.2f;
							chunk->vertices[vertex_count*3] = wx + (i == 1 || i == 2);
							chunk->vertices[vertex_count*3+1] = wy + 1;
							chunk->vertices[vertex_count*3+2] = wz + (i == 2 || i == 3);
							vertex_count++;
						}
					}

					// Bottom face (Y-)
					if(should_render_face(chunk, x, y, z, 5)) {
						for(int i = 0; i < 4; i++) {
							chunk->colors[vertex_count*3] = r * 0.6f;
							chunk->colors[vertex_count*3+1] = g * 0.6f;
							chunk->colors[vertex_count*3+2] = b * 0.6f;
							chunk->vertices[vertex_count*3] = wx + (i == 2 || i == 3);
							chunk->vertices[vertex_count*3+1] = wy;
							chunk->vertices[vertex_count*3+2] = wz + (i == 1 || i == 2);
							vertex_count++;
						}
					}
				}
			}
		}
	}

	chunk->vertex_count = vertex_count;

	// Update VBOs
	gl_gen_buffers(1, &chunk->vbo);
	gl_gen_buffers(1, &chunk->color_vbo);

	gl_bind_buffer(GL_ARRAY_BUFFER, chunk->vbo);
	gl_buffer_data(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), chunk->vertices, GL_STATIC_DRAW);

	gl_bind_buffer(GL_ARRAY_BUFFER, chunk->color_vbo);
	gl_buffer_data(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), chunk->colors, GL_STATIC_DRAW);

	chunk->needs_update = false;
}