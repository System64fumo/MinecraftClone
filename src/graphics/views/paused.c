#include "views.h"
#include "textures.h"
#include "config.h"
#include <stdio.h>

void view_pause_init() {
	// Resume button
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((200 * settings.gui_scale) / 2),
		.y = screen_center_y + (5 * settings.gui_scale),
		.width = 200 * settings.gui_scale,
		.height = 20 * settings.gui_scale,
		.tex_x = 0,
		.tex_y = 66,
		.tex_width = 200,
		.tex_height = 20,
		.texture_id = ui_textures
	});

	// Quit button
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((200 * settings.gui_scale) / 2),
		.y = screen_center_y - ((20 + 5) * settings.gui_scale),
		.width = 200 * settings.gui_scale,
		.height = 20 * settings.gui_scale,
		.tex_x = 0,
		.tex_y = 66,
		.tex_width = 200,
		.tex_height = 20,
		.texture_id = ui_textures
	});

	draw_text("Back to Game", screen_center_x, screen_center_y + (11 * settings.gui_scale), 'C');
	draw_text("Save and Quit to Title", screen_center_x, screen_center_y - (19 * settings.gui_scale), 'C');
}

void view_pause_hover(uint16_t cursor_x, uint16_t cursor_y) {
	if (check_hit(cursor_x, cursor_y, 0)) {
		ui_elements[0].tex_y = 86;
	}
	else if (check_hit(cursor_x, cursor_y, 1)) {
		ui_elements[1].tex_y = 86;
	}
	else {
		ui_elements[0].tex_y = 66;
		ui_elements[1].tex_y = 66;
	}
	update_ui_buffer();
}