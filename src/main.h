#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <GL/gl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

#define CHUNK_SIZE 16
#define CHUNKS_X 16
#define CHUNKS_Y 4
#define CHUNKS_Z 16
#define MAX_VERTICES 65536
//#define DEBUG
#define FPS_UPDATE_INTERVAL 500
#define FPS_HISTORY_SIZE 10

inline int screen_width = 1280;
inline int screen_height = 720;
inline int chunk_radius = 10;

inline float fov = 70.0f;
inline float near = 0.1f;
inline float far = 200.0f;

inline float screen_center_x;
inline float screen_center_y;

inline Uint32 lastTime;
inline Uint32 lastFpsUpdate;
inline float deltaTime = 0.0f;
inline int frameCount = 0;
inline float averageFps = 0.0f;
inline int fpsIndex = 0;
inline float fpsHistory[FPS_HISTORY_SIZE];

inline SDL_Event event;
inline GLuint block_vbo;
inline SDL_Window* window = NULL;
inline SDL_GLContext glContext;

// VBO function pointers
inline PFNGLGENBUFFERSPROC gl_gen_buffers = NULL;
inline PFNGLBINDBUFFERPROC gl_bind_buffer = NULL;
inline PFNGLBUFFERDATAPROC gl_buffer_data = NULL;
inline PFNGLDELETEBUFFERSPROC gl_delete_buffers = NULL;

// VAIO function pointers
inline PFNGLGENVERTEXARRAYSPROC gl_gen_vertex_arrays = NULL;
inline PFNGLBINDVERTEXARRAYPROC gl_bind_vertex_array = NULL;
inline PFNGLDELETEVERTEXARRAYSPROC gl_delete_vertex_arrays = NULL;
inline PFNGLVERTEXATTRIBPOINTERPROC gl_vertex_attrib_pointer = NULL;
inline PFNGLENABLEVERTEXATTRIBARRAYPROC gl_enable_vertex_attrib_array = NULL;

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
	GLuint vao;
	GLuint vbo;
	GLuint color_vbo;
	int vertex_count;
	float* vertices;
	float* colors;
	bool needs_update;
} Chunk;
inline Chunk chunks[CHUNKS_X][CHUNKS_Y][CHUNKS_Z];

// Function prototypes
int main_loop(Player* player);
void change_resolution();
void cleanup();
void draw_hud(float fps, Player* player);
void process_keyboard_movement(const Uint8* key_state, Player* player, float delta_time);
void bake_chunk(Chunk* chunk);
void load_chunk(unsigned char cx, unsigned char cy, unsigned char cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, unsigned char chunk_x, unsigned char chunk_y, unsigned char chunk_z);
bool raycast(Player* player, float max_distance, int* out_x, int* out_y, int* out_z, Chunk** out_chunk, float* out_distance);
void draw_block_highlight(float x, float y, float z);