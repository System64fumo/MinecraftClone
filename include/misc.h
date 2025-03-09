#ifndef _MISC_H_
#define _MISC_H_

#define CHUNK_SIZE 16
#define WORLD_HEIGHT 16
#define MAX_ENTITIES_PER_CHUNK 128
#define RENDER_DISTANCE 16
#define MAX_BLOCK_TYPES 256
#define MAX_VERTICES 98304 // CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 6 * 4;
#define UI_SCALING 2.5f

typedef struct {
	float x, y, z;
} vec3;

#endif // _MISC_H_