#include "main.h"
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

static void* ini_thread_func(void* arg) {
	IniFile* ini = (IniFile*)arg;

	ini_load_file(ini);

	int fd = inotify_init1(IN_NONBLOCK);
	if (fd < 0) return NULL;

	int wd = inotify_add_watch(fd, ini->filename, IN_MODIFY | IN_CLOSE_WRITE);
	if (wd < 0) {
		close(fd);
		return NULL;
	}

	char buf[sizeof(struct inotify_event) + NAME_MAX + 1];

	while (1) {
		int len = read(fd, buf, sizeof(buf));
		if (len <= 0) {
			usleep(100000); // avoid busy loop
			continue;
		}

		struct inotify_event* event = (struct inotify_event*)buf;
		if (event->mask & (IN_MODIFY | IN_CLOSE_WRITE)) {
			ini_load_file(ini);
		}
	}

	close(wd);
	close(fd);
	return NULL;
}

void ini_parse_async(const char* filename, IniFile* ini) {
	memset(ini, 0, sizeof(IniFile));
	ini->filename = filename;
	ini->ready = 0;
	pthread_create(&ini->thread, NULL, ini_thread_func, ini);
	pthread_detach(ini->thread);
}

void initialize_config() {
	settings.window_width = 1280;
	settings.window_height = 720;
	settings.fov = 70.0f;

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
		fclose(config_file);
	}
	ini_parse_async(config_path, &ini);
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
		settings.fov = atof(fov);

	printf("Config loaded: %dx%d, fov=%.1f\n", 
		   settings.window_width,
		   settings.window_height,
		   settings.fov);
}