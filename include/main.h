#ifndef MAIN_H
#define MAIN_H

#define GLFW_EXPOSE_NATIVE_WAYLAND
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <wayland-client.h>
#include <stdbool.h>
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
extern uint8_t hotbar_slot;

extern double time_current;
extern double delta_time;
extern float framerate;
extern float frametime;
extern mat4 model, view, projection;

extern float near;
extern float far;
extern float aspect;
extern bool game_focused;
extern GLFWwindow* window;

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
void limit_fps();

void touch_down(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, struct wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y);
void touch_up(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, int32_t id);
void touch_motion(void *data, struct wl_touch *wl_touch, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y);
void check_touch_hold();

Block* get_block_at(Chunk*** chunks, int world_block_x, int world_block_y, int world_block_z);
void draw_block_highlight(vec3 pos, uint8_t block_id);
int is_block_solid(Chunk*** chunks, int world_block_x, int world_block_y, int world_block_z);
void calculate_chunk_and_block(int world_coord, int* chunk_coord, int* block_coord);
bool is_chunk_in_bounds(int render_x, int chunk_y, int render_z);
void update_adjacent_chunks(Chunk*** chunks, uint8_t render_x, uint8_t render_y, uint8_t render_z, int block_x, int block_y, int block_z);
void generate_single_block_mesh(float x, float y, float z, uint8_t block_id, Mesh faces[6]);
void add_quad(Chunk* chunk, float x, float y, float z, uint8_t normal, uint8_t texture_id,
			  const face_vertex_t face_data[4], uint8_t width, uint8_t height,
			  uint8_t sky_light, uint8_t block_light,
			  Vertex vertices[], uint32_t indices[],
			  uint32_t* vertex_count, uint32_t* index_count);

bool init_mesh_thread();
void cleanup_mesh_thread();
void enqueue_chunk_update_priority(uint8_t x, uint8_t y, uint8_t z);
unsigned char* generate_light_texture();

bool are_all_neighbors_loaded(uint8_t x, uint8_t y, uint8_t z);
bool is_face_visible(Chunk* chunk, int8_t x, int8_t y, int8_t z, uint8_t face);
void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d, uint8_t* x, uint8_t* y, uint8_t* z);
uint8_t get_face_light(Chunk* chunk, int x, int y, int z, uint8_t face);
uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block);
uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width);
void generate_chunk_mesh(Chunk* chunk);
void init_chunk_lighting(Chunk* chunk);
void init_column_lighting(Chunk column[WORLD_HEIGHT]);
void update_block_lighting(int wx, int wy, int wz, uint8_t old_id, uint8_t new_id);

Chunk*** allocate_chunks();
void free_chunks(Chunk*** chunks);

#endif