#define MAX_UI_ELEMENTS 3
#define MAX_CUBE_ELEMENTS 9

typedef struct {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	uint8_t tex_x;
	uint8_t tex_y;
	uint8_t tex_width;
	uint8_t tex_height;
} ui_element_t;

typedef struct {
	vec2 pos;
	float width, height, depth;
	float rotation_x, rotation_y, rotation_z;
	uint8_t tex_x;
	uint8_t tex_y;
	uint8_t tex_width;
	uint8_t tex_height;
} cube_element_t;

static const float vertices_template[] = {
	// Front face (Z+)
	0.0f, 0.0f, 1.0f,
	1.0f, 0.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	0.0f, 1.0f, 1.0f,

	// Back face (Z-)
	0.0f, 0.0f, 0.0f,
	1.0f, 0.0f, 0.0f,
	1.0f, 1.0f, 0.0f,
	0.0f, 1.0f, 0.0f
};

static const uint8_t edge_indices[] = {
	// Front face edges
	0, 1, // Bottom edge
	1, 2, // Right edge
	2, 3, // Top edge
	3, 0, // Left edge

	// Back face edges
	4, 5, // Bottom edge
	5, 6, // Right edge
	6, 7, // Top edge
	7, 4, // Left edge

	// Side edges
	0, 4, // Bottom-left edge
	1, 5, // Bottom-right edge
	2, 6, // Top-right edge
	3, 7  // Top-left edge
};

static float cube_vertices[] = {
	// Front face (Z+)
	-0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	
	// Back face (Z-)
	 0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	
	// Left face (X-)
	-0.5f, -0.5f, -0.5f,
	-0.5f, -0.5f,  0.5f,
	-0.5f,  0.5f,  0.5f,
	-0.5f,  0.5f, -0.5f,
	
	// Right face (X+)
	 0.5f, -0.5f,  0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f,  0.5f, -0.5f,
	 0.5f,  0.5f,  0.5f,
	
	// Top face (Y+)
	-0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f,  0.5f,
	 0.5f,  0.5f, -0.5f,
	-0.5f,  0.5f, -0.5f,
	
	// Bottom face (Y-)
	-0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f, -0.5f,
	 0.5f, -0.5f,  0.5f,
	-0.5f, -0.5f,  0.5f
};

static uint8_t cube_indices[] = {
	// Front face
	0, 1, 2,  0, 2, 3,
	// Back face
	4, 5, 6,  4, 6, 7,
	// Left face
	8, 9, 10,  8, 10, 11,
	// Right face
	12, 13, 14,  12, 14, 15,
	// Top face
	16, 17, 18,  16, 18, 19,
	// Bottom face
	20, 21, 22,  20, 22, 23
};

static float cube_tex_coords[] = {
	// Front face
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	
	// Back face
	1.0f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	
	// Left face
	1.0f, 1.0f,
	0.0f, 1.0f,
	0.0f, 0.0f,
	1.0f, 0.0f,
	
	// Right face
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	
	// Top face
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 0.0f,
	
	// Bottom face
	0.0f, 0.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
	0.0f, 1.0f
};

void init_cube_rendering();
void draw_cube_element(const cube_element_t* cube);
void render_3d_elements();

void init_ui();
void init_block_highlight();
void setup_ui_elements();
void update_ui();
void render_ui();
void cleanup_ui();