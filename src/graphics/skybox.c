#include "main.h"
#include "shaders.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Skybox variables
unsigned int skyboxTexture;
unsigned int skyboxVAO, skyboxVBO;
GLuint view_loc, proj_loc;

// Clouds variables
unsigned int cloudsTexture;
unsigned int cloudsVAO, cloudsVBO;
GLuint clouds_view_loc, clouds_proj_loc, clouds_model_loc;
float cloudsTexOffsetX = 0.0f;
float cloudsTexOffsetZ = 0.0f;
const float CLOUDS_HEIGHT = 192.0f;
const float CLOUDS_SPEED = 0.1f;
const float CLOUDS_QUAD_SIZE = 500.0f;

float skyboxVertices[] = {
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,

	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,

	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f
};

float cloudsVertices[] = {
	-1.0f,  0.0f, -1.0f,  0.0f, 1.0f,
	 1.0f,  0.0f, -1.0f,  1.0f, 1.0f,
	-1.0f,  0.0f,  1.0f,  0.0f, 0.0f,
	 1.0f,  0.0f,  1.0f,  1.0f, 0.0f,
};

void generate_clouds_texture() {
	const int width = 16;
	const int height = 16;
	uint8_t *data = malloc(width * height * 4); // RGBA

	// Clear to fully transparent
	memset(data, 0, width * height * 4);

	// Create big pixel clouds
	const int CLOUD_PIXEL_SIZE = 1; // Size of each cloud "pixel" in texture pixels
	const int PIXEL_SPACING = 1;   // Space between cloud pixels
	
	for (int y = 0; y < height; y += PIXEL_SPACING) {
		for (int x = 0; x < width; x += PIXEL_SPACING) {
			// 25% chance to place a cloud pixel
			if (rand() % 100 < 25) {
				for (int dy = 0; dy < CLOUD_PIXEL_SIZE; dy++) {
					for (int dx = 0; dx < CLOUD_PIXEL_SIZE; dx++) {
						int px = x + dx;
						int py = y + dy;
						
						if (px < width && py < height) {
							int idx = (py * width + px) * 4;
							data[idx + 0] = 255; // R
							data[idx + 1] = 255; // G
							data[idx + 2] = 255; // B
							data[idx + 3] = 200; // Fixed semi-transparent alpha
						}
					}
				}
			}
		}
	}


	glGenTextures(1, &cloudsTexture);
	glBindTexture(GL_TEXTURE_2D, cloudsTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	
	free(data);
}

void skybox_init() {
	// Initialize skybox
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	view_loc = glGetUniformLocation(skybox_shader, "view");
	proj_loc = glGetUniformLocation(skybox_shader, "projection");

	glGenTextures(1, &skyboxTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	uint8_t size = 16;
	uint8_t skybox_data[6][size][size][3];

	for (uint8_t y = 0; y < size; y++) {
		float vertical_progress;
		uint8_t transition_width = 2;
		
		if (y < size/2 - transition_width/2) {
			vertical_progress = 0.0f;
		} else if (y > size/2 + transition_width/2) {
			vertical_progress = 1.0f;
		} else {
			vertical_progress = (float)(y - (size/2 - transition_width/2)) / (float)transition_width;
		}
		
		for (uint8_t x = 0; x < size; x++) {
			unsigned char r, g, b;			

			unsigned char top_r = 128;
			unsigned char top_g = 168;
			unsigned char top_b = 255;

			unsigned char bottom_r = 192;
			unsigned char bottom_g = 216;
			unsigned char bottom_b = 255;
	
			r = top_r + (unsigned char)((bottom_r - top_r) * vertical_progress);
			g = top_g + (unsigned char)((bottom_g - top_g) * vertical_progress);
			b = top_b + (unsigned char)((bottom_b - top_b) * vertical_progress);
	
			skybox_data[0][y][x][0] = top_r;
			skybox_data[0][y][x][1] = top_g;
			skybox_data[0][y][x][2] = top_b;
	
			skybox_data[1][y][x][0] = bottom_r;
			skybox_data[1][y][x][1] = bottom_g;
			skybox_data[1][y][x][2] = bottom_b;
	
			for (uint8_t face = 2; face < 6; face++) {
				skybox_data[face][y][x][0] = r;
				skybox_data[face][y][x][1] = g;
				skybox_data[face][y][x][2] = b;
			}
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[1]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[2]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[3]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[4]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[5]);

	// Initialize clouds
	glGenVertexArrays(1, &cloudsVAO);
	glGenBuffers(1, &cloudsVBO);
	glBindVertexArray(cloudsVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cloudsVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cloudsVertices), cloudsVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	
	clouds_view_loc = glGetUniformLocation(clouds_shader, "view");
	clouds_proj_loc = glGetUniformLocation(clouds_shader, "projection");
	clouds_model_loc = glGetUniformLocation(clouds_shader, "model");
	
	generate_clouds_texture();
}


void update_clouds() {
	cloudsTexOffsetX += CLOUDS_SPEED * 0.001f;
	cloudsTexOffsetZ += CLOUDS_SPEED * 0.0005f;

	cloudsTexOffsetX = fmodf(cloudsTexOffsetX, 1.0f);
	cloudsTexOffsetZ = fmodf(cloudsTexOffsetZ, 1.0f);
}

void skybox_render() {
	// Render skybox
	glUseProgram(skybox_shader);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glUniform1i(glGetUniformLocation(skybox_shader, "skybox"), 0);

	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 24);
	glBindVertexArray(0);

	// Render clouds
	glUseProgram(clouds_shader);

	float model[16];
	float translation[16];
	float scale[16];
	
	matrix4_identity(model);
	matrix4_identity(translation);
	matrix4_identity(scale);

	matrix4_translate(translation, 0.0f, CLOUDS_HEIGHT, 0.0f);
	matrix4_scale(scale, CLOUDS_QUAD_SIZE, 1.0f, CLOUDS_QUAD_SIZE);
	matrix4_multiply(model, translation, scale);
	
	glUniformMatrix4fv(clouds_view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(clouds_proj_loc, 1, GL_FALSE, projection);
	glUniformMatrix4fv(clouds_model_loc, 1, GL_FALSE, model);

	GLuint texOffsetLoc = glGetUniformLocation(clouds_shader, "texOffset");
	glUniform2f(texOffsetLoc, cloudsTexOffsetX, cloudsTexOffsetZ);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cloudsTexture);
	glUniform1i(glGetUniformLocation(clouds_shader, "cloudsTexture"), 0);
	
	glBindVertexArray(cloudsVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void cleanup_skybox() {
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);
	glDeleteProgram(skybox_shader);
	
	glDeleteVertexArrays(1, &cloudsVAO);
	glDeleteBuffers(1, &cloudsVBO);
	glDeleteTextures(1, &cloudsTexture);
	glDeleteProgram(clouds_shader);
}