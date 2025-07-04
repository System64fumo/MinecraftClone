#include "main.h"
#include "gui.h"
#include "textures.h"
#include "config.h"
#include <stdarg.h>
#include <stdio.h>

uint8_t font_data[255][2] = {
	[0 ... 254] = {8, 8},
	['a' ... 'z'] = {6, 8},
	['A' ... 'Z'] = {6, 8},
	['0' ... '9'] = {6, 8},
	['w'] = {6, 6},
	['h'] = {6, 8},
	['b'] = {6, 8},
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
	['('] = {5, 8},
	[')'] = {5, 8},
};

uint16_t get_text_length(char* ptr) {
	uint16_t offset = 0;
	while (*ptr != '\0') {
		offset += font_data[(unsigned char)*ptr++][0] * settings.gui_scale;
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
		.width = font_data[chr][0] * settings.gui_scale,
		.height = font_data[chr][1] * settings.gui_scale,
		.tex_x = index_x,
		.tex_y = index_y + (8 - font_data[chr][1]) * 2,
		.tex_width = font_data[chr][0] * 2, // 2x because the atlas is 128x128 but we expect 256x256
		.tex_height = font_data[chr][1] * 2,
		.texture_id = font_textures
	};
	add_ui_element(&char_element);
}

void draw_text(char* text, uint16_t x, uint16_t y, char alignment) {
	uint16_t length = get_text_length(text);
	
	// Adjust x position based on alignment
	switch (alignment) {
		case 'C':  // Center
			x -= length / 2;
			break;
		case 'R':  // Right
			x -= length;
			break;
		case 'L':  // Left (default)
		default:
			// No adjustment needed
			break;
	}
	
	uint16_t offset = 0;
	while (*text != '\0') {
		draw_char(*text, x + offset, y);
		offset += font_data[(unsigned char)*text][0] * settings.gui_scale;
		text++;
	}
}

// Variadic version for formatted text
void draw_textf(uint16_t x, uint16_t y, char alignment, const char* format, ...) {
	char buffer[128];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	draw_text(buffer, x, y, alignment);
}