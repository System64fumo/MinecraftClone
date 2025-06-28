#include "main.h"
#include "framebuffer.h"
#include "shaders.h"
#include "config.h"
#include "gui.h"
#include <stdio.h>

float near = 0.1f;
float far = 500.0f;
float aspect = 1.7f;
GLFWwindow* window = NULL;

int initialize_window() {
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	#ifndef DEBUG
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	#endif
	//glfwWindowHint(GLFW_SAMPLES, 8);

	window = glfwCreateWindow(settings.window_width, settings.window_height, "Minecraft Clone", NULL, NULL);
	if (!window) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, key_callback);

	glewInit();

	// Debugging
	#ifdef DEBUG
	printf("OpenGL Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
	printf("OpenGL Renderer: %s\n", (const char*)glGetString(GL_RENDERER));
	printf("OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));

	profiler_init();
	profiler_create("Shaders");
	profiler_create("Mesh");
	profiler_create("Merge");
	profiler_create("Render");
	profiler_create("GUI");
	profiler_create("Culling");
	profiler_create("Framebuffer");
	profiler_create("World");
	#endif

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// TODO: Write changes back to config file
	settings.window_width = width;
	settings.window_height = height;

	aspect = (float)settings.window_width / (float)settings.window_height;
	screen_center_x = settings.window_width / 2;
	screen_center_y = settings.window_height / 2;

	glViewport(0, 0, settings.window_width, settings.window_height);
	matrix4_identity(projection);
	matrix4_perspective(projection, settings.fov * DEG_TO_RAD, aspect, near, far);
	setup_framebuffer(width, height);
	update_ui();

	load_shader_constants();
}

void set_fov(float fov) {
	matrix4_perspective(projection, fov * DEG_TO_RAD, aspect, near, far);
}