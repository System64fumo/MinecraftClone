#ifndef MAIN_H
#define MAIN_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include "renderer.h"
#include "misc.h"
#include "world.h"

#ifdef DEBUG
#include "profiler.h"
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
	#include <arm_neon.h>
	#define USE_ARM_OPTIMIZED_CODE 1
#else
	#define USE_ARM_OPTIMIZED_CODE 0
#endif

// Externs
extern unsigned short screen_center_x;
extern unsigned short screen_center_y;

extern uint8_t hotbar_slot;

extern double time_difference;
extern double delta_time;
extern float framerate;
extern float frametime;
extern mat4 model, view, projection;

extern float near;
extern float far;
extern float aspect;
extern GLFWwindow* window;

extern uint8_t block_data[MAX_BLOCK_TYPES][8];
extern Chunk*** chunks;

// Function prototypes
int initialize_window();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void set_hotbar_slot(uint8_t slot);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void process_input(GLFWwindow* window, Chunk*** chunks);
void setup_matrices();
void set_fov(float fov);

Block* get_block_at(Chunk*** chunks, int world_block_x, int world_block_y, int world_block_z);
void draw_block_highlight(vec3 pos);
int is_block_solid(Chunk*** chunks, int world_block_x, int world_block_y, int world_block_z);
void calculate_chunk_and_block(int world_coord, int* chunk_coord, int* block_coord);
bool is_chunk_in_bounds(int render_x, int chunk_y, int render_z);
void update_adjacent_chunks(Chunk*** chunks, uint8_t render_x, uint8_t render_y, uint8_t render_z, int block_x, int block_y, int block_z);
void generate_single_block_mesh(float x, float y, float z, uint8_t block_id, FaceMesh faces[6]);
void clear_face_data(FaceMesh faces[6]);

void process_chunks();
unsigned char* generateLightTexture3D(int* out_width, int* out_height, int* out_depth);
bool saveTextureSliceAsPNG(const unsigned char* texture_data, int y_slice, const char* filename);

bool are_all_neighbors_loaded(uint8_t x, uint8_t y, uint8_t z);
bool is_face_visible(Chunk* chunk, int8_t x, int8_t y, int8_t z, uint8_t face);
void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d, uint8_t* x, uint8_t* y, uint8_t* z);
uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block);
uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width);
void generate_chunk_mesh(Chunk* chunk);
void set_chunk_lighting(Chunk* chunk);

Chunk*** allocate_chunks();
void free_chunks(Chunk*** chunks);

#endif