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

void ini_parse(IniFile* ini) {
	FILE* f = fopen(ini->filepath, "r");
	if (!f) return;

	char line[MAX_LINE];
	IniSection* current = NULL;
	ini->section_count = 0;

	while (fgets(line, sizeof(line), f)) {
		char* trimmed = line;
		while (*trimmed == ' ' || *trimmed == '\t') trimmed++;

		if (*trimmed == '#' || *trimmed == ';' || *trimmed == '\n') continue;

		if (*trimmed == '[') {
			char* end = strchr(trimmed, ']');
			if (end) {
				*end = 0;
				if (ini->section_count < MAX_SECTIONS) {
					current = &ini->sections[ini->section_count++];
					strncpy(current->name, trimmed + 1, sizeof(current->name));
					current->kv_count = 0;
				}
			}
		} else {
			char* eq = strchr(trimmed, '=');
			if (eq && current && current->kv_count < MAX_KEYS) {
				*eq = 0;
				char* key = trimmed;
				char* value = eq + 1;
				key[strcspn(key, "\r\n")] = 0;
				value[strcspn(value, "\r\n")] = 0;
				IniKeyValue* kv = &current->kv[current->kv_count++];
				strncpy(kv->key, key, sizeof(kv->key));
				strncpy(kv->value, value, sizeof(kv->value));
			}
		}
	}

	fclose(f);
}

const char* ini_get(IniFile* ini, const char* section, const char* key) {
	for (int i = 0; i < ini->section_count; i++) {
		if (strcmp(ini->sections[i].name, section) == 0) {
			for (int j = 0; j < ini->sections[i].kv_count; j++) {
				if (strcmp(ini->sections[i].kv[j].key, key) == 0) {
					return ini->sections[i].kv[j].value;
				}
			}
		}
	}
	return NULL;
}

void* ini_watch_thread(void* arg) {
	IniFile* ini = (IniFile*)arg;

	struct pollfd pfd = {
		.fd = ini->watch_fd,
		.events = POLLIN
	};

	while (ini->watching) {
		if (poll(&pfd, 1, -1) > 0 && (pfd.revents & POLLIN)) {
			char buffer[WATCH_BUF_SIZE * 16];
			int length = read(ini->watch_fd, buffer, sizeof(buffer));
			if (length <= 0) continue;

			int i = 0;
			while (i < length) {
				struct inotify_event* event = (struct inotify_event*)&buffer[i];
				if (event->mask & IN_CLOSE_WRITE) {
					pthread_mutex_lock(&ini->mutex);
					ini_parse(ini);
					parse_config();
					pthread_mutex_unlock(&ini->mutex);
				}
				i += sizeof(struct inotify_event) + event->len;
			}
		}
	}

	return NULL;
}

void ini_start_watch_thread(IniFile* ini) {
	ini->watch_fd = inotify_init1(IN_NONBLOCK);
	if (ini->watch_fd < 0) {
		perror("inotify_init1");
		exit(1);
	}

	ini->watch_desc = inotify_add_watch(ini->watch_fd, ini->filepath, IN_CLOSE_WRITE);
	if (ini->watch_desc < 0) {
		perror("inotify_add_watch");
		close(ini->watch_fd);
		exit(1);
	}

	pthread_mutex_init(&ini->mutex, NULL);
	ini->watching = 1;
	pthread_t thread;
	pthread_create(&thread, NULL, ini_watch_thread, ini);
	pthread_detach(thread);
}

void ini_stop_watch_thread(IniFile* ini) {
	ini->watching = 0;
	inotify_rm_watch(ini->watch_fd, ini->watch_desc);
	close(ini->watch_fd);
	pthread_mutex_destroy(&ini->mutex);
}

int ini_check_reload(IniFile* ini) {
	char buffer[WATCH_BUF_SIZE * 16];
	int length = read(ini->watch_fd, buffer, sizeof(buffer));
	if (length <= 0) return 0;

	int i = 0;
	while (i < length) {
		struct inotify_event* event = (struct inotify_event*)&buffer[i];
		if (event->mask & IN_CLOSE_WRITE) {
			ini_parse(ini);
			return 1;
		}
		i += sizeof(struct inotify_event) + event->len;
	}

	return 0;
}

void initialize_config() {
	char config_path[1024];
	const char* home_path = getenv("HOME");
	snprintf(config_path, sizeof(config_path), "%s/%s", home_path, ".config/ccraft/config.ini");
	if (!access(config_path, F_OK) == 0) {
		char* dir_path = strdup(config_path);
		char* last_slash = strrchr(dir_path, '/');
		if (last_slash) {
			*last_slash = '\0';
			mkdir(dir_path, 0755);
		}

		FILE* config_file = fopen(config_path, "w");
		fprintf(config_file, "[main]\n");
		fprintf(config_file, "screen_width = %d\n", screen_width);
		fprintf(config_file, "screen_height = %d\n", screen_height);
		fprintf(config_file, "fov = %f\n", fov);
		fprintf(config_file, "near = %f\n", near);
		fprintf(config_file, "far = %f\n", far);
		fprintf(config_file, "aspect = %f\n", aspect);
		fclose(config_file);
	}

	IniFile ini = { .filepath = config_path };
	ini_parse(&ini);
	ini_start_watch_thread(&ini);
}

void parse_config() {
	// TODO: Implement config parsing logic here
	printf("Config reloaded.\n");
}
