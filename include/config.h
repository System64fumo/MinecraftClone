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
	pthread_t thread;
	int ready;
} IniFile;

typedef struct {
	float fov;
	float sky_brightness;
	unsigned short window_width;
	unsigned short window_height;
} config;

extern IniFile ini;

void initialize_config();
void ini_parse_async(const char* filename, IniFile* ini);
const char* ini_get(IniFile* ini, const char* section, const char* key);
void parse_config(IniFile* ini);