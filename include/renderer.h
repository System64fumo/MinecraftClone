#ifndef RENDERER_H
#define RENDERER_H

#include "misc.h"
#include <stdint.h>

typedef struct {
	float x, y, z;
	uint8_t face_id;
	uint8_t texture_id;
	float size_u, size_v;
	// uvec4 ligh_level0;
	// uvec4 ligh_level1;
	// uvec4 ligh_level2;
	// uvec4 ligh_level3;
	// uvec4 ligh_level4;
	// uvec4 ligh_level5;
	// uvec4 ligh_level6;
	// uvec4 ligh_level7;
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

extern unsigned int opaque_VAOs[6];
extern unsigned int opaque_VBOs[6];
extern unsigned int opaque_EBOs[6];
extern unsigned int transparent_VAOs[6];
extern unsigned int transparent_VBOs[6];
extern unsigned int transparent_EBOs[6];

extern bool mesh_mode;
extern bool frustum_changed;
extern bool mesh_needs_rebuild;
extern uint16_t draw_calls;

void init_gl_buffers();
void update_frustum();
void rebuild_combined_visible_mesh();
void render_chunks();
void cleanup_renderer();

#endif