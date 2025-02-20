#include "main.h"

const char* postProcessingVertexShaderSource = 
"#version 330 core\n"
"layout (location = 0) in vec2 aPos;\n"
"layout (location = 1) in vec2 aTexCoords;\n"
"out vec2 TexCoords;\n"
"void main() {\n"
"    TexCoords = aTexCoords;\n"
"    gl_Position = vec4(aPos, 0.0, 1.0);\n"
"}\n";

const char* postProcessingFragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"in vec2 TexCoords;\n"
"uniform sampler2D screenTexture;\n"
"void main() {\n"
"    FragColor = texture(screenTexture, TexCoords);\n"
"}\n";

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

	postProcessingShader = glCreateProgram();
	uint32_t postprocess_vertex_shader = compile_shader(postProcessingVertexShaderSource, GL_VERTEX_SHADER);
	uint32_t postprocess_fragment_shader = compile_shader(postProcessingFragmentShaderSource, GL_FRAGMENT_SHADER);

	glAttachShader(postProcessingShader, postprocess_vertex_shader);
	glAttachShader(postProcessingShader, postprocess_fragment_shader);
	glLinkProgram(postProcessingShader);

	glDeleteShader(postprocess_vertex_shader);
	glDeleteShader(postprocess_fragment_shader);

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_SHADER);
	#endif
}
