#include "main.h"
#include "shaders.h"
#include "framebuffer.h"
#include "gui.h"
#include <stdio.h>

unsigned int world_shader, post_process_shader, ui_shader, skybox_shader, clouds_shader;

unsigned int model_uniform_location            = -1;
unsigned int atlas_uniform_location            = -1;
unsigned int view_uniform_location             = -1;
unsigned int projection_uniform_location       = -1;
unsigned int ui_projection_uniform_location    = -1;
unsigned int ui_state_uniform_location         = -1;
unsigned int highlight_uniform_location        = -1;
unsigned int screen_texture_uniform_location   = -1;
unsigned int texture_fb_depth_uniform_location = -1;
unsigned int far_uniform_location              = -1;
unsigned int inv_projection_uniform_location   = -1;
unsigned int inv_view_uniform_location         = -1;
unsigned int sky_brightness_uniform_location   = -1;
unsigned int post_sky_brightness_uniform_location   = -1;
unsigned int clouds_sky_brightness_uniform_location = -1;

static unsigned int compile_shader(const char *src, int type) {
	unsigned int s = glCreateShader(type);
	glShaderSource(s, 1, &src, NULL);
	glCompileShader(s);
	int ok;
	glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(s, 512, NULL, log);
		printf("Shader compile error: %s\n", log);
		return 0;
	}
	return s;
}

static unsigned int load_shader(const char *vert_path, const char *frag_path) {
	unsigned int prog = glCreateProgram();
	unsigned int vs   = compile_shader(load_file(vert_path), GL_VERTEX_SHADER);
	unsigned int fs   = compile_shader(load_file(frag_path), GL_FRAGMENT_SHADER);
	glAttachShader(prog, vs);
	glAttachShader(prog, fs);
	glLinkProgram(prog);
	glDeleteShader(vs);
	glDeleteShader(fs);
	return prog;
}

void load_shaders(void) {
#ifdef DEBUG
	profiler_start(PROFILER_ID_SHADER, false);
#endif
	world_shader        = load_shader("../shaders/world.vert",       "../shaders/world.frag");
	post_process_shader = load_shader("../shaders/postprocess.vert", "../shaders/postprocess.frag");
	ui_shader           = load_shader("../shaders/ui.vert",          "../shaders/ui.frag");
	skybox_shader       = load_shader("../shaders/skybox.vert",      "../shaders/skybox.frag");
	clouds_shader       = load_shader("../shaders/clouds.vert",      "../shaders/clouds.frag");
	load_shader_constants();
#ifdef DEBUG
	profiler_stop(PROFILER_ID_SHADER, false);
#endif
}

void load_shader_constants(void) {
	glUseProgram(post_process_shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_fb_color);
	glUniform1i(screen_texture_uniform_location, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_fb_depth);
	glUniform1i(texture_fb_depth_uniform_location, 1);
	glUniform1f(far_uniform_location, far);
	glUniform1i(ui_state_uniform_location, ui_state);
}

void cache_uniform_locations(void) {
	model_uniform_location            = glGetUniformLocation(world_shader,        "model");
	atlas_uniform_location            = glGetUniformLocation(world_shader,        "textureAtlas");
	view_uniform_location             = glGetUniformLocation(world_shader,        "view");
	projection_uniform_location       = glGetUniformLocation(world_shader,        "projection");
	highlight_uniform_location        = glGetUniformLocation(world_shader,        "highlight");
	sky_brightness_uniform_location   = glGetUniformLocation(world_shader,        "sky_brightness");
	ui_projection_uniform_location    = glGetUniformLocation(ui_shader,           "projection");
	ui_state_uniform_location         = glGetUniformLocation(post_process_shader, "ui_state");
	screen_texture_uniform_location   = glGetUniformLocation(post_process_shader, "screenTexture");
	texture_fb_depth_uniform_location = glGetUniformLocation(post_process_shader, "u_texture_fb_depth");
	inv_projection_uniform_location   = glGetUniformLocation(post_process_shader, "u_inv_projection");
	inv_view_uniform_location         = glGetUniformLocation(post_process_shader, "u_inv_view");
	far_uniform_location              = glGetUniformLocation(post_process_shader, "u_far");
	post_sky_brightness_uniform_location   = glGetUniformLocation(post_process_shader, "u_sky_brightness");
	clouds_sky_brightness_uniform_location = glGetUniformLocation(clouds_shader,       "u_sky_brightness");
}
