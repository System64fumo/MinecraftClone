#include <pthread.h>

#define MAX_SECTIONS 32
#define MAX_KEYS 64
#define MAX_LINE 256
#define WATCH_BUF_SIZE (sizeof(struct inotify_event) + NAME_MAX + 1)

typedef struct {
	char key[64];
	char value[128];
} IniKeyValue;

typedef struct {
	char name[64];
	IniKeyValue kv[MAX_KEYS];
	int kv_count;
} IniSection;

typedef struct {
	IniSection sections[MAX_SECTIONS];
	int section_count;
	const char* filepath;
	int watch_fd;
	int watch_desc;
	pthread_mutex_t mutex;
	int watching;
} IniFile;

void ini_parse(IniFile* ini);
const char* ini_get(IniFile* ini, const char* section, const char* key);
void* ini_watch_thread(void* arg);
void ini_start_watch_thread(IniFile* ini);
void ini_stop_watch_thread(IniFile* ini);
int ini_check_reload(IniFile* ini);
void initialize_config();
void parse_config();