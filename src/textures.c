#include <GL/glew.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

unsigned int block_textures, ui_textures, font_textures;

// Function pointers for WebP functions
typedef uint8_t* (*WebPDecodeRGBAPtr)(const uint8_t* data, size_t data_size, int* width, int* height);
typedef void (*WebPFreePtr)(void* ptr);

static void* webp_handle = NULL;
static WebPDecodeRGBAPtr WebPDecodeRGBA = NULL;
static WebPFreePtr WebPFree = NULL;

int load_webp_library() {
	webp_handle = dlopen("libwebp.so", RTLD_LAZY);
	if (!webp_handle) {
		fprintf(stderr, "Failed to load libwebp.so: %s\n", dlerror());
		return 0;
	}

	WebPDecodeRGBA = (WebPDecodeRGBAPtr)dlsym(webp_handle, "WebPDecodeRGBA");
	WebPFree = (WebPFreePtr)dlsym(webp_handle, "WebPFree");

	if (!WebPDecodeRGBA || !WebPFree) {
		fprintf(stderr, "Failed to load WebP functions: %s\n", dlerror());
		dlclose(webp_handle);
		webp_handle = NULL;
		return 0;
	}

	return 1;
}

void unload_webp_library() {
	if (webp_handle) {
		dlclose(webp_handle);
		webp_handle = NULL;
		WebPDecodeRGBA = NULL;
		WebPFree = NULL;
	}
}

unsigned int load_texture(const char* path) {
	if (!webp_handle && !load_webp_library()) {
		printf("WebP library not available\n");
		return 0;
	}

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

void load_textures() {
	load_webp_library();
	char exec_path[1024];
	ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1);
	if (len == -1) {
		perror("readlink failed");
		exit(EXIT_FAILURE);
	}
	exec_path[len] = '\0';

	char* lastSlash = strrchr(exec_path, '/');
	if (lastSlash)
		*lastSlash = '\0';

	char font_path[1024];
	if (snprintf(font_path, sizeof(font_path), "%s/%s", exec_path, "assets/font.webp") >= sizeof(font_path)) {
		fprintf(stderr, "font_path truncated\n");
		exit(EXIT_FAILURE);
	}
	font_textures = load_texture(font_path);

	char atlas_path[1024];
	if (snprintf(atlas_path, sizeof(atlas_path), "%s/%s", exec_path, "assets/atlas.webp") >= sizeof(atlas_path)) {
		fprintf(stderr, "atlas_path truncated\n");
		exit(EXIT_FAILURE);
	}
	block_textures = load_texture(atlas_path);

	char gui_path[1024];
	if (snprintf(gui_path, sizeof(gui_path), "%s/%s", exec_path, "assets/gui.webp") >= sizeof(gui_path)) {
		fprintf(stderr, "gui_path truncated\n");
		exit(EXIT_FAILURE);
	}
	ui_textures = load_texture(gui_path);
	unload_webp_library();
}