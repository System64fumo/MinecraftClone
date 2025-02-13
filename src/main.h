#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// Defines
#define CHUNK_SIZE 16
#define WORLD_HEIGHT 4
#define WORLD_SIZE UINT8_MAX
#define MAX_ENTITIES_PER_CHUNK 1024
#define RENDERR_DISTANCE 16

// Structs
typedef struct {
	float x, y, z;
} Vec3;

typedef struct {
	float x, y, z;
	float dirX, dirY, dirZ;
	float yaw, pitch;
	unsigned char speed;
} Entity;

typedef struct {
	unsigned char id;
	unsigned char metadata;
} Block;

typedef struct {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	unsigned char x, y, z;
	unsigned char ci_x, ci_y, ci_z;
	unsigned int VAO, VBO, EBO;
	bool needs_update;
	unsigned int vertex_count;
	unsigned int index_count;
} Chunk;

// Externs
extern unsigned short screen_width;
extern unsigned short screen_height;
extern unsigned short screen_center_x;
extern unsigned short screen_center_y;

extern float fov;
extern float near;
extern float far;
extern float aspect;

extern float deltaTime;
extern float model[16], view[16], projection[16];

extern unsigned int shaderProgram;

extern Chunk chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];
extern Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDERR_DISTANCE * CHUNK_SIZE];

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void setupMatrices();
void cleanup();

void matrix4_identity(float* mat);
void matrix4_translate(float* mat, float x, float y, float z);
void matrix4_rotate(float* mat, float angle, float x, float y, float z);
void matrix4_perspective(float* mat, float fovy, float aspect, float near, float far);

void load_around_entity(Entity* entity);
void load_chunk(unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, unsigned char cx, unsigned char cy, unsigned char cz);
void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z);
void drawChunk(Chunk* chunk, unsigned int shaderProgram, float* model);
void render_chunks();
