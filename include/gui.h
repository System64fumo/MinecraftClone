#define MAX_UI_ELEMENTS 3

typedef struct {
	float x;
	float y;
	float width;
	float height;
	int tex_x;
	int tex_y;
	int tex_width;
	int tex_height;
} ui_element_t;

void init_ui();
void update_ui();
void render_ui();
void cleanup_ui();