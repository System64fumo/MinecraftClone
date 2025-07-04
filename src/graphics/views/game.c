#include "main.h"
#include "views.h"
#include "textures.h"
#include "config.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

bool debug_view = false;

typedef struct {
	char compatible[256];
	char cpuinfo[256];
	char renderer[25];
	int core_count;
	bool initialized;
} DebugInfoCache;

static DebugInfoCache debug_cache = {0};

void init_debug_info_cache() {
	if (debug_cache.initialized) return;

	// Initialize compatible string
	FILE *f = fopen("/proc/device-tree/compatible", "r");
	if (f) {
		char buffer[512];
		size_t bytes_read = fread(buffer, 1, sizeof(buffer), f);
		fclose(f);
		
		if (bytes_read > 0) {
			buffer[sizeof(buffer) - 1] = '\0';
			char *current = buffer;
			char *last_soc = NULL;

			while (current < buffer + bytes_read && *current) {
				last_soc = current;
				current += strlen(current) + 1;
			}
			
			if (last_soc) {
				char *comma = strrchr(last_soc, ',');
				strncpy(debug_cache.compatible, comma ? comma + 1 : last_soc, sizeof(debug_cache.compatible) - 1);
				debug_cache.compatible[sizeof(debug_cache.compatible) - 1] = '\0';

				// Clean up non-printable characters
				for (char *p = debug_cache.compatible; *p; p++) {
					if (*p < ' ' || *p > '~') *p = '\0';
				}
			}
		}
	}

	// Initialize CPU info
	DIR *cpus_dir = opendir("/proc/device-tree/cpus");
	if (cpus_dir) {
		struct dirent *entry;
		int first_core = 1;
		
		while ((entry = readdir(cpus_dir))) {
			if (strncmp(entry->d_name, "cpu@", 4) == 0) {
				debug_cache.core_count++;

				if (first_core) {
					first_core = 0;
					char cpu_path[512];
					snprintf(cpu_path, sizeof(cpu_path), 
							 "/proc/device-tree/cpus/%s/compatible",
							 entry->d_name);
					
					f = fopen(cpu_path, "r");
					if (f) {
						char buffer[512];
						if (fgets(buffer, sizeof(buffer), f)) {
							char *last_part = strrchr(buffer, ',');
							if (last_part) {
								strncpy(debug_cache.cpuinfo, last_part + 1, sizeof(debug_cache.cpuinfo) - 1);
								debug_cache.cpuinfo[sizeof(debug_cache.cpuinfo) - 1] = '\0';

								char *nl = strchr(debug_cache.cpuinfo, '\n');
								if (nl) *nl = '\0';
							}
						}
						fclose(f);
					}
				}
			}
		}
		closedir(cpus_dir);
	}

	const GLubyte* renderer = glGetString(GL_RENDERER);
	if (renderer) {
		strncpy(debug_cache.renderer, (const char*)renderer, sizeof(debug_cache.renderer) - 1);
		debug_cache.renderer[sizeof(debug_cache.renderer) - 1] = '\0';
	}

	debug_cache.initialized = true;
}

void view_game_debug() {
	init_debug_info_cache();

	char debug_info[256];
	if (debug_cache.core_count > 0) {
		snprintf(debug_info, sizeof(debug_info), "%s %dx %s",
				debug_cache.compatible,
				debug_cache.core_count,
				debug_cache.cpuinfo);
	} else {
		snprintf(debug_info, sizeof(debug_info), "%s (unknown cores)",
				debug_cache.compatible);
	}

	draw_textf(
		settings.window_width - (2 * settings.gui_scale),
		settings.window_height - ((8 + 3) * settings.gui_scale),
		'R',
		"%s",
		debug_info);

	// Draw renderer info
	draw_textf(
		settings.window_width - (2 * settings.gui_scale),
		settings.window_height - (((8 * 2) + 3) * settings.gui_scale),
		'R',
		"%.24s",
		debug_cache.renderer);
}

void view_game_init() {
	// Crosshair
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - (8 * settings.gui_scale),
		.y = screen_center_y - (8 * settings.gui_scale),
		.width = 16 * settings.gui_scale,
		.height = 16 * settings.gui_scale,
		.tex_x = 240,
		.tex_y = 0,
		.tex_width = 16,
		.tex_height = 16,
		.texture_id = ui_textures
	});

	// Hotbar
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((182 * settings.gui_scale) / 2),
		.y = 0,
		.width = 182 * settings.gui_scale,
		.height = 22 * settings.gui_scale,
		.tex_x = 0,
		.tex_y = 0,
		.tex_width = 182,
		.tex_height = 22,
		.texture_id = ui_textures
	});
	
	// Hotbar slot
	add_ui_element(&(ui_element_t) {
		.x = screen_center_x - ((182 * settings.gui_scale) / 2) - (1 * settings.gui_scale) + ((20 * (hotbar_slot % 9)) * settings.gui_scale),
		.y = 0 - (1 * settings.gui_scale),
		.width = 24 * settings.gui_scale,
		.height = 24 * settings.gui_scale,
		.tex_x = 0,
		.tex_y = 22,
		.tex_width = 24,
		.tex_height = 24,
		.texture_id = ui_textures
	});

	// Hotbar blocks
	for (uint8_t i = 0; i < 9; i++) {
		uint8_t block_id = i + 1 + (floor(hotbar_slot / 9) * 9);
		vec2 pos = {
			.x = screen_center_x - ((182 * settings.gui_scale) / 2) + ( i * 20 * settings.gui_scale) + (3 * settings.gui_scale),
			.y = 3 * settings.gui_scale,
		};
		draw_item(block_id, pos);
	}

	// Selected block
	cube_element_t selected = {
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
	add_cube_element(&selected);

	draw_textf(3, settings.window_height - ((8 + 3) * settings.gui_scale), 'L', 
			"FPS: %1.2f, %1.3fms", framerate, frametime);

	if (debug_view)
		view_game_debug();
}