#ifdef DEBUG
#include "main.h"
#include <stdio.h>
#include <string.h>

static ProfilerInstance timers[MAX_TIMERS];
static int num_timers = 0;

void profiler_init() {
	memset(timers, 0, sizeof(timers));
}

int profiler_create(const char* name) {
	if (num_timers >= MAX_TIMERS) return -1;
	
	int idx = num_timers++;
	strncpy(timers[idx].name, name, MAX_NAME_LENGTH-1);
	timers[idx].active = 1;
	glGenQueries(1, &timers[idx].gpu_query_start);
	glGenQueries(1, &timers[idx].gpu_query_end);
	return idx;
}

void profiler_start(int timer_id, bool time_gpu) {
	if (timer_id < 0 || timer_id >= num_timers || !timers[timer_id].active) return;
	
	// Start CPU timing
	clock_gettime(CLOCK_MONOTONIC, &timers[timer_id].cpu_start_time);
	
	// Start GPU timing
	if (time_gpu)
		glQueryCounter(timers[timer_id].gpu_query_start, GL_TIMESTAMP);
}

void profiler_stop(int timer_id, bool time_gpu) {
	if (timer_id < 0 || timer_id >= num_timers || !timers[timer_id].active) return;
	
	// Stop CPU timing
	clock_gettime(CLOCK_MONOTONIC, &timers[timer_id].cpu_end_time);
	
	// Stop GPU timing
	if (time_gpu)
		glQueryCounter(timers[timer_id].gpu_query_end, GL_TIMESTAMP);
	
	// Wait for GPU queries to be available
	GLuint64 start_time, end_time;
	if (time_gpu) {
		GLint gpu_available = 0;
		while (!gpu_available) {
			glGetQueryObjectiv(timers[timer_id].gpu_query_end, GL_QUERY_RESULT_AVAILABLE, &gpu_available);
		}

		// Get GPU timing results
		glGetQueryObjectui64v(timers[timer_id].gpu_query_start, GL_QUERY_RESULT, &start_time);
		glGetQueryObjectui64v(timers[timer_id].gpu_query_end, GL_QUERY_RESULT, &end_time);
	}

	// Calculate CPU time in milliseconds
	timers[timer_id].cpu_time = 
		(timers[timer_id].cpu_end_time.tv_sec - timers[timer_id].cpu_start_time.tv_sec) * 1000.0 +
		(timers[timer_id].cpu_end_time.tv_nsec - timers[timer_id].cpu_start_time.tv_nsec) / 1000000.0;
	
	// Calculate GPU time in milliseconds
	if (time_gpu)
		timers[timer_id].gpu_time = (end_time - start_time) / 1000000.0;
}

void profiler_print_all() {
	printf("\e[1;1H\e[2J");
	printf("%-20s %-15s %-15s\n", "Profiler Name", "CPU", "GPU");
	printf("--------------------------------------------------\n");
	for (int i = 0; i < num_timers; i++) {
		if (timers[i].active) {
			printf("%-20s %-15.3f %-15.3f\n", 
				timers[i].name, 
				timers[i].cpu_time, 
				timers[i].gpu_time);
		}
	}
	printf("\n");
}

void profiler_cleanup() {
	for (int i = 0; i < num_timers; i++) {
		if (timers[i].active) {
			glDeleteQueries(1, &timers[i].gpu_query_start);
			glDeleteQueries(1, &timers[i].gpu_query_end);
		}
	}
	num_timers = 0;
}
#endif
