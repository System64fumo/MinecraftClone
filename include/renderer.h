#ifndef RENDERER_H
#define RENDERER_H

#include "misc.h"

typedef struct {
	float x, y, z;
	uint16_t packed_data;  // bits 0-7: normal, bits 8-15: texture_id
	uint32_t packed_size;  // bits 0-8: size_u, bits 9-17: size_v
} Vertex;

typedef struct {
	float pos[3];
	float uv[2];
} face_vertex_t;

typedef struct {
	Vertex* vertices;
	uint32_t* indices;
	uint16_t vertex_count;
	uint16_t index_count;
} Mesh;

#define PACK_VERTEX_DATA(normal, texture_id) \
	((uint16_t)(normal) | ((uint16_t)(texture_id) << 8))

#define PACK_SIZE_DATA(size_u, size_v) \
	((uint32_t)(size_u) | ((uint32_t)(size_v) << 9))

extern bool mesh_mode;
extern bool frustum_changed;
extern bool mesh_needs_rebuild;
extern uint16_t draw_calls;

#define FACE_BACK   (1 << 0)
#define FACE_LEFT   (1 << 1)
#define FACE_FRONT  (1 << 2)
#define FACE_RIGHT  (1 << 3)
#define FACE_BOTTOM (1 << 5)
#define FACE_TOP	(1 << 4)
#define ALL_FACES   (0x3F)

extern uint8_t*** visibility_map;
extern uint32_t opaque_VAO, opaque_VBO, opaque_EBO, opaque_index_count;
extern uint32_t transparent_VAO, transparent_VBO, transparent_EBO, transparent_index_count;

void init_gl_buffers();
void update_frustum();
void rebuild_combined_visible_mesh();
void render_chunks();
void cleanup_renderer();

#endif