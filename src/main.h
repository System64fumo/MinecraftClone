#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>

#define CHUNK_SIZE 16
#define MAX_VERTICES 65536

inline int screen_width = 1280;
inline int screen_height = 720;

inline GLuint block_vbo;
inline TTF_Font* font = NULL;

// VBO function pointers
inline PFNGLGENBUFFERSPROC gl_gen_buffers = NULL;
inline PFNGLBINDBUFFERPROC gl_bind_buffer = NULL;
inline PFNGLBUFFERDATAPROC gl_buffer_data = NULL;
inline PFNGLDELETEBUFFERSPROC gl_delete_buffers = NULL;

// Player struct
typedef struct {
	float x;
	float y;
	float z;
	float yaw;
	float pitch;
	float speed;
} Player;

// Block struct
typedef struct {
	int id;
	int metadata;
	float x;
	float y;
	float z;
} Block;

typedef struct {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	float x, y, z;
	GLuint vbo;
	GLuint color_vbo;
	int vertex_count;
	float* vertices;
	float* colors;
	bool needs_update;
} Chunk;

// Function prototypes
void draw_hud(float fps);
void process_keyboard_movement(const Uint8* key_state, Player* player, float delta_time);
void bake_chunk(Chunk* chunk);
void draw_hud();
void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z);