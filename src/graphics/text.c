#include "main.h"
#include "gui.h"
#include "textures.h"

uint8_t font_data[255][2] = {
	[0 ... 254] = {8, 8},
	['a' ... 'z'] = {6, 6},
	['A' ... 'Z'] = {6, 8},
	['0' ... '9'] = {6, 8},
	['d'] = {6, 8},
	['k'] = {6, 8},
	['o'] = {6, 6},
	['i'] = {2, 8},
	['l'] = {3, 8},
	['t'] = {4, 8},
	[' '] = {4, 8},
	['"'] = {4, 8},
	['!'] = {2, 8},
	['.'] = {2, 8},
	[','] = {4, 8},
	['\''] = {2, 8},
	['`'] = {3, 8},
	[':'] = {2, 8},
	[';'] = {2, 8},
	['|'] = {2, 8},
};

uint16_t get_text_length(char* ptr) {
	uint16_t offset = 0;
	while (*ptr != '\0') {
		offset += font_data[(unsigned char)*ptr++][0] * UI_SCALING;
	}
	return offset;
}

void draw_char(unsigned char chr, uint16_t x, uint16_t y) {
	uint8_t index_x, index_y;
	uint8_t col = chr % 16;
	uint8_t row = chr / 16;
	index_x = col * 16;
	index_y = row * 16;

	ui_element_t char_element = {
		.x = x,
		.y = y,
		.width = font_data[chr][0] * UI_SCALING,
		.height = font_data[chr][1] * UI_SCALING,
		.tex_x = index_x,
		.tex_y = index_y + (8 - font_data[chr][1]) * 2,
		.tex_width = font_data[chr][0] * 2, // 2x because the atlas is 128x128 but we expect 256x256
		.tex_height = font_data[chr][1] * 2,
		.texture_id = font_textures
	};
	add_ui_element(&char_element);
}

void draw_text(char* ptr, uint16_t x, uint16_t y) {
	uint8_t char_index = 0;
	uint16_t offset = 9;
	while (*ptr != '\0') {
		draw_char(*ptr, x + offset, y);
		offset += font_data[(unsigned char)*ptr][0] * UI_SCALING;
		ptr++;
		char_index++;
	}
}