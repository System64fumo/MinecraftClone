#ifndef GUI_H
#define GUI_H

#include "misc.h"

#define MAX_CUBE_ELEMENTS 10

#define UI_STATE_RUNNING 0
#define UI_STATE_PAUSED 1

typedef struct {
	int16_t x;
	int16_t y;
	uint16_t width;
	uint16_t height;
	uint8_t tex_x;
	uint8_t tex_y;
	uint8_t tex_width;
	uint8_t tex_height;
	uint8_t texture_id;
} ui_element_t;

typedef struct {
	vec2 pos;
	float width, height, depth;
	float rotation_x, rotation_y, rotation_z;
	uint8_t id;
} cube_element_t;

typedef struct {
	vec2 position;
	vec2 tex_coord;
	uint32_t tex_id;
} ui_vertex_t;

typedef struct {
	unsigned int texture_id;
	uint16_t start_index;
	uint16_t count;
} ui_batch_t;

extern uint8_t ui_state;
extern uint8_t ui_active_2d_elements;
extern uint8_t ui_active_3d_elements;
extern uint8_t ui_batch_count;
extern ui_element_t* ui_elements;
extern cube_element_t* cube_elements;

extern unsigned short screen_center_x;
extern unsigned short screen_center_y;

void init_cube_rendering();
void draw_cube_element(const cube_element_t* cube);
void render_3d_elements();
bool add_ui_element(const ui_element_t *element);
bool add_cube_element(const cube_element_t *element);
void remove_cube_element(uint8_t index);

void draw_text(char* text, uint16_t x, uint16_t y, bool centered);
void draw_textf(uint16_t x, uint16_t y, bool centered, const char* format, ...);

void init_ui();
void draw_item(uint8_t id, vec2 pos);
bool check_hit(uint16_t hit_x, uint16_t hit_y, uint8_t element_id);
void update_ui_buffer();
void update_ui();
void render_ui();
void cleanup_ui();

#endif