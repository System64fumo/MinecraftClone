#include "main.h"

void draw_block_highlight(float x, float y, float z) {
	static const float vertices_template[] = {
		// Front face (Z-)
		-1.01f, -1.01f, -1.01f,
		0.01f, -1.01f, -1.01f,
		0.01f, 0.01f, -1.01f,
		-1.01f, 0.01f, -1.01f,
		-1.01f, -1.01f, -1.01f,
		
		// Back face (Z+)
		-1.01f, -1.01f, 0.01f,
		-1.01f, 0.01f, 0.01f,
		0.01f, 0.01f, 0.01f,
		0.01f, -1.01f, 0.01f,
		-1.01f, -1.01f, 0.01f,
		
		// Left face (X-)
		-1.01f, -1.01f, -1.01f,
		-1.01f, 0.01f, -1.01f,
		-1.01f, 0.01f, 0.01f,
		-1.01f, -1.01f, 0.01f,
		-1.01f, -1.01f, -1.01f,
		
		// Right face (X+)
		0.01f, -1.01f, -1.01f,
		0.01f, -1.01f, 0.01f,
		0.01f, 0.01f, 0.01f,
		0.01f, 0.01f, -1.01f,
		0.01f, -1.01f, -1.01f,
		
		// Top face (Y+)
		-1.01f, 0.01f, -1.01f,
		0.01f, 0.01f, -1.01f,
		0.01f, 0.01f, 0.01f,
		-1.01f, 0.01f, 0.01f,
		-1.01f, 0.01f, -1.01f,
		
		// Bottom face (Y-)
		-1.01f, -1.01f, -1.01f,
		-1.01f, -1.01f, 0.01f,
		0.01f, -1.01f, 0.01f,
		0.01f, -1.01f, -1.01f,
		-1.01f, -1.01f, -1.01f
	};

	static unsigned int vbo = 0;
	float vertices[90];  // Increased size to accommodate closing vertices

	if (!vbo) {
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_template), vertices_template, GL_STATIC_DRAW);
	}

	// Translate vertices
	for (int i = 0; i < 90; i += 3) {
		vertices[i] = vertices_template[i] + x;
		vertices[i + 1] = vertices_template[i + 1] + y;
		vertices[i + 2] = vertices_template[i + 2] + z;
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glColor3f(1.0f, 1.0f, 1.0f);

	glVertexPointer(3, GL_FLOAT, 0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Draw each face as a separate line loop
	for (int i = 0; i < 6; i++) {
		glDrawArrays(GL_LINE_LOOP, i * 5, 5);
	}

	glDisableClientState(GL_VERTEX_ARRAY);
}

bool should_render_face(Chunk* chunk, unsigned char x, unsigned char y, unsigned char z, unsigned char face) {
	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
		return false;
	}

	// Get current chunk coordinates
	int cx = chunk->ci_x;
	int cy = chunk->ci_y;
	int cz = chunk->ci_z;

	switch(face) {
		case 0: // Front (Z-)
			if (z == 0) {
				if (cz > 0 && chunks[cx][cy][cz-1].vbo != 0) {
					return chunks[cx][cy][cz-1].blocks[x][y][CHUNK_SIZE-1].id == 0;
				}
				return true;
			}
			return chunk->blocks[x][y][z-1].id == 0;
			
		case 1: // Back (Z+)
			if (z == CHUNK_SIZE-1) {
				if (cz < RENDERR_DISTANCE-1 && chunks[cx][cy][cz+1].vbo != 0) {
					return chunks[cx][cy][cz+1].blocks[x][y][0].id == 0;
				}
				return true;
			}
			return chunk->blocks[x][y][z+1].id == 0;
			
		case 2: // Left (X-)
			if (x == 0) {
				if (cx > 0 && chunks[cx-1][cy][cz].vbo != 0) {
					return chunks[cx-1][cy][cz].blocks[CHUNK_SIZE-1][y][z].id == 0;
				}
				return true;
			}
			return chunk->blocks[x-1][y][z].id == 0;
			
		case 3: // Right (X+)
			if (x == CHUNK_SIZE-1) {
				if (cx < RENDERR_DISTANCE-1 && chunks[cx+1][cy][cz].vbo != 0) {
					return chunks[cx+1][cy][cz].blocks[0][y][z].id == 0;
				}
				return true;
			}
			return chunk->blocks[x+1][y][z].id == 0;
			
		case 4: // Top (Y+)
			if (y == CHUNK_SIZE-1) {
				if (cy < WORLD_HEIGHT-1 && chunks[cx][cy+1][cz].vbo != 0) {
					return chunks[cx][cy+1][cz].blocks[x][0][z].id == 0;
				}
				return true;
			}
			return chunk->blocks[x][y+1][z].id == 0;
			
		case 5: // Bottom (Y-)
			if (y == 0) {
				if (cy > 0 && chunks[cx][cy-1][cz].vbo != 0) {
					return chunks[cx][cy-1][cz].blocks[x][CHUNK_SIZE-1][z].id == 0;
				}
				return true;
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

	// Free old VBOs if they exist
	if (chunk->vbo != 0) {
		glDeleteBuffers(1, &chunk->vbo);
	}
	if (chunk->color_vbo != 0) {
		glDeleteBuffers(1, &chunk->color_vbo);
	}

	// Update VBOs
	glGenBuffers(1, &chunk->vbo);
	glGenBuffers(1, &chunk->color_vbo);

	glBindBuffer(GL_ARRAY_BUFFER, chunk->vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), chunk->vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, chunk->color_vbo);
	glBufferData(GL_ARRAY_BUFFER, vertex_count * 3 * sizeof(float), chunk->colors, GL_STATIC_DRAW);

	// Free the CPU-side buffers
	free(chunk->vertices);
	free(chunk->colors);
	chunk->vertices = NULL;
	chunk->colors = NULL;

	chunk->needs_update = false;
}

void render_chunks() {
	// First pass: bake any chunks that need updating
	#ifdef DEBUG
	profiler_start(PROFILER_ID_BAKE);
	#endif
	for(int cx = 0; cx < RENDERR_DISTANCE; cx++) {
		for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
			for(int cz = 0; cz < RENDERR_DISTANCE; cz++) {
				if (chunks[cx][cy][cz].needs_update) {
					bake_chunk(&chunks[cx][cy][cz]);

					// Re-Render neighboring chunks
					if (cz > 0 && chunks[cx][cy][cz-1].vbo != 0) {
						bake_chunk(&chunks[cx][cy][cz-1]);
					}
					if (cz < RENDERR_DISTANCE-1 && chunks[cx][cy][cz+1].vbo != 0) {
						bake_chunk(&chunks[cx][cy][cz+1]);
					}
							
					if (cx > 0 && chunks[cx-1][cy][cz].vbo != 0) {
						bake_chunk(&chunks[cx-1][cy][cz]);
					}
					if (cx < RENDERR_DISTANCE-1 && chunks[cx+1][cy][cz].vbo != 0) {
						bake_chunk(&chunks[cx+1][cy][cz]);
					}

					if (cy > 0 && chunks[cx][cy-1][cz].vbo != 0) {
						bake_chunk(&chunks[cx][cy-1][cz]);
					}
					if (cy < WORLD_HEIGHT-1 && chunks[cx][cy+1][cz].vbo != 0) {
						bake_chunk(&chunks[cx][cy+1][cz]);
					}
				}
			}
		}
	}
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_BAKE);
	#endif

	// Second pass: batch render all chunks
	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER);
	#endif
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	for(int cx = 0; cx < RENDERR_DISTANCE; cx++) {
		for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
			for(int cz = 0; cz < RENDERR_DISTANCE; cz++) {
				if (chunks[cx][cy][cz].vertex_count > 0) {
					glPushMatrix();
					glTranslatef(chunks[cx][cy][cz].x * (CHUNK_SIZE - 1), 
							    chunks[cx][cy][cz].y * (CHUNK_SIZE - 1), 
							    chunks[cx][cy][cz].z * (CHUNK_SIZE - 1));

					glBindBuffer(GL_ARRAY_BUFFER, chunks[cx][cy][cz].vbo);
					glVertexPointer(3, GL_FLOAT, 0, 0);

					glBindBuffer(GL_ARRAY_BUFFER, chunks[cx][cy][cz].color_vbo);
					glColorPointer(3, GL_FLOAT, 0, 0);

					glDrawArrays(GL_QUADS, 0, chunks[cx][cy][cz].vertex_count);

					glPopMatrix();
				}
			}
		}
	}

	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER);
	#endif
}