#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef DEBUG
#include "profiler.h"
#endif

#define CHUNK_SIZE 16
#define WORLD_HEIGHT 4
#define WORLD_SIZE UINT8_MAX
#define MAX_VERTICES 65536
#define MAX_ENTITIES_PER_CHUNK 1024
#define RENDERR_DISTANCE 10

#define FPS_UPDATE_INTERVAL 500
#define FPS_HISTORY_SIZE 10

// Entity struct
typedef struct {
	float x, y, z;
	float yaw, pitch;
	unsigned char speed;
} Entity;

// Block struct
typedef struct {
	unsigned char id;
	unsigned char metadata;
} Block;

typedef struct Chunk {
	unsigned char x, y, z;
	unsigned char ci_x, ci_y, ci_z;
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	GLuint vbo;
	GLuint color_vbo;
	int vertex_count;
	float* vertices;
	float* colors;
	bool needs_update;
} Chunk;

// Externs
extern int screen_width;
extern int screen_height;

extern float fov;
extern float near;
extern float far;

extern float screen_center_x;
extern float screen_center_y;

extern uint32_t lastTime;
extern uint32_t lastFpsUpdate;
extern float deltaTime;
extern int frameCount;
extern float averageFps;
extern int fpsIndex;
extern float fpsHistory[FPS_HISTORY_SIZE];

extern GLuint buffer;
extern Chunk chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];
extern Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDERR_DISTANCE * CHUNK_SIZE];

// Function prototypes
void cleanup();
void draw_hud(float fps, Entity* player);
void render_chunks();

void load_chunk(unsigned char x, unsigned char y, unsigned char z, unsigned char cx, unsigned char cy, unsigned char cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z);

void get_targeted_block(Entity* player, int* out_x, int* out_y, int* out_z);
void draw_block_highlight(float x, float y, float z);

void display();
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void special(int key, int x, int y);
void specialUp(int key, int x, int y);
void idle();
void mouse(int x, int y);