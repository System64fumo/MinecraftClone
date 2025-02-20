#include "main.h"

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

void load_shaders() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_SHADER);
	#endif
	shaderProgram = glCreateProgram();
	uint32_t vertex_shader = compile_shader(vertexShaderSource, GL_VERTEX_SHADER);
	uint32_t fragment_shader = compile_shader(fragmentShaderSource, GL_FRAGMENT_SHADER);

	glAttachShader(shaderProgram, vertex_shader);
	glAttachShader(shaderProgram, fragment_shader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_SHADER);
	#endif
}