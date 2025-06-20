#ifndef MISC_H
#define MISC_H

#include <stdint.h>

#define CHUNK_SIZE 16
#define WORLD_HEIGHT 16
#define MAX_ENTITIES_PER_CHUNK 128
#define RENDER_DISTANCE 16
#define MAX_BLOCK_TYPES 256
#define MAX_VERTICES 98304 // CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;
#define UI_SCALING 3.0f

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

static const float DEG_TO_RAD = M_PI / 180.0f;

typedef struct {
	uint32_t x, y, z, w;
} uvec4;

typedef struct {
	float x, y, z, w;
} vec4;

typedef struct {
	float x, y, z;
} vec3;

typedef struct {
	float x, y;
} vec2;

typedef float mat4[16];

void setup_matrices();
void matrix4_identity(float* mat);
void matrix4_translate(float* mat, float x, float y, float z);
void matrix4_multiply(mat4 result, mat4 mat1, mat4 mat2);
void matrix4_rotate(float* mat, float angle, float x, float y, float z);
void matrix4_perspective(float* mat, float fovy, float aspect, float near, float far);
void matrix4_scale(float* mat, float x, float y, float z);
void matrix4_rotate_x(float* matrix, float angle);
void matrix4_rotate_y(float* matrix, float angle);
void matrix4_rotate_z(float* matrix, float angle);
void do_time_stuff();
const char* load_file(const char* filename);
unsigned int load_texture(const char* path);
int write_binary_file(const char *filename, const void *data, unsigned long size);
void *read_binary_file(const char *filename, unsigned long *size);
void* create_array(unsigned long size, unsigned long element_size);

#endif // MISC_H