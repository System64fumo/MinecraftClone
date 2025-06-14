#ifndef GUI_H
#define GUI_H

#include "misc.h"
#include <stdint.h>

#define MAX_CUBE_ELEMENTS 9

#define UI_STATE_RUNNING 0
#define UI_STATE_PAUSED 1

typedef struct {
	uint16_t x;
	uint16_t y;
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
	GLuint texture_id;
	uint16_t start_index;
	uint16_t count;
} ui_batch_t;

extern uint8_t ui_state;
extern uint8_t ui_active_2d_elements;
extern uint8_t ui_active_3d_elements;
extern uint8_t ui_batch_count;
extern ui_element_t* ui_elements;

void init_cube_rendering();
void draw_cube_element(const cube_element_t* cube);
void render_3d_elements();
bool add_ui_element(const ui_element_t *element);

uint16_t get_text_length(char* ptr);
void draw_char(unsigned char chr, uint16_t x, uint16_t y);
void draw_text(char* ptr, uint16_t x, uint16_t y);

void init_ui();
void init_block_highlight();
bool check_hit(uint16_t hit_x, uint16_t hit_y, uint8_t element_id);
void update_ui_buffer();
void update_ui();
void render_ui();
void cleanup_ui();

#endif