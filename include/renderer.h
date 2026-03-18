#ifndef RENDERER_H
#define RENDERER_H

#include "misc.h"
#include <stdatomic.h>

typedef struct Chunk Chunk;

typedef struct {
	int32_t  x;
	uint16_t y;
	int32_t  z;
	uint16_t packed_data;
	uint32_t packed_size;
} Vertex;

typedef struct {
	float pos[3];
	float uv[2];
} face_vertex_t;

typedef struct {
	Vertex   *vertices;
	uint32_t *indices;
	uint16_t  vertex_count;
	uint16_t  index_count;
} Mesh;

#define PACK_VERTEX_DATA(normal, texture_id) \
	((uint16_t)(normal) | ((uint16_t)(texture_id) << 8))

#define PACK_SIZE_DATA(size_u, size_v) \
	((uint32_t)(size_u) | ((uint32_t)(size_v) << 9))

extern bool       mesh_mode;
extern bool       frustum_changed;
extern _Atomic bool mesh_needs_rebuild;
extern uint16_t   draw_calls;

#define FACE_BACK   (1 << 0)
#define FACE_LEFT   (1 << 1)
#define FACE_FRONT  (1 << 2)
#define FACE_RIGHT  (1 << 3)
#define FACE_BOTTOM (1 << 5)
#define FACE_TOP    (1 << 4)
#define ALL_FACES   (0x3F)

extern uint8_t ***visibility_map;

void init_gl_buffers();
void update_frustum();
void rebuild_combined_visible_mesh();
void render_chunks();
void cleanup_renderer();
void chunk_alloc_gpu_buffers(Chunk *chunk);
void chunk_free_gpu_buffers(Chunk *chunk);
void chunk_upload_mesh(Chunk *chunk);

#endif
