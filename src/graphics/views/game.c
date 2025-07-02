#include "main.h"
#include "views.h"
#include "textures.h"
#include "config.h"
#include <stdio.h>
#include <math.h>

void view_setup_game() {
	// Crosshair
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - (8 * UI_SCALING),
		.y = screen_center_y - (8 * UI_SCALING),
		.width = 16 * UI_SCALING,
		.height = 16 * UI_SCALING,
		.tex_x = 240,
		.tex_y = 0,
		.tex_width = 16,
		.tex_height = 16,
		.texture_id = ui_textures
	});

	// Hotbar
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((182 * UI_SCALING) / 2),
		.y = 0,
		.width = 182 * UI_SCALING,
		.height = 22 * UI_SCALING,
		.tex_x = 0,
		.tex_y = 0,
		.tex_width = 182,
		.tex_height = 22,
		.texture_id = ui_textures
	});
	
	// Hotbar slot
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((182 * UI_SCALING) / 2) - (1 * UI_SCALING) + ((20 * (hotbar_slot % 9)) * UI_SCALING),
		.y = 0 - (1 * UI_SCALING),
		.width = 24 * UI_SCALING,
		.height = 24 * UI_SCALING,
		.tex_x = 0,
		.tex_y = 22,
		.tex_width = 24,
		.tex_height = 24,
		.texture_id = ui_textures
	});

	// Hotbar blocks
	for (uint8_t i = 0; i < MAX_CUBE_ELEMENTS - 1; i++) {
		uint8_t block_id = i + 1 + (floor(hotbar_slot / 9) * 9);
		vec2 pos = {
			.x = screen_center_x - ((182 * UI_SCALING) / 2) + ( i * 20 * UI_SCALING) + (3 * UI_SCALING),
			.y = 3 * UI_SCALING,
		};
		draw_item(block_id, pos);
	}

	// Selected block
	if (ui_active_3d_elements < MAX_CUBE_ELEMENTS) {
		cube_elements[ui_active_3d_elements] = (cube_element_t){
			.pos.x = 1.15,
			.pos.y = -1.65,
			.width = 1,
			.height = 1,
			.depth = 1,
			.rotation_x = -10 * DEG_TO_RAD,
			.rotation_y = -30 * DEG_TO_RAD,
			.rotation_z = -2.5 * DEG_TO_RAD,
			.id = hotbar_slot + 1
		};
		ui_active_3d_elements++;
	}

	char fps_text[23];
	snprintf(fps_text, sizeof(fps_text), "FPS: %1.2f, %1.3fms", framerate, frametime);
	draw_text(fps_text, 3, settings.window_height - ((8 + 3) * UI_SCALING));
}