#include "views.h"
#include "textures.h"
#include <stdio.h>

void view_setup_pause() {
	// Resume button
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((200 * UI_SCALING) / 2),
		.y = screen_center_y + (5 * UI_SCALING),
		.width = 200 * UI_SCALING,
		.height = 20 * UI_SCALING,
		.tex_x = 0,
		.tex_y = 66,
		.tex_width = 200,
		.tex_height = 20,
		.texture_id = ui_textures
	});

	// Quit button
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((200 * UI_SCALING) / 2),
		.y = screen_center_y - ((20 + 5) * UI_SCALING),
		.width = 200 * UI_SCALING,
		.height = 20 * UI_SCALING,
		.tex_x = 0,
		.tex_y = 66,
		.tex_width = 200,
		.tex_height = 20,
		.texture_id = ui_textures
	});

	char back_text[13];
	snprintf(back_text, sizeof(back_text), "Back to Game");
	draw_text(back_text, screen_center_x - (get_text_length(back_text) / 2), screen_center_y + (11 * UI_SCALING));

	char quit_text[23];
	snprintf(quit_text, sizeof(quit_text), "Save and Quit to Title");
	draw_text(quit_text, screen_center_x - (get_text_length(quit_text) / 2), screen_center_y - (19 * UI_SCALING));
}
