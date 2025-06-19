#include "main.h"

unsigned int skyboxTexture;
unsigned int skyboxVAO, skyboxVBO;

float skyboxVertices[] = {
	// positions		  
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};
	
void skybox_init() {
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// Create gradient texture
	glGenTextures(1, &skyboxTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Generate gradient data
	const int size = 16;
	unsigned char skybox_data[6][size][size][3];

	for (int y = 0; y < size; y++) {
		float vertical_progress;
		const int transition_width = 2;
		
		if (y < size/2 - transition_width/2) {
			vertical_progress = 0.0f;
		} else if (y > size/2 + transition_width/2) {
			vertical_progress = 1.0f;
		} else {
			vertical_progress = (float)(y - (size/2 - transition_width/2)) / (float)transition_width;
		}
		
		for (int x = 0; x < size; x++) {
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
	
			for (int face = 2; face < 6; face++) {
				skybox_data[face][y][x][0] = r;
				skybox_data[face][y][x][1] = g;
				skybox_data[face][y][x][2] = b;
			}
		}
	}

	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[2]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[3]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[0]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[1]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[4]);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, GL_RGB, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, skybox_data[5]);
}

void skybox_render() {
	glDepthFunc(GL_LEQUAL);
	glUseProgram(skybox_shader);
	
	GLuint view_loc = glGetUniformLocation(skybox_shader, "view");
	GLuint proj_loc = glGetUniformLocation(skybox_shader, "projection");
	
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection);
	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glUniform1i(glGetUniformLocation(skybox_shader, "skybox"), 0);
	
	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	
	glDepthFunc(GL_LESS);
}

void cleanup_skybox() {
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);
	glDeleteProgram(skybox_shader);
}