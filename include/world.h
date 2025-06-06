#ifndef WORLD_H
#define WORLD_H

#include <pthread.h>

typedef struct {
	int x;
	int y;
	int z;
	float distance;
} chunk_load_item_t;

extern int last_cx;
extern int last_cy;
extern int last_cz;

extern pthread_mutex_t chunks_mutex;

void load_around_entity(Entity* entity);
void load_chunk_data(Chunk* chunk, unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z);
void start_world_gen_thread();

#endif