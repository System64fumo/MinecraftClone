#include "main.h"

void draw_block_highlight(float x, float y, float z) {
	static const float vertices_template[] = {
		// Front face (Z-)
		-1.01f, -1.01f, -1.01f,
		0.01f, -1.01f, -1.01f,
		0.01f, 0.01f, -1.01f,
		-1.01f, 0.01f, -1.01f,
		
		// Back face (Z+)
		-1.01f, -1.01f, 0.01f,
		-1.01f, 0.01f, 0.01f,
		0.01f, 0.01f, 0.01f,
		0.01f, -1.01f, 0.01f,
		
		// Left face (X-)
		-1.01f, -1.01f, -1.01f,
		-1.01f, 0.01f, -1.01f,
		-1.01f, 0.01f, 0.01f,
		-1.01f, -1.01f, 0.01f,
		
		// Right face (X+)
		0.01f, -1.01f, -1.01f,
		0.01f, -1.01f, 0.01f,
		0.01f, 0.01f, 0.01f,
		0.01f, 0.01f, -1.01f,
		
		// Top face (Y+)
		-1.01f, 0.01f, -1.01f,
		0.01f, 0.01f, -1.01f,
		0.01f, 0.01f, 0.01f,
		-1.01f, 0.01f, 0.01f,
		
		// Bottom face (Y-)
		-1.01f, -1.01f, -1.01f,
		-1.01f, -1.01f, 0.01f,
		0.01f, -1.01f, 0.01f,
		0.01f, -1.01f, -1.01f
	};

	static unsigned int vbo = 0;
	float vertices[72];

	if (!vbo) {
		gl_gen_buffers(1, &vbo);
		gl_bind_buffer(GL_ARRAY_BUFFER, vbo);
		gl_buffer_data(GL_ARRAY_BUFFER, sizeof(vertices_template), vertices_template, GL_STATIC_DRAW);
	}

	// Translate vertices
	for (int i = 0; i < 72; i += 3) {
		vertices[i] = vertices_template[i] + x;
		vertices[i + 1] = vertices_template[i + 1] + y;
		vertices[i + 2] = vertices_template[i + 2] + z;
	}

	gl_bind_buffer(GL_ARRAY_BUFFER, vbo);
	gl_buffer_data(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

	glVertexPointer(3, GL_FLOAT, 0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);

	glDrawArrays(GL_QUADS, 0, 24);

	glDisableClientState(GL_VERTEX_ARRAY);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);
}

bool should_render_face(Chunk* chunk, unsigned char x, unsigned char y, unsigned char z, unsigned char face) {
	if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) {
		return false;
	}

	// Get current chunk coordinates
	int cx = chunk->x / (CHUNK_SIZE/2);
	int cy = chunk->y / (CHUNK_SIZE/2);
	int cz = chunk->z / (CHUNK_SIZE/2);

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
				if (cz < CHUNKS_Z-1 && chunks[cx][cy][cz+1].vbo != 0) {
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
				if (cx < CHUNKS_X-1 && chunks[cx+1][cy][cz].vbo != 0) {
					return chunks[cx+1][cy][cz].blocks[0][y][z].id == 0;
				}
				return true;
			}
			return chunk->blocks[x+1][y][z].id == 0;
			
		case 4: // Top (Y+)
			if (y == CHUNK_SIZE-1) {
				if (cy < CHUNKS_Y-1 && chunks[cx][cy+1][cz].vbo != 0) {
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
		chunk->vertices = (float*)malloc(MAX_VERTICES * 3 * sizeof(float));
	}
	if (!chunk->colors) {
		chunk->colors = (float*)malloc(MAX_VERTICES * 3 * sizeof(float));
	}

	int vertex_count = 0;
	const float face_shading[] = {1.0f, 1.0f, 0.8f, 0.8f, 1.2f, 0.6f};

	// For each face direction
	for(int face = 0; face < 6; face++) {
		// Create mask of visible faces
		bool mask[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE] = {{{false}}};
		
		// Fill mask with visible faces
		for(int x = 0; x < CHUNK_SIZE; x++) {
			for(int y = 0; y < CHUNK_SIZE; y++) {
				for(int z = 0; z < CHUNK_SIZE; z++) {
					if(chunk->blocks[x][y][z].id != 0 && should_render_face(chunk, x, y, z, face)) {
						mask[x][y][z] = true;
					}
				}
			}
		}

		// Greedy meshing algorithm
		for(int x = 0; x < CHUNK_SIZE; x++) {
			for(int y = 0; y < CHUNK_SIZE; y++) {
				for(int z = 0; z < CHUNK_SIZE; z++) {
					if(mask[x][y][z]) {
						int block_id = chunk->blocks[x][y][z].id;
						
						// Set base color
						float r, g, b;
						switch(block_id) {
							case 1: r = 0.6f; g = 0.4f; b = 0.2f; break;  // Dirt
							case 2: r = 0.2f; g = 0.8f; b = 0.2f; break;  // Grass
							case 3: r = 0.5f; g = 0.5f; b = 0.5f; break;  // Stone
							default: r = g = b = 1.0f;
						}

						// Find width
						int width = 1;
						while(x + width < CHUNK_SIZE && 
							  mask[x + width][y][z] && 
							  chunk->blocks[x + width][y][z].id == block_id) {
							width++;
						}

						// Find height
						int height = 1;
						bool canExpand = true;
						while(y + height < CHUNK_SIZE && canExpand) {
							for(int dx = 0; dx < width; dx++) {
								if(!mask[x + dx][y + height][z] || 
								   chunk->blocks[x + dx][y + height][z].id != block_id) {
									canExpand = false;
									break;
								}
							}
							if(canExpand) height++;
						}

						// Clear the mask for merged faces
						for(int dx = 0; dx < width; dx++) {
							for(int dy = 0; dy < height; dy++) {
								mask[x + dx][y + dy][z] = false;
							}
						}

						float shade = face_shading[face];
						float cr = r * shade;
						float cg = g * shade;
						float cb = b * shade;

						float wx = chunk->x + x;
						float wy = chunk->y + y;
						float wz = chunk->z + z;

						// Generate vertices for merged face
						float vertices[12];
						switch(face) {
							case 0: // Front
								vertices[0] = wx;        vertices[1] = wy;         vertices[2] = wz;
								vertices[3] = wx + width;vertices[4] = wy;         vertices[5] = wz;
								vertices[6] = wx + width;vertices[7] = wy + height;vertices[8] = wz;
								vertices[9] = wx;        vertices[10]= wy + height;vertices[11]= wz;
								break;
							case 1: // Back
								vertices[0] = wx + width;vertices[1] = wy;         vertices[2] = wz + 1;
								vertices[3] = wx;        vertices[4] = wy;         vertices[5] = wz + 1;
								vertices[6] = wx;        vertices[7] = wy + height;vertices[8] = wz + 1;
								vertices[9] = wx + width;vertices[10]= wy + height;vertices[11]= wz + 1;
								break;
							case 2: // Left
								vertices[0] = wx;        vertices[1] = wy;         vertices[2] = wz + 1;
								vertices[3] = wx;        vertices[4] = wy;         vertices[5] = wz;
								vertices[6] = wx;        vertices[7] = wy + height;vertices[8] = wz;
								vertices[9] = wx;        vertices[10]= wy + height;vertices[11]= wz + 1;
								break;
							case 3: // Right
								vertices[0] = wx + 1;    vertices[1] = wy;         vertices[2] = wz;
								vertices[3] = wx + 1;    vertices[4] = wy;         vertices[5] = wz + 1;
								vertices[6] = wx + 1;    vertices[7] = wy + height;vertices[8] = wz + 1;
								vertices[9] = wx + 1;    vertices[10]= wy + height;vertices[11]= wz;
								break;
							case 4: // Top
								vertices[0] = wx;        vertices[1] = wy + 1;     vertices[2] = wz;
								vertices[3] = wx + width;vertices[4] = wy + 1;     vertices[5] = wz;
								vertices[6] = wx + width;vertices[7] = wy + 1;     vertices[8] = wz + 1;
								vertices[9] = wx;        vertices[10]= wy + 1;     vertices[11]= wz + 1;
								break;
							case 5: // Bottom
								vertices[0] = wx;        vertices[1] = wy;         vertices[2] = wz + 1;
								vertices[3] = wx + width;vertices[4] = wy;         vertices[5] = wz + 1;
								vertices[6] = wx + width;vertices[7] = wy;         vertices[8] = wz;
								vertices[9] = wx;        vertices[10]= wy;         vertices[11]= wz;
								break;
						}

						// Add vertices and colors
						for(int i = 0; i < 4; i++) {
							chunk->vertices[vertex_count*3] = vertices[i*3];
							chunk->vertices[vertex_count*3+1] = vertices[i*3+1];
							chunk->vertices[vertex_count*3+2] = vertices[i*3+2];
							
							chunk->colors[vertex_count*3] = cr;
							chunk->colors[vertex_count*3+1] = cg;
							chunk->colors[vertex_count*3+2] = cb;
							
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