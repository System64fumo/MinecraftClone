#include "main.h"
#include "framebuffer.h"
#include "gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webp/decode.h>
#include <math.h>

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
		framerate = (1.0 / time_difference) * time_counter;
		frametime = (time_difference / time_counter) * 1000;
		time_previous = time_current;
		time_counter = 0;

		process_chunks();
		if (frustum_changed) {
			update_frustum();
			frustum_changed = false;
		}
	}

	// 1 TPS
	if (fmod(time_current, 1.0f) < delta_time) {
		if (ui_state == UI_STATE_RUNNING)
			update_ui();

		#ifdef DEBUG
		profiler_print_all();
		
		// Chunk memory tracking
		uint32_t total_opaque_vertices = 0;
		uint32_t total_opaque_indices = 0;
		uint32_t total_transparent_vertices = 0;
		uint32_t total_transparent_indices = 0;
		uint32_t loaded_chunks = 0;
		uint32_t visible_chunks = 0;

		for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
			for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (chunk->is_loaded) loaded_chunks++;
					if (chunk->is_visible) visible_chunks++;
					
					if (!chunk->is_visible) continue;
					
					for (uint8_t face = 0; face < 6; face++) {
						total_opaque_vertices += chunk->faces[face].vertex_count;
						total_opaque_indices += chunk->faces[face].index_count;
						total_transparent_vertices += chunk->transparent_faces[face].vertex_count;
						total_transparent_indices += chunk->transparent_faces[face].index_count;
					}
				}
			}
		}

		// Framebuffer memory tracking
		size_t fb_memory = 0;
		if (colorTexture) {
			GLint width, height, internal_format;
			glBindTexture(GL_TEXTURE_2D, colorTexture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internal_format);
			fb_memory += width * height * (internal_format == GL_RGB8 ? 3 : 4); // 3 bytes for RGB8, 4 for RGBA8
		}
		if (depthTexture) {
			GLint width, height;
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			fb_memory += width * height * 4; // Depth24 typically uses 4 bytes
		}

		// UI memory tracking
		size_t ui_memory = 0;
		ui_memory += ui_active_2d_elements * sizeof(ui_element_t);
		ui_memory += ui_active_2d_elements * 4 * sizeof(ui_vertex_t); // 4 vertices per element
		ui_memory += ui_batch_count * sizeof(ui_batch_t);
		ui_memory += ui_active_3d_elements * sizeof(cube_element_t);

		// Calculate total VRAM usage
		size_t vram_usage = 0;
		// Chunk geometry VRAM
		vram_usage += (total_opaque_vertices * sizeof(Vertex)) + (total_opaque_indices * sizeof(uint32_t));
		vram_usage += (total_transparent_vertices * sizeof(Vertex)) + (total_transparent_indices * sizeof(uint32_t));
		// Framebuffer VRAM
		vram_usage += fb_memory;
		// UI VRAM
		vram_usage += ui_memory;
		// Texture VRAM (estimate)
		vram_usage += (256 * 256 * 4) * 2; // block_textures + ui_textures estimate

		printf("Chunks: %d loaded, %d visible\n", loaded_chunks, visible_chunks);
		printf("Verticies: %d opaque, %d transparent\n", total_opaque_vertices, total_transparent_vertices);
		printf("Indices: %d opaque, %d transparent\n", total_opaque_indices, total_transparent_indices);
		printf("Framebuffer: %zukb\n", fb_memory / 1024);
		printf("UI: %d elements, %zukb\n", ui_active_2d_elements, ui_memory / 1024);
		printf("Total VRAM estimate: %zukb\n", vram_usage / 1024);
		printf("Draw calls: %d\n", draw_calls);
		printf("FPS: %.1f (%.2fms)\n", framerate, frametime);
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
	if (last_slash)
		*last_slash = '\0';

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

unsigned int load_texture(const char* path) {
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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

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

void* create_array(size_t size, size_t element_size) {
	void *arr = malloc(size * element_size);
	if (arr == NULL) {
		perror("malloc failed");
		exit(EXIT_FAILURE);
	}
	return arr;
}