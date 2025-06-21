#include "main.h"

unsigned int skyboxTexture;
unsigned int skyboxVAO, skyboxVBO, skyboxIBO;
GLuint view_loc, proj_loc;

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

unsigned int skyboxIndices[] = {
	0, 1, 2, 3,
	4, 5, 6, 7,
	8, 9, 10, 11,
	12, 13, 14, 15,
	16, 17, 18, 19,
	20, 21, 22, 23
};

DrawElementsIndirectCommand skyboxDrawCommands[6];

void skybox_init() {
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glGenBuffers(1, &skyboxIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, skyboxIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(skyboxIndices), skyboxIndices, GL_STATIC_DRAW);

	for (uint8_t i = 0; i < 6; i++) {
		skyboxDrawCommands[i].count = 4;
		skyboxDrawCommands[i].instanceCount = 1;
		skyboxDrawCommands[i].firstIndex = i * 4;
		skyboxDrawCommands[i].baseVertex = 0;
		skyboxDrawCommands[i].baseInstance = 0;
	}

	GLuint indirectBuffer;
	glGenBuffers(1, &indirectBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(skyboxDrawCommands), skyboxDrawCommands, GL_STATIC_DRAW);
	
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
}

void skybox_render() {
	glDepthFunc(GL_LEQUAL);
	glUseProgram(skybox_shader);
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, view);
	glUniformMatrix4fv(proj_loc, 1, GL_FALSE, projection);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	glUniform1i(glGetUniformLocation(skybox_shader, "skybox"), 0);

	glBindVertexArray(skyboxVAO);

	glMultiDrawElementsIndirect(
		GL_TRIANGLE_STRIP,	// Primitive type
		GL_UNSIGNED_INT,	// Index type
		NULL,				// Pointer to indirect commands (already in buffer)
		6,					// Draw count (6 faces)
		0					// Stride (0 if commands are tightly packed)
	);
	draw_calls++;

	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

void cleanup_skybox() {
	glDeleteVertexArrays(1, &skyboxVAO);
	glDeleteBuffers(1, &skyboxVBO);
	glDeleteBuffers(1, &skyboxIBO);
	glDeleteProgram(skybox_shader);
}