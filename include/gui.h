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
} cube_element_t;

extern cube_element_t cube_elements[MAX_CUBE_ELEMENTS];

void init_cube_rendering();
void draw_cube_element(const cube_element_t* cube);
void render_3d_elements();

void init_ui();
void init_block_highlight();
void setup_ui_elements();
void update_ui();
void render_ui();
void cleanup_ui();