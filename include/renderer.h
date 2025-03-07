typedef struct {
	float x, y, z;
	uint8_t face_id;
	uint8_t texture_id;
	uint8_t width, height;
	uint32_t light_data;
} Vertex;

typedef struct {
	Vertex* vertices;
	uint32_t* indices;
	uint32_t vertex_count;
	uint32_t index_count;
	uint32_t capacity_vertices;
	uint32_t capacity_indices;
} CombinedMesh;

extern CombinedMesh combined_mesh;
extern unsigned int combined_VAO;
extern unsigned int combined_VBO;
extern unsigned int combined_EBO;
extern bool mesh_mode;
extern uint16_t draw_calls;

void init_gl_buffers();
void update_chunks_visibility();
void print_rendered_chunks();
void render_chunks();
void cleanup_renderer();