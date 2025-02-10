#include <GL/glew.h>
#include <GL/gl.h>
#include <time.h>

#define MAX_TIMERS 32
#define MAX_NAME_LENGTH 64

#define PROFILER_ID_HUD 0
#define PROFILER_ID_BAKE 1
#define PROFILER_ID_RENDER 2
#define PROFILER_ID_WORLD_GEN 3

typedef struct {
    char name[MAX_NAME_LENGTH];
    struct timespec cpu_start_time;
    struct timespec cpu_end_time;
    GLuint gpu_query_start;
    GLuint gpu_query_end;
    GLuint64 gpu_time_elapsed;
    double cpu_time;
    double gpu_time;
    int active;
} ProfilerInstance;

void profiler_init();
int profiler_create(const char* name);
void profiler_start(int timer_id);
void profiler_stop(int timer_id);
void profiler_print_all();
void profiler_cleanup();