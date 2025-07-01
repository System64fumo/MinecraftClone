#include "main.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <limits.h>
#include <pthread.h>
#include <poll.h>
#include <ctype.h>

IniFile ini;
config settings;

void parse_config(IniFile* ini) {
	// TODO: Fix hot-reloading..
	const char* width = ini_get(ini, "main", "window_width");
	if (width)
		settings.window_width = atoi(width);

	const char* height = ini_get(ini, "main", "window_height");
	if (height)
		settings.window_height = atoi(height);

	const char* fov = ini_get(ini, "main", "fov");
	if (fov)
		settings.fov_desired = atof(fov);

	const char* distance = ini_get(ini, "render", "distance");
	if (distance) {
		settings.render_distance = atoi(distance) * 2;
		far = (settings.render_distance * 2) * CHUNK_SIZE;
	}

	const char* culling = ini_get(ini, "render", "culling");
	if (culling)
		settings.frustum_culling = culling[0] == 't' || culling[0] == 'T';

	const char* fancy = ini_get(ini, "render", "fancy");
	if (fancy)
		settings.fancy_graphics = fancy[0] == 't' || culling[0] == 'T';

	settings.sky_brightness = 1.0;

	printf("Config loaded: %dx%d, fov=%.1f\n", 
			settings.window_width,
			settings.window_height,
			settings.fov);
}

static char* trim(char* str) {
	while (isspace(*str)) str++;
	char* end = str + strlen(str) - 1;
	while (end > str && isspace(*end)) *end-- = '\0';
	return str;
}

static void ini_load_file(IniFile* ini) {
	FILE* f = fopen(ini->filename, "r");
	if (!f) return;

	ini->section_count = 0;
	char line[512];
	IniSection* current = NULL;

	while (fgets(line, sizeof(line), f)) {
		char* trimmed = trim(line);
		if (*trimmed == ';' || *trimmed == '#' || *trimmed == '\0') continue;

		if (*trimmed == '[') {
			char* end = strchr(trimmed, ']');
			if (end && ini->section_count < MAX_SECTIONS) {
				*end = '\0';
				current = &ini->sections[ini->section_count++];
				strncpy(current->name, trimmed + 1, MAX_NAME);
				current->key_count = 0;
			}
		} else if (current && strchr(trimmed, '=')) {
			if (current->key_count < MAX_KEYS) {
				char* eq = strchr(trimmed, '=');
				*eq = '\0';
				char* key = trim(trimmed);
				char* value = trim(eq + 1);
				IniKey* k = &current->keys[current->key_count++];
				strncpy(k->key, key, MAX_NAME);
				strncpy(k->value, value, MAX_VALUE);
			}
		}
	}

	fclose(f);
	ini->ready = 1;
	parse_config(ini);
}

void initialize_config() {
	settings.window_width = 845;
	settings.window_height = 480;
	settings.fov_desired = 70.0f;
	settings.fov = settings.fov_desired;
	settings.render_distance = 16;
	settings.frustum_culling = true;
	settings.fancy_graphics = true;

	char config_path[1024];
	const char* home_path = getenv("HOME");
	if (!home_path) {
		fprintf(stderr, "HOME environment variable not set\n");
		return;
	}

	snprintf(config_path, sizeof(config_path), "%s/.config/ccraft/config.ini", home_path);

	char dir_path[1024];
	snprintf(dir_path, sizeof(dir_path), "%s/.config/ccraft", home_path);
	mkdir(dir_path, 0755);
	
	if (access(config_path, F_OK) != 0) {
		FILE* config_file = fopen(config_path, "w");
		if (!config_file) {
			perror("fopen");
			return;
		}
		
		fprintf(config_file, "[main]\n");
		fprintf(config_file, "window_width = %d\n", settings.window_width);
		fprintf(config_file, "window_height = %d\n", settings.window_height);
		fprintf(config_file, "fov = %.1f\n", settings.fov);
		fprintf(config_file, "\n[render]\n");
		fprintf(config_file, "distance = %d\n", settings.render_distance / 2);
		fprintf(config_file, "culling = true\n");
		fprintf(config_file, "fancy = true\n");
		fclose(config_file);
	}
	ini.filename = config_path;
	ini_load_file(&ini);
}

const char* ini_get(IniFile* ini, const char* section, const char* key) {
	if (!ini->ready) return NULL;
	for (int i = 0; i < ini->section_count; i++) {
		if (strcmp(ini->sections[i].name, section) == 0) {
			for (int j = 0; j < ini->sections[i].key_count; j++) {
				if (strcmp(ini->sections[i].keys[j].key, key) == 0) {
					return ini->sections[i].keys[j].value;
				}
			}
		}
	}
	return NULL;
}