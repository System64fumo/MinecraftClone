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
#define CHUNKS_X 16
#define CHUNKS_Y 4
#define CHUNKS_Z 16
#define MAX_VERTICES 65536
#define DEBUG 1
#define FPS_UPDATE_INTERVAL 500
#define FPS_HISTORY_SIZE 10

inline int screen_width = 1280;
inline int screen_height = 720;
inline int chunk_radius = 10;

inline Uint32 lastTime;
inline Uint32 lastFpsUpdate;
inline float deltaTime = 0.0f;
inline int frameCount = 0;
inline float averageFps = 0.0f;
inline int fpsIndex = 0;
inline float fpsHistory[FPS_HISTORY_SIZE];

inline SDL_Event event;
inline GLuint block_vbo;
inline TTF_Font* font = NULL;
inline SDL_Window* window = NULL;
inline SDL_GLContext glContext;

// VBO function pointers
inline PFNGLGENBUFFERSPROC gl_gen_buffers = NULL;
inline PFNGLBINDBUFFERPROC gl_bind_buffer = NULL;
inline PFNGLBUFFERDATAPROC gl_buffer_data = NULL;
inline PFNGLDELETEBUFFERSPROC gl_delete_buffers = NULL;

// Player struct
typedef struct {
	float x, y, z;
	float yaw, pitch;
	unsigned char speed;
} Player;

// Block struct
typedef struct {
	unsigned char id;
	unsigned char metadata;
} Block;

typedef struct Chunk {
	unsigned char x, y, z;
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
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
int main_loop(Player* player);
void cleanup();
void draw_hud(float fps, Player* player);
void process_keyboard_movement(const Uint8* key_state, Player* player, float delta_time);
void bake_chunk(Chunk* chunk);
void load_chunk(unsigned char cx, unsigned char cy, unsigned char cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z);
bool raycast(Player* player, float max_distance, int* out_x, int* out_y, int* out_z, Chunk** out_chunk, float* out_distance);
void draw_block_highlight(float x, float y, float z);