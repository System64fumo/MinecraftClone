#ifndef CONFIG_H
#define CONFIG_H

#define MAX_SECTIONS 32
#define MAX_KEYS 64
#define MAX_NAME 64
#define MAX_VALUE 256

typedef struct {
	char key[MAX_NAME];
	char value[MAX_VALUE];
} IniKey;

typedef struct {
	char name[MAX_NAME];
	IniKey keys[MAX_KEYS];
	int key_count;
} IniSection;

typedef struct {
	IniSection sections[MAX_SECTIONS];
	int section_count;
	const char* filename;
	int ready;
} IniFile;

typedef struct {
	float fov;
	float fov_desired;
	float sky_brightness;
	unsigned short window_width;
	unsigned short window_height;
	uint8_t render_distance;
	bool frustum_culling;
	bool fancy_graphics;
} config;

extern IniFile ini;
extern config settings;

void initialize_config();
const char* ini_get(IniFile* ini, const char* section, const char* key);

#endif