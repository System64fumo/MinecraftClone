#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define CHUNK_SIZE 16
#define CHUNKS_X 4
#define CHUNKS_Y 4
#define CHUNKS_Z 4
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
	unsigned char id;
	unsigned char metadata;
	unsigned char x, y, z;
} Block;

typedef struct Chunk {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	unsigned char x, y, z;
	GLuint vbo;
	GLuint color_vbo;
	int vertex_count;
	float* vertices;
	float* colors;
	bool needs_update;
	struct Chunk* neighbors[6]; // north, south, east, west, up, down
} Chunk;

inline Chunk chunks[CHUNKS_X][CHUNKS_Y][CHUNKS_Z];

// Function prototypes
void draw_hud(float fps, Player* player);
void process_keyboard_movement(const Uint8* key_state, Player* player, float delta_time);
void bake_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z);