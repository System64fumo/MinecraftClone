#include "main.h"
#include <stdio.h>

unsigned int world_shader, post_process_shader, ui_shader;

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
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_SHADER, false);
	#endif
}
