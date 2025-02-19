#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#ifdef DEBUG
#include "profiler.h"
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

// Defines
#define CHUNK_SIZE 16
#define WORLD_HEIGHT 4
#define MAX_ENTITIES_PER_CHUNK 1024
#define RENDER_DISTANCE 16
#define MAX_BLOCK_TYPES 256
#define MAX_VERTICES 98304 // CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;

// Structs
typedef struct {
	uint8_t x, y, z;
	uint8_t face_id;
	uint8_t texture_id;
	uint8_t width, height;
} Vertex;

typedef struct {
	float x, y, z;
	float yaw, pitch;
	uint8_t speed;
} Entity;

typedef struct {
	uint8_t id;
	uint8_t metadata;
} Block;

typedef struct {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	int32_t x, y, z;
	uint8_t ci_x, ci_y, ci_z;
	uint32_t VAO, VBO, EBO;
	bool needs_update;
	uint32_t vertex_count;
	uint32_t index_count;
} Chunk;

// Externs
extern unsigned short screen_width;
extern unsigned short screen_height;
extern unsigned short screen_center_x;
extern unsigned short screen_center_y;

extern uint8_t hotbar_slot;
extern char game_dir[255];

extern float fov;
extern float near;
extern float far;
extern float aspect;

extern bool frustum_faces[6];

extern int last_cx;
extern int last_cz;
extern float deltaTime;
extern unsigned short fps_average;
extern float model[16], view[16], projection[16];

extern unsigned int shaderProgram;
extern const char* vertexShaderSource;
extern const char* fragmentShaderSource;

extern uint8_t block_textures[MAX_BLOCK_TYPES][6];
extern Chunk chunks[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];
extern Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDER_DISTANCE * CHUNK_SIZE];

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void processInput(GLFWwindow* window);
void setupMatrices();
void cleanup();

void get_targeted_block(Entity* entity, int* out_x, int* out_y, int* out_z, char* out_face);
void draw_block_highlight(float x, float y, float z);

void matrix4_identity(float* mat);
void matrix4_translate(float* mat, float x, float y, float z);
void matrix4_rotate(float* mat, float angle, float x, float y, float z);
void matrix4_perspective(float* mat, float fovy, float aspect, float near, float far);
void count_framerate();
const char* load_file(const char* filename);
unsigned int loadTexture(const char* path);
int write_binary_file(const char *filename, const void *data, size_t size);
void *read_binary_file(const char *filename, size_t *size);

void load_around_entity(Entity* entity);
void load_chunk(unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z);
void drawChunk(Chunk* chunk, unsigned int shaderProgram, float* model);
void render_chunks();

bool is_face_visible(Chunk* chunk, int8_t x, int8_t y, int8_t z, uint8_t face);
void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d, uint8_t* x, uint8_t* y, uint8_t* z);
uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block);
uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width);
void generate_vertices(uint8_t face, uint8_t x, uint8_t y, uint8_t z, uint8_t width, uint8_t height, Block* block, Vertex vertices[], uint16_t* vertex_count);
void generate_indices(uint16_t base_vertex, uint32_t indices[], uint16_t* index_count);

void init_renderer(void);