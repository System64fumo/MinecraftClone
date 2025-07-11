#include "main.h"
#include "shaders.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Skybox variables
unsigned int skybox_vao, skybox_vbo;
unsigned int skybox_view_loc, skybox_proj_loc, skybox_time_loc;

// Voxel clouds variables
unsigned int clouds_texture;
unsigned int clouds_vao, clouds_vbo, clouds_ebo;
unsigned int clouds_view_loc, clouds_proj_loc, clouds_model_loc;
float clouds_tex_offset_x = 0.0f;
float day_night_time = 0.25f;
mat4 cloud_model;

// Constants
static const float CLOUDS_HEIGHT = 192.0f;
static const float CLOUDS_SPEED = 0.025f;
//static const float DAY_CYCLE_SPEED = 0.00025f;
static const int TEXTURE_SIZE = 128;
static const float VOXEL_SIZE = 32.0f;
static const float VOXEL_HEIGHT_SCALE = 0.5f;

static float *voxel_vertices = NULL;
static unsigned int *voxel_indices = NULL;
static int total_vertices = 0;
static int total_indices = 0;
static uint8_t *texture_data = NULL;

// Simplified skybox vertices (triangle strip)
static const float skybox_vertices[] = {
	-1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f
};

static const int8_t cube_face_data[6][4][6] = {
	// Front (+Z): normal = 0
	{{-1, -1,  1, 0, 0, 0}, { 1, -1,  1, 0, 1, 0}, { 1,  1,  1, 0, 1, 1}, {-1,  1,  1, 0, 0, 1}},
	// Back (-Z): normal = 1  
	{{-1, -1, -1, 1, 1, 0}, {-1,  1, -1, 1, 1, 1}, { 1,  1, -1, 1, 0, 1}, { 1, -1, -1, 1, 0, 0}},
	// Left (-X): normal = 2
	{{-1,  1,  1, 2, 1, 0}, {-1,  1, -1, 2, 1, 1}, {-1, -1, -1, 2, 0, 1}, {-1, -1,  1, 2, 0, 0}},
	// Right (+X): normal = 3
	{{ 1,  1,  1, 3, 0, 0}, { 1, -1,  1, 3, 0, 1}, { 1, -1, -1, 3, 1, 1}, { 1,  1, -1, 3, 1, 0}},
	// Top (+Y): normal = 4
	{{-1,  1, -1, 4, 0, 1}, {-1,  1,  1, 4, 0, 0}, { 1,  1,  1, 4, 1, 0}, { 1,  1, -1, 4, 1, 1}},
	// Bottom (-Y): normal = 5
	{{-1, -1, -1, 5, 1, 1}, { 1, -1, -1, 5, 0, 1}, { 1, -1,  1, 5, 0, 0}, {-1, -1,  1, 5, 1, 0}}
};

static const float face_normals[6][3] = {
	{ 0.0f,  0.0f,  1.0f},  // Front
	{ 0.0f,  0.0f, -1.0f},  // Back
	{-1.0f,  0.0f,  0.0f},  // Left
	{ 1.0f,  0.0f,  0.0f},  // Right
	{ 0.0f,  1.0f,  0.0f},  // Top
	{ 0.0f, -1.0f,  0.0f}   // Bottom
};

static const unsigned int face_indices[6] = {0, 1, 2, 2, 3, 0};

static inline int is_voxel_solid(int x, int y) {
	return (x >= 0 && x < TEXTURE_SIZE && y >= 0 && y < TEXTURE_SIZE) ? 
		   texture_data[y * TEXTURE_SIZE + x] > 0 : 0;
}

static inline int should_render_face(int x, int y, int face) {
	static const int dx[] = {0, 0, -1, 1, 0, 0};
	static const int dy[] = {1, -1, 0, 0, 0, 0};
	
	// Always render top/bottom faces (indices 4 and 5)
	if (face >= 4) return 1;
	
	return !is_voxel_solid(x + dx[face], y + dy[face]);
}

static void generate_clouds_texture(void) {
	texture_data = calloc(TEXTURE_SIZE * TEXTURE_SIZE, sizeof(uint8_t));
	
	// Optimized cluster-based generation
	for (int cy = 0; cy < TEXTURE_SIZE / 4; cy++) {
		for (int cx = 0; cx < TEXTURE_SIZE / 4; cx++) {
			srand(cx * 1000 + cy);
			if (rand() % 100 < 40) {
				// Fill cluster area
				for (int dy = 0; dy < 4; dy++) {
					for (int dx = 0; dx < 4; dx++) {
						int x = cx * 4 + dx;
						int y = cy * 4 + dy;
						if (x < TEXTURE_SIZE && y < TEXTURE_SIZE) {
							srand(x * 1000 + y);
							if (rand() % 100 < 80) {
								texture_data[y * TEXTURE_SIZE + x] = 255;
							}
						}
					}
				}
			}
		}
	}
	
	// Create and upload texture
	glGenTextures(1, &clouds_texture);
	glBindTexture(GL_TEXTURE_2D, clouds_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TEXTURE_SIZE, TEXTURE_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, texture_data);
}

static void generate_voxel_mesh(void) {
	// Count visible faces
	int total_faces = 0;
	for (int y = 0; y < TEXTURE_SIZE; y++) {
		for (int x = 0; x < TEXTURE_SIZE; x++) {
			if (!texture_data[y * TEXTURE_SIZE + x]) continue;
			
			for (uint8_t face = 0; face < 6; face++) {
				if (should_render_face(x, y, face)) {
					total_faces++;
				}
			}
		}
	}
	
	if (total_faces == 0) {
		total_vertices = total_indices = 0;
		return;
	}
	
	total_vertices = total_faces * 4;
	total_indices = total_faces * 6;
	
	voxel_vertices = malloc(total_vertices * 8 * sizeof(float));
	voxel_indices = malloc(total_indices * sizeof(unsigned int));
	
	int vertex_idx = 0, index_idx = 0, current_vertex = 0;
	
	// Generate mesh data
	for (int y = 0; y < TEXTURE_SIZE; y++) {
		for (int x = 0; x < TEXTURE_SIZE; x++) {
			if (!texture_data[y * TEXTURE_SIZE + x]) continue;
			
			float world_x = (x - TEXTURE_SIZE * 0.5f) * VOXEL_SIZE;
			float world_z = (y - TEXTURE_SIZE * 0.5f) * VOXEL_SIZE;
			
			for (uint8_t face = 0; face < 6; face++) {
				if (!should_render_face(x, y, face)) continue;
				
				// Add vertices for this face
				for (uint8_t v = 0; v < 4; v++) {
					int base = vertex_idx * 8;
					const int8_t *face_data = cube_face_data[face][v];
					
					// Position (convert from int8 to world coordinates)
					voxel_vertices[base + 0] = (face_data[0] * 0.5f) * VOXEL_SIZE + world_x;
					voxel_vertices[base + 1] = (face_data[1] * 0.5f * VOXEL_HEIGHT_SCALE) * VOXEL_SIZE + CLOUDS_HEIGHT;
					voxel_vertices[base + 2] = (face_data[2] * 0.5f) * VOXEL_SIZE + world_z;
					
					// Normal (lookup from table)
					int normal_idx = face_data[3];
					voxel_vertices[base + 3] = face_normals[normal_idx][0];
					voxel_vertices[base + 4] = face_normals[normal_idx][1];
					voxel_vertices[base + 5] = face_normals[normal_idx][2];
					
					// Texture coordinates (convert from int8 to float)
					voxel_vertices[base + 6] = (float)face_data[4];
					voxel_vertices[base + 7] = (float)face_data[5];
					
					vertex_idx++;
				}
				
				// Add indices
				for (int i = 0; i < 6; i++) {
					voxel_indices[index_idx++] = face_indices[i] + current_vertex;
				}
				current_vertex += 4;
			}
		}
	}
}

void skybox_init(void) {
	// Initialize skybox
	glGenVertexArrays(1, &skybox_vao);
	glGenBuffers(1, &skybox_vbo);
	glBindVertexArray(skybox_vao);
	glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
	
	// Cache uniform locations
	skybox_view_loc = glGetUniformLocation(skybox_shader, "view");
	skybox_proj_loc = glGetUniformLocation(skybox_shader, "projection");
	skybox_time_loc = glGetUniformLocation(skybox_shader, "time");
	
	// Generate clouds
	generate_clouds_texture();
	generate_voxel_mesh();

	// Setup clouds
	glGenVertexArrays(1, &clouds_vao);
	glGenBuffers(1, &clouds_vbo);
	glGenBuffers(1, &clouds_ebo);
		
	glBindVertexArray(clouds_vao);
	glBindBuffer(GL_ARRAY_BUFFER, clouds_vbo);
	glBufferData(GL_ARRAY_BUFFER, total_vertices * 8 * sizeof(float), voxel_vertices, GL_STATIC_DRAW);
		
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, clouds_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_indices * sizeof(unsigned int), voxel_indices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), NULL);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

	clouds_view_loc = glGetUniformLocation(clouds_shader, "view");
	clouds_proj_loc = glGetUniformLocation(clouds_shader, "projection");
	clouds_model_loc = glGetUniformLocation(clouds_shader, "model");

	matrix4_identity(cloud_model);
}

void update_clouds(void) {
	clouds_tex_offset_x += CLOUDS_SPEED;
	matrix4_translate(cloud_model, clouds_tex_offset_x, 0.0f, 0.0f);
	// TODO: Enable time flow
	//day_night_time = fmodf(day_night_time + DAY_CYCLE_SPEED, 1.0f);
}

void skybox_render(void) {
	// Render skybox
	glUseProgram(skybox_shader);
	glUniformMatrix4fv(skybox_view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(skybox_proj_loc, 1, GL_FALSE, projection);
	glUniform1f(skybox_time_loc, day_night_time);
	
	glBindVertexArray(skybox_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 24);
	draw_calls++;
	
	// Render clouds
	glUseProgram(clouds_shader);
	glUniformMatrix4fv(clouds_view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(clouds_proj_loc, 1, GL_FALSE, projection);
	glUniformMatrix4fv(clouds_model_loc, 1, GL_FALSE, cloud_model);
	glEnable(GL_DEPTH_TEST);
	glBindVertexArray(clouds_vao);
	glDrawElements(GL_TRIANGLES, total_indices, GL_UNSIGNED_INT, NULL);
	draw_calls++;
	
	glBindVertexArray(0);
}

void cleanup_skybox(void) {
	// Cleanup OpenGL objects
	glDeleteVertexArrays(1, &skybox_vao);
	glDeleteBuffers(1, &skybox_vbo);
	glDeleteProgram(skybox_shader);
	
	if (total_vertices > 0) {
		glDeleteVertexArrays(1, &clouds_vao);
		glDeleteBuffers(1, &clouds_vbo);
		glDeleteBuffers(1, &clouds_ebo);
	}
	
	glDeleteTextures(1, &clouds_texture);
	glDeleteProgram(clouds_shader);
	
	// Free memory
	free(voxel_vertices);
	free(voxel_indices);
	free(texture_data);
	
	voxel_vertices = NULL;
	voxel_indices = NULL;
	texture_data = NULL;
}