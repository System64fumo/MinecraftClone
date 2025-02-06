#include "main.h"

// Helper function to check if a block face should be rendered
bool should_render_face(Chunk* chunk, int x, int y, int z, int face) {
	if (x < 0 || y < 0 || z < 0 || x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
		return false;
	}

	switch(face) {
		case 0: return z == 0            || chunk->blocks[x][y][z-1].id == 0;	// Front
		case 1: return z == CHUNK_SIZE-1 || chunk->blocks[x][y][z+1].id == 0;	// Back
		case 2: return x == 0            || chunk->blocks[x-1][y][z].id == 0;	// Left
		case 3: return x == CHUNK_SIZE-1 || chunk->blocks[x+1][y][z].id == 0;	// Right
		case 4: return y == CHUNK_SIZE-1 || chunk->blocks[x][y+1][z].id == 0;	// Top
		case 5: return y == 0            || chunk->blocks[x][y-1][z].id == 0;	// Bottom
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
	if (chunk->vbo == 0) {
		gl_gen_buffers(1, &chunk->vbo);
	}
	if (chunk->color_vbo == 0) {
		gl_gen_buffers(1, &chunk->color_vbo);
	}

	gl_bind_buffer(GL_ARRAY_BUFFER, chunk->vbo);
	gl_buffer_data(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), chunk->vertices, GL_STATIC_DRAW);

	gl_bind_buffer(GL_ARRAY_BUFFER, chunk->color_vbo);
	gl_buffer_data(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), chunk->colors, GL_STATIC_DRAW);

	chunk->needs_update = false;
}