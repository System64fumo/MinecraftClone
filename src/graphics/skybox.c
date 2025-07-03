#include "main.h"
#include "shaders.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// Skybox variables
unsigned int skyboxVAO, skyboxVBO;
GLuint view_loc, proj_loc, time_loc;

// Clouds variables
unsigned int cloudsTexture;
unsigned int cloudsVAO, cloudsVBO;
GLuint clouds_view_loc, clouds_proj_loc, clouds_model_loc;
float cloudsTexOffsetX = 0.0f;
float cloudsTexOffsetZ = 0.0f;
float dayNightTime = 0.25f;
const float CLOUDS_HEIGHT = 192.0f;
const float CLOUDS_SPEED = 0.1f;
const float CLOUDS_QUAD_SIZE = 500.0f;
const float DAY_CYCLE_SPEED = 0.05f;

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
	uint8_t *data = calloc(width, height);

	const int CLOUD_PIXEL_SIZE = 1;
	const int PIXEL_SPACING = 1;
	
	for (int y = 0; y < height; y += PIXEL_SPACING) {
		for (int x = 0; x < width; x += PIXEL_SPACING) {
			if (rand() % 100 < 25) {
				for (int dy = 0; dy < CLOUD_PIXEL_SIZE; dy++) {
					for (int dx = 0; dx < CLOUD_PIXEL_SIZE; dx++) {
						int px = x + dx;
						int py = y + dy;
						
						if (px < width && py < height) {
							int idx = py * width + px;
							data[idx] = 255;
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
	
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
	time_loc = glGetUniformLocation(skybox_shader, "time");

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
#include <stdio.h>
void update_clouds() {
	cloudsTexOffsetX += CLOUDS_SPEED * 0.001f;
	cloudsTexOffsetZ += CLOUDS_SPEED * 0.0005f;

	cloudsTexOffsetX = fmodf(cloudsTexOffsetX, 1.0f);
	cloudsTexOffsetZ = fmodf(cloudsTexOffsetZ, 1.0f);

	// TODO: Implement day/night cycles
	// dayNightTime += DAY_CYCLE_SPEED * 0.001f;
	// dayNightTime = fmodf(dayNightTime, 1.0f);
}

void skybox_render() {
	// Render skybox
	glUseProgram(skybox_shader);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection);
	//glUniform1f(time_loc, dayNightTime);

	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 24);
	draw_calls++;
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

	glUniform2f(clouds_offset_uniform_location, cloudsTexOffsetX, cloudsTexOffsetZ);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cloudsTexture);
	glUniform1i(glGetUniformLocation(clouds_shader, "cloudsTexture"), 0);
	
	glBindVertexArray(cloudsVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	draw_calls++;
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