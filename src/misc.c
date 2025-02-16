#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

double fpsHistory[180] = {0};  // 180 frames = ~3 seconds at 60fps
int fpsIndex = 0;
double fpsSum = 0;

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

void count_framerate() {
	// Calculate FPS and maintain rolling average
	fpsSum -= fpsHistory[fpsIndex];
	fpsHistory[fpsIndex] = 1.0f / deltaTime;
	fpsSum += fpsHistory[fpsIndex];
	fpsIndex = (fpsIndex + 1) % 180;

	fps_average = (int)(fpsSum / 180);
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

	char full_path[1025];
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

	FILE* file = fopen(path, "rb");
	if (!file) {
		printf("Failed to open file: %s\n", path);
		return 0;
	}

	png_byte header[8];
	fread(header, 1, 8, file);
	if (png_sig_cmp(header, 0, 8)) {
		printf("File is not a valid PNG: %s\n", path);
		fclose(file);
		return 0;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		printf("Failed to create PNG read struct\n");
		fclose(file);
		return 0;
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		printf("Failed to create PNG info struct\n");
		png_destroy_read_struct(&png, NULL, NULL);
		fclose(file);
		return 0;
	}

	if (setjmp(png_jmpbuf(png))) {
		printf("Error during PNG reading\n");
		png_destroy_read_struct(&png, &info, NULL);
		fclose(file);
		return 0;
	}

	png_init_io(png, file);
	png_set_sig_bytes(png, 8);
	png_read_info(png, info);

	int width = png_get_image_width(png, info);
	int height = png_get_image_height(png, info);
	png_byte color_type = png_get_color_type(png, info);
	png_byte bit_depth = png_get_bit_depth(png, info);

	if (bit_depth == 16)
		png_set_strip_16(png);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png);

	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png);

	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	png_read_update_info(png, info);

	png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
	int row_bytes = png_get_rowbytes(png, info);
	png_bytep image_data = (png_bytep)malloc(row_bytes * height);

	for (int y = 0; y < height; y++) {
		row_pointers[y] = image_data + y * row_bytes;
	}

	png_read_image(png, row_pointers);

	GLenum format;
	if (color_type == PNG_COLOR_TYPE_RGB)
		format = GL_RGB;
	else if (color_type == PNG_COLOR_TYPE_RGBA)
		format = GL_RGBA;
	else {
		printf("Unsupported PNG color type\n");
		free(image_data);
		free(row_pointers);
		png_destroy_read_struct(&png, &info, NULL);
		fclose(file);
		return 0;
	}

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, image_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clean up
	free(image_data);
	free(row_pointers);
	png_destroy_read_struct(&png, &info, NULL);
	fclose(file);

	return textureID;
}

// Write binary data to a file
int write_binary_file(const char *filename, const void *data, size_t size) {
    FILE *file = fopen(filename, "wb");
    if (!file) return -1;

    size_t written = fwrite(data, 1, size, file);
    fclose(file);

    return (written == size) ? 0 : -1;
}

// Read binary data from a file
void *read_binary_file(const char *filename, size_t *size) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;

    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    rewind(file);

    void *data = malloc(*size);
    if (!data) {
        fclose(file);
        return NULL;
    }

    fread(data, 1, *size, file);
    fclose(file);
    return data;
}