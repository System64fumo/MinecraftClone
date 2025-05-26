#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdbool.h>
#include "renderer.h"
#include "misc.h"
#include "config.h"

#ifdef DEBUG
#include "profiler.h"
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#if defined(__ARM_NEON) || defined(__ARM_NEON__)
	#include <arm_neon.h>
	#define USE_ARM_OPTIMIZED_CODE 1
#else
	#define USE_ARM_OPTIMIZED_CODE 0
#endif

// Typedefs
typedef struct {
	vec3 pos;
	float yaw, pitch;
	float width, height;
	float eye_level;
	uint8_t speed;
	bool is_grounded;
	float vertical_velocity;
	uint8_t inventory_size;
	uint8_t* inventory_data; // TODO: Implement proper inventory struct
} Entity;

typedef struct {
	uint8_t id;
	uint8_t light_data;
} Block;

typedef struct {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	int32_t x, y, z;
	uint8_t ci_x, ci_y, ci_z;
	bool needs_update;
	bool needs_mesh_update;
	bool is_loaded;
	bool lighting_changed;
	uint32_t vertex_count;
	uint32_t index_count;
	Vertex* vertices;
	uint32_t* indices;

	Vertex* transparent_vertices;
	uint32_t* transparent_indices;
	uint32_t transparent_vertex_count;
	uint32_t transparent_index_count;
} Chunk;

typedef struct {
	uint32_t start_index;
	uint32_t index_count;
	bool visible;
} ChunkRenderData;

// Externs
extern config settings;
extern unsigned short screen_center_x;
extern unsigned short screen_center_y;

extern uint8_t hotbar_slot;
extern char game_dir[255];

extern float near;
extern float far;
extern float aspect;

extern float sky_brightness;

extern double delta_time;
extern float framerate;
extern float frametime;
extern float model[16], view[16], projection[16];

extern unsigned int shaderProgram, postProcessingShader, ui_shader, cube_shader;
extern unsigned int block_textures, ui_textures;

extern uint8_t block_data[MAX_BLOCK_TYPES][8];
extern Chunk*** chunks;
extern ChunkRenderData chunk_render_data[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];
extern Entity global_entities[MAX_ENTITIES_PER_CHUNK];

extern unsigned int model_uniform_location;
extern unsigned int atlas_uniform_location;
extern unsigned int view_uniform_location;
extern unsigned int projection_uniform_location;
extern unsigned int ui_projection_uniform_location;

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void set_hotbar_slot(uint8_t slot);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void process_input(GLFWwindow* window);
void setup_matrices();

vec3 get_direction(float pitch, float yaw);
void update_frustum();
void get_targeted_block(Entity entity, vec3 direction, float reach, vec3* pos_out, char* out_face);
Block* get_block_at(int world_block_x, int world_block_y, int world_block_z);
void draw_block_highlight(vec3 pos);
bool is_block_solid(int world_block_x, int world_block_y, int world_block_z);
void calculate_chunk_and_block(int world_coord, int* chunk_coord, int* block_coord);
bool is_chunk_in_bounds(int render_x, int chunk_y, int render_z);
void update_adjacent_chunks(int render_x, int chunk_y, int render_z, int block_x, int block_y, int block_z);
bool check_entity_collision(float x, float y, float z, float width, float height);
void update_entity_physics(Entity* player, float delta_time);
Entity create_entity(uint8_t id);

unsigned int load_shader(const char* vertex_path, const char* fragment_path);
void load_shaders();

void process_chunks();

bool is_face_visible(Chunk* chunk, int8_t x, int8_t y, int8_t z, uint8_t face);
void map_coordinates(uint8_t face, uint8_t u, uint8_t v, uint8_t d, uint8_t* x, uint8_t* y, uint8_t* z);
uint8_t find_width(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block);
uint8_t find_height(Chunk* chunk, uint8_t face, uint8_t u, uint8_t v, uint8_t x, uint8_t y, uint8_t z, bool mask[CHUNK_SIZE][CHUNK_SIZE], Block* block, uint8_t width);
void generate_slab_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count);
void generate_cross_vertices(float x, float y, float z, Block* block, Vertex vertices[], uint32_t* vertex_count);
void generate_vertices(uint8_t face, float x, float y, float z, uint8_t width, uint8_t height, Block* block, Vertex vertices[], uint32_t* vertex_count, uint8_t light_data);
void generate_indices(uint32_t base_vertex, uint32_t indices[], uint32_t* index_count);
void generate_chunk_mesh(Chunk* chunk);
void set_chunk_lighting(Chunk* chunk);

Chunk*** allocate_chunks(int render_distance, int world_height);
void free_chunks(Chunk ***chunks, int render_distance, int world_height);