#include "main.h"
#include <stdlib.h>
#include <string.h>
#include <webp/decode.h>

double lastFrame = 0.0f;
double delta_time = 0.0f;
double time_current = 0.0f;
double time_difference = 0.0f;
double time_previous = 0.0f;
int time_counter = 0;
float framerate = 0.0f;
float frametime = 0.0f;

void do_time_stuff() {
	time_current = glfwGetTime();
	delta_time = time_current - lastFrame;
	lastFrame = time_current;
	time_difference = time_current - time_previous;
	time_counter++;

	// 20 TPS
	if (time_difference >= 0.05f) {
		#ifdef DEBUG
		profiler_print_all();
		#endif

		framerate = (1.0 / time_difference) * time_counter;
		frametime = (time_difference / time_counter) * 1000;
		printf("FPS: %.2f, %.3f ms\n", framerate, frametime);
		time_previous = time_current;
		time_counter = 0;

		process_chunks();

		#ifdef DEBUG
		pthread_mutex_lock(&mesh_mutex);
		printf("Vertex count: %d\n", combined_mesh.vertex_count);
		printf("Index count: %d\n", combined_mesh.index_count);
		printf("VRAM estimate: %ldmb\n", (sizeof(Vertex) * combined_mesh.vertex_count) / 1024 / 1024);
		printf("Draw calls: %d\n", draw_calls);
		pthread_mutex_unlock(&mesh_mutex);
		#endif
	}
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

	// Read the entire file into memory
	FILE* file = fopen(path, "rb");
	if (!file) {
		printf("Failed to open file: %s\n", path);
		return 0;
	}

	fseek(file, 0, SEEK_END);
	size_t file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
		
	uint8_t* file_data = (uint8_t*)malloc(file_size);
	if (!file_data) {
		printf("Failed to allocate memory for file data\n");
		fclose(file);
		return 0;
	}
		
	if (fread(file_data, 1, file_size, file) != file_size) {
		printf("Failed to read file data\n");
		free(file_data);
		fclose(file);
		return 0;
	}
	fclose(file);

	// Decode WebP
	int width, height;
	uint8_t* image_data = WebPDecodeRGBA(file_data, file_size, &width, &height);
	free(file_data);
		
	if (!image_data) {
		printf("Failed to decode WebP image: %s\n", path);
		return 0;
	}

	// Upload to OpenGL
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		
	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	WebPFree(image_data);

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
