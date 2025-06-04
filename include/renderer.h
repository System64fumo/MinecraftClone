#include "misc.h"
#include <stdint.h>

typedef struct {
	float x, y, z;
	uint8_t face_id;
	uint8_t texture_id;
	float size_u, size_v;
} Vertex;

typedef struct {
	Vertex* vertices;
	uint32_t* indices;
	uint32_t vertex_count;
	uint32_t index_count;
	uint64_t capacity_vertices;
	uint64_t capacity_indices;
} FaceMesh;

extern unsigned int opaque_VAOs[6];
extern unsigned int opaque_VBOs[6];
extern unsigned int opaque_EBOs[6];
extern unsigned int transparent_VAOs[6];
extern unsigned int transparent_VBOs[6];
extern unsigned int transparent_EBOs[6];

extern bool mesh_mode;
extern uint16_t draw_calls;

void init_gl_buffers();
void update_frustum();
void render_chunks_combined();
void rebuild_combined_visible_mesh();
void render_chunks();
void cleanup_renderer();