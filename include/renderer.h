#ifndef RENDERER_H
#define RENDERER_H

#include "misc.h"

typedef struct {
	float x, y, z;
	uint8_t normal;
	uint8_t texture_id;
	float size_u, size_v;
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
} FaceMesh;

extern unsigned int opaque_VAO;
extern unsigned int opaque_VBO;
extern unsigned int opaque_EBO;
extern unsigned int transparent_VAO;
extern unsigned int transparent_VBO;
extern unsigned int transparent_EBO;

extern bool mesh_mode;
extern bool frustum_changed;
extern bool mesh_needs_rebuild;
extern uint16_t draw_calls;

extern bool visibility_map[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];

void init_gl_buffers();
void update_frustum();
void rebuild_combined_visible_mesh();
void render_chunks();
void cleanup_renderer();

#endif