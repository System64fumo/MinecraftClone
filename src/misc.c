#include "main.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>

void matrix4_identity(float* mat) {
	mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
	mat[1] = mat[2] = mat[3] = mat[4] = mat[6] = mat[7] = mat[8] = mat[9] = mat[11] = mat[12] = mat[13] = mat[14] = 0.0f;
}

void matrix4_translate(float* mat, float x, float y, float z) {
	mat[12] = x;
	mat[13] = y;
	mat[14] = z;
}

void matrix4_rotate(float* mat, float angle, float x, float y, float z) {
	float c = cosf(angle);
	float s = sinf(angle);
	float nc = 1 - c;

	float len = sqrtf(x*x + y*y + z*z);
	x /= len; y /= len; z /= len;

	mat[0] = x*x*nc + c;    mat[4] = x*y*nc - z*s;  mat[8] = x*z*nc + y*s;
	mat[1] = y*x*nc + z*s;  mat[5] = y*y*nc + c;    mat[9] = y*z*nc - x*s;
	mat[2] = x*z*nc - y*s;  mat[6] = y*z*nc + x*s;  mat[10] = z*z*nc + c;
}

void matrix4_perspective(float* mat, float fovy, float aspect, float near, float far) {
	float f = 1.0f / tanf(fovy * 0.5f);

	mat[0] = f / aspect;
	mat[5] = f;
	mat[10] = (far + near) / (near - far);
	mat[11] = -1.0f;
	mat[14] = (2.0f * far * near) / (near - far);
	mat[15] = 0.0f;
}


const char* load_file(const char* filename) {
	char current_dir[1024];
	if (realpath(__FILE__, current_dir) == NULL) {
		perror("Error getting current directory");
		return NULL;
	}

	char* last_slash = strrchr(current_dir, '/');
	if (last_slash != NULL) {
		*last_slash = '\0';
	}

	char full_path[1024];
	snprintf(full_path, sizeof(full_path), "%s/%s", current_dir, filename);

	char resolved_path[1024];
	if (realpath(full_path, resolved_path) == NULL) {
		perror("Error resolving file path");
		return NULL;
	}

	FILE* file = fopen(resolved_path, "rb");
	if (file == NULL) {
		perror("Error opening file");
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* buffer = (char*)malloc(file_size + 1);
	if (buffer == NULL) {
		perror("Memory allocation error");
		fclose(file);
		return NULL;
	}

	size_t bytes_read = fread(buffer, 1, file_size, file);
	if (bytes_read != file_size) {
		perror("Error reading file");
		free(buffer);
		fclose(file);
		return NULL;
	}

	buffer[file_size] = '\0';

	fclose(file);
	return buffer;
}



unsigned int loadTexture(const char* path) {
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data) {
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		stbi_image_free(data);
	} else {
		printf("Failed to load texture: %s\n", path);
		stbi_image_free(data);
	}

	return textureID;
}
