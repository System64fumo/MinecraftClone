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

// Light level helpers — shared by all files that read/write block light data.
// Layout: bits 4-7 = sky light (0-15), bits 0-3 = block light (0-15)
#define SKY_LIGHT(b)		 (((b) >> 4) & 0xF)
#define BLOCK_LIGHT(b)	   ((b) & 0xF)
#define PACK_LIGHT(sky, blk) ((uint8_t)(((sky) << 4) | ((blk) & 0xF)))

typedef struct Chunk {
	Block blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
	int32_t x, y, z;
	uint8_t ci_x, ci_y, ci_z;
	bool needs_update;
	bool is_loaded;
	bool lighting_changed;

	Mesh faces[6];
	Mesh transparent_faces[6];

	// Per-chunk GPU buffers — uploaded once when mesh is built,
	// drawn directly without any CPU-side merge pass.
	uint32_t opaque_vao, opaque_vbo, opaque_ebo;
	uint32_t transparent_vao, transparent_vbo, transparent_ebo;
	uint32_t opaque_index_count;
	uint32_t transparent_index_count;
	bool gpu_buffers_valid;
	bool mesh_dirty;  // set by mesh thread after building, cleared by main thread after upload
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