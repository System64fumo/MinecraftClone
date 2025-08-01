#ifndef PROFILER_H
#define PROFILER_H

#include <time.h>
#include <stdbool.h>

#define MAX_TIMERS 32
#define MAX_NAME_LENGTH 64

#define PROFILER_ID_SHADER 0
#define PROFILER_ID_MESH 1
#define PROFILER_ID_MERGE 2
#define PROFILER_ID_RENDER 3
#define PROFILER_ID_UI 4
#define PROFILER_ID_CULLING 5
#define PROFILER_ID_FRAMEBUFFER 6
#define PROFILER_ID_WORLD_GEN 7

typedef struct {
	char name[MAX_NAME_LENGTH];
	struct timespec cpu_start_time;
	struct timespec cpu_end_time;
	unsigned int gpu_query_start;
	unsigned int gpu_query_end;
	unsigned int gpu_time_elapsed;
	double cpu_time;
	double gpu_time;
	int active;
} ProfilerInstance;

void profiler_init();
int profiler_create(const char* name);
void profiler_start(int timer_id, bool time_gpu);
void profiler_stop(int timer_id, bool time_gpu);
void profiler_print_all();
void profiler_cleanup();

#endif