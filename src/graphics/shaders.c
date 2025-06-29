#include "main.h"
#include "shaders.h"
#include "framebuffer.h"
#include "gui.h"
#include <stdio.h>

unsigned int world_shader, post_process_shader, ui_shader, skybox_shader, clouds_shader;

unsigned int compile_shader(const char* shader_source, int type) {
	int success;
	char infoLog[512];
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &shader_source, NULL);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		printf("Shader compilation failed: %s\n", infoLog);
		return 0;
	}
	return shader;
}

unsigned int load_shader(const char* vertex_path, const char* fragment_path) {
	unsigned int shader = glCreateProgram();
	uint32_t vertex_shader = compile_shader(load_file(vertex_path), GL_VERTEX_SHADER);
	uint32_t fragment_shader = compile_shader(load_file(fragment_path), GL_FRAGMENT_SHADER);

	glAttachShader(shader, vertex_shader);
	glAttachShader(shader, fragment_shader);
	glLinkProgram(shader);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader;
}

void load_shaders() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_SHADER, false);
	#endif
	world_shader = load_shader("../shaders/world.vert", "../shaders/world.frag");
	post_process_shader = load_shader("../shaders/postprocess.vert", "../shaders/postprocess.frag");
	ui_shader = load_shader("../shaders/ui.vert", "../shaders/ui.frag");
	skybox_shader = load_shader("../shaders/skybox.vert", "../shaders/skybox.frag");
	clouds_shader = load_shader("../shaders/clouds.vert", "../shaders/clouds.frag");
	load_shader_constants();
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_SHADER, false);
	#endif
}

void load_shader_constants() {
	glUseProgram(post_process_shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_fb_color);
	glUniform1i(screen_texture_uniform_location, 0);
	
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture_fb_depth);
	glUniform1i(texture_fb_depth_uniform_location, 1);

	glUniform1f(near_uniform_location, near);
	glUniform1f(far_uniform_location, far);
	glUniform1i(ui_state_uniform_location, ui_state);
}

void cache_uniform_locations() {
	model_uniform_location = glGetUniformLocation(world_shader, "model");
	atlas_uniform_location = glGetUniformLocation(world_shader, "textureAtlas");
	view_uniform_location = glGetUniformLocation(world_shader, "view");
	projection_uniform_location = glGetUniformLocation(world_shader, "projection");
	ui_projection_uniform_location = glGetUniformLocation(ui_shader, "projection");
	ui_state_uniform_location = glGetUniformLocation(post_process_shader, "ui_state");
	screen_texture_uniform_location = glGetUniformLocation(post_process_shader, "screenTexture");
	texture_fb_depth_uniform_location = glGetUniformLocation(post_process_shader, "u_texture_fb_depth");
	near_uniform_location = glGetUniformLocation(post_process_shader, "u_near");
	far_uniform_location = glGetUniformLocation(post_process_shader, "u_far");
}