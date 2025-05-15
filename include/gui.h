#define MAX_UI_ELEMENTS 3

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

void init_ui();
void init_block_highlight();
void setup_ui_elements();
void update_ui();
void render_ui();
void cleanup_ui();