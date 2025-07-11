#ifndef WORLD_H
#define WORLD_H

#include "renderer.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>

typedef struct Entity Entity;

#define BTYPE_REGULAR 0
#define BTYPE_SLAB 1
#define BTYPE_CROSS 2
#define BTYPE_LIQUID 3
#define BTYPE_LEAF 4

// TODO: Replace static definitions with enums
enum BlockModel {
	REGULAR,
	SLAB,
	CROSS,
	LIQUID,
	LEAF
};

typedef struct {
	int x;
	int y;
	int z;
	float distance;
} chunk_load_item_t;

typedef struct {
	int8_t x, y, z;
	uint8_t block_id;
} structure_block_t;

typedef struct {
	structure_block_t* blocks;
	uint8_t block_count;
	uint8_t width, height, depth;
} structure_t;

typedef struct Block {
	uint8_t id;
	uint8_t light_level;
} Block;

typedef struct {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	int32_t x, y, z;
	uint8_t ci_x, ci_y, ci_z;
	bool needs_update;
	bool is_loaded;
	bool lighting_changed;

	Mesh faces[6];
	Mesh transparent_faces[6];
} Chunk;

extern uint8_t block_data[MAX_BLOCK_TYPES][8];
extern Chunk*** chunks;

extern structure_block_t tree_blocks[];
extern structure_t tree_structure;

static const int SEA_LEVEL = 64;

static const float continent_scale = 0.005f;
static const float mountain_scale = 0.03f;
static const float flatness_scale = 0.02f;
static const float cave_scale = 0.1f;
static const float cave_simplex_scale = 0.1f;
static const float structure_scale = 0.1f;
static const bool flat_world_gen = false;

extern pthread_mutex_t chunks_mutex;
extern _Atomic int world_offset_x;
extern _Atomic int world_offset_z;

void process_chunks();
void move_world(int x, int y, int z);
void load_around_entity(Entity* entity);
void load_chunk_data(Chunk* chunk, unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z);
bool can_place_tree(int world_x, int surface_y, int world_z, bool is_grass_surface);
void generate_structure_in_chunk(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z,
							   structure_t* structure, int structure_world_x, int structure_world_y, int structure_world_z,
							   bool* empty_chunk);
void start_world_gen_thread();
void stop_world_gen_thread();

#endif