#include "main.h"
#include "entity.h"
#include "framebuffer.h"
#include "skybox.h"
#include "gui.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

double lastFrame   = -1.0f;
double delta_time  = 0.0f;
double time_current= 0.0f;
double time_difference = 1.0f;
double time_previous   = 0.0f;
int    time_counter    = 0;
float  framerate   = 0.0f;
float  frametime   = 0.0f;
float  last_fov    = false;
static double frame_start_time = 0.0;
static double sleep_adjustment  = 0.0;

void limit_fps(void) {
	double target_fps = !game_focused ? 10.0
	    : (settings.vsync || settings.fps_limit == 0) ? 0.0 : settings.fps_limit;
	if (target_fps > 0) {
		double target_ft  = 1.0 / target_fps;
		double frame_time = time_current - frame_start_time;
		double remaining  = target_ft - frame_time - sleep_adjustment;
		if (remaining > 0) {
			struct timespec st = {
				.tv_sec  = (time_t)remaining,
				.tv_nsec = (long)((remaining - (time_t)remaining) * 1e9)
			};
			nanosleep(&st, NULL);
			double after = glfwGetTime();
			sleep_adjustment = (after - time_current) - remaining;
			time_current = after;
		} else {
			sleep_adjustment = 0.0;
		}
	}
	frame_start_time = time_current;
}

void do_time_stuff(void) {
	time_current   = glfwGetTime();
	delta_time     = time_current - lastFrame;
	lastFrame      = time_current;
	time_difference= time_current - time_previous;
	time_counter++;

	if (frustum_changed) {
		update_frustum();
		frustum_changed = false;
	}

	if (last_fov != settings.fov) {
		set_fov(settings.fov);
		last_fov = settings.fov;
	}

	if (mesh_needs_rebuild)
		rebuild_combined_visible_mesh();

	// Every frame: check for new chunks to load and enqueue missing columns.
	// This runs cheaply — it only enqueues slots not already in the heap.
	if (ui_state == UI_STATE_RUNNING)
		load_around_entity(&global_entities[0]);

	// 20 TPS tick
	if (time_difference >= 0.05f) {
		framerate    = (1.0 / time_difference) * time_counter;
		frametime    = (time_difference / time_counter) * 1000;
		time_previous= time_current;
		time_counter = 0;

		if (ui_state == UI_STATE_RUNNING)
			update_clouds();

		process_chunks();
	}

	// 1 TPS tick
	if (fmod(time_current, 1.0f) < delta_time) {
		if (ui_state == UI_STATE_RUNNING)
			update_ui();

#ifdef DEBUG
		profiler_print_all();

		uint32_t total_ov = 0, total_oi = 0, total_tv = 0, total_ti = 0;
		uint32_t loaded = 0, visible = 0;
		for (int x = 0; x < settings.render_distance; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++) {
				for (int z = 0; z < settings.render_distance; z++) {
					Chunk *c = &chunks[x][y][z];
					if (c->is_loaded) loaded++;
					if (visibility_map[x][y][z]) visible++;
					if (!visibility_map[x][y][z]) continue;
					for (int f = 0; f < 6; f++) {
						total_ov += c->faces[f].vertex_count;
						total_oi += c->faces[f].index_count;
						total_tv += c->transparent_faces[f].vertex_count;
						total_ti += c->transparent_faces[f].index_count;
					}
				}
			}
		}

		size_t fb_mem = 0;
		if (texture_fb_color) {
			GLint w, h, fmt;
			glBindTexture(GL_TEXTURE_2D, texture_fb_color);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,           &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT,          &h);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
			fb_mem += w * h * (fmt == GL_RGB8 ? 3 : 4);
		}
		if (texture_fb_depth) {
			GLint w, h;
			glBindTexture(GL_TEXTURE_2D, texture_fb_depth);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &w);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &h);
			fb_mem += w * h * 4;
		}

		printf("Chunks: %u loaded, %u visible\n", loaded, visible);
		printf("Verts: %u opaque, %u transparent\n", total_ov, total_tv);
		printf("Indices: %u opaque, %u transparent\n", total_oi, total_ti);
		printf("Framebuffer: %zukb\n", fb_mem / 1024);
		printf("Draw calls: %d\n", draw_calls);
		printf("FPS: %.1f (%.2fms)\n", framerate, frametime);
#endif
	}
}

const char* load_file(const char *filename) {
	char cur[1024];
	if (!realpath(__FILE__, cur)) return NULL;
	char *slash = strrchr(cur, '/');
	if (slash) *slash = '\0';
	char full[1025];
	snprintf(full, sizeof(full), "%s/%s", cur, filename);
	char resolved[1024];
	if (!realpath(full, resolved)) return NULL;
	FILE *f = fopen(resolved, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = malloc(sz + 1);
	if (!buf) { fclose(f); return NULL; }
	if (fread(buf, 1, sz, f) != (size_t)sz) { free(buf); fclose(f); return NULL; }
	buf[sz] = '\0';
	fclose(f);
	return buf;
}

int write_binary_file(const char *filename, const void *data, size_t size) {
	FILE *f = fopen(filename, "wb");
	if (!f) return -1;
	size_t written = fwrite(data, 1, size, f);
	fclose(f);
	return written == size ? 0 : -1;
}

void *read_binary_file(const char *filename, size_t *size) {
	FILE *f = fopen(filename, "rb");
	if (!f) return NULL;
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	rewind(f);
	void *data = malloc(*size);
	if (!data) { fclose(f); return NULL; }
	fread(data, 1, *size, f);
	fclose(f);
	return data;
}

void *create_array(size_t size, size_t element_size) {
	void *arr = malloc(size * element_size);
	if (!arr) { perror("malloc failed"); exit(EXIT_FAILURE); }
	return arr;
}
