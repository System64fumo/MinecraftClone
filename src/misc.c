#include "main.h"
#include <math.h>
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

void setup_matrices() {
	matrix4_identity(view);

	float pitch = global_entities[0].pitch * (M_PI / 180.0f);
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);

	#if USE_ARM_OPTIMIZED_CODE

	float cos_pitch = cosf(pitch);
	float sin_pitch = sinf(pitch);
	float cos_yaw = cosf(yaw);
	float sin_yaw = sinf(yaw);

	float32x4_t f = {cos_yaw * cos_pitch, sin_pitch, sin_yaw * cos_pitch, 0.0f};
	float32x4_t len_sq = vmulq_f32(f, f);
	float len = sqrtf(vaddvq_f32(len_sq));
	f = vdivq_f32(f, vdupq_n_f32(len));

	float32x4_t s = {-vgetq_lane_f32(f, 2), 0.0f, vgetq_lane_f32(f, 0), 0.0f};
	len_sq = vmulq_f32(s, s);
	len = sqrtf(vaddvq_f32(len_sq));
	s = vdivq_f32(s, vdupq_n_f32(len));

	float32x4_t u = {
		vgetq_lane_f32(s, 1) * vgetq_lane_f32(f, 2) - vgetq_lane_f32(s, 2) * vgetq_lane_f32(f, 1),
		vgetq_lane_f32(s, 2) * vgetq_lane_f32(f, 0) - vgetq_lane_f32(s, 0) * vgetq_lane_f32(f, 2),
		vgetq_lane_f32(s, 0) * vgetq_lane_f32(f, 1) - vgetq_lane_f32(s, 1) * vgetq_lane_f32(f, 0),
		0.0f
	};

	view[0] = vgetq_lane_f32(s, 0); view[4] = vgetq_lane_f32(s, 1); view[8] = vgetq_lane_f32(s, 2);
	view[1] = vgetq_lane_f32(u, 0); view[5] = vgetq_lane_f32(u, 1); view[9] = vgetq_lane_f32(u, 2);
	view[2] = -vgetq_lane_f32(f, 0); view[6] = -vgetq_lane_f32(f, 1); view[10] = -vgetq_lane_f32(f, 2);

	float32x4_t pos = {global_entities[0].x, global_entities[0].y + global_entities[0].eye_level, global_entities[0].z, 0.0f};
	view[12] = -vaddvq_f32(vmulq_f32(s, pos));
	view[13] = -vaddvq_f32(vmulq_f32(u, pos));
	view[14] = vaddvq_f32(vmulq_f32(f, pos));

	#else // Non ARM platforms

	float f[3];
	f[0] = cosf(yaw) * cosf(pitch);
	f[1] = sinf(pitch);
	f[2] = sinf(yaw) * cosf(pitch);
	float len = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	f[0] /= len; f[1] /= len; f[2] /= len;

	float s[3] = {
		f[1] * 0 - f[2] * 1,
		f[2] * 0 - f[0] * 0,
		f[0] * 1 - f[1] * 0
	};
	len = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
	s[0] /= len; s[1] /= len; s[2] /= len;

	float u[3] = {
		s[1] * f[2] - s[2] * f[1],
		s[2] * f[0] - s[0] * f[2],
		s[0] * f[1] - s[1] * f[0]
	};

	view[0] = s[0]; view[4] = s[1]; view[8] = s[2];
	view[1] = u[0]; view[5] = u[1]; view[9] = u[2];
	view[2] = -f[0]; view[6] = -f[1]; view[10] = -f[2];
	view[12] = -(s[0] * global_entities[0].x + s[1] * global_entities[0].y + global_entities[0].eye_level + s[2] * global_entities[0].z);
	view[13] = -(u[0] * global_entities[0].x + u[1] * global_entities[0].y + global_entities[0].eye_level + u[2] * global_entities[0].z);
	view[14] = (f[0] * global_entities[0].x + f[1] * global_entities[0].y + global_entities[0].eye_level + f[2] * global_entities[0].z);
	#endif
}

void matrix4_identity(float* mat) {
	mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
	mat[1] = mat[2] = mat[3] = mat[4] = mat[6] = mat[7] = mat[8] = mat[9] = mat[11] = mat[12] = mat[13] = mat[14] = 0.0f;
}

void matrix4_translate(float* mat, float x, float y, float z) {
	mat[12] = x;
	mat[13] = y;
	mat[14] = z;
}

void matrix4_multiply(float result[16], const float mat1[16], const float mat2[16]) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i * 4 + j] = 0.0f;
			for (int k = 0; k < 4; k++) {
				result[i * 4 + j] += mat1[i * 4 + k] * mat2[k * 4 + j];
			}
		}
	}
}

void matrix4_rotate(float* mat, float angle, float x, float y, float z) {
	float c = cosf(angle);
	float s = sinf(angle);
	float nc = 1 - c;

	float len = sqrtf(x*x + y*y + z*z);
	x /= len; y /= len; z /= len;

	mat[0] = x*x*nc + c;	mat[4] = x*y*nc - z*s;	mat[8] = x*z*nc + y*s;
	mat[1] = y*x*nc + z*s;	mat[5] = y*y*nc + c;	mat[9] = y*z*nc - x*s;
	mat[2] = x*z*nc - y*s;	mat[6] = y*z*nc + x*s;	mat[10] = z*z*nc + c;
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

void matrix4_scale(float* mat, float x, float y, float z) {
	mat[0] *= x;
	mat[5] *= y;
	mat[10] *= z;
}

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
