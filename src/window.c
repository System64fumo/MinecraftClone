#include "main.h"
#include "framebuffer.h"
#include "gui.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// TODO: Write changes back to config file
	settings.window_width = width;
	settings.window_height = height;

	aspect = (float)settings.window_width / (float)settings.window_height;
	screen_center_x = settings.window_width / 2;
	screen_center_y = settings.window_height / 2;

	glViewport(0, 0, settings.window_width, settings.window_height);
	matrix4_identity(projection);
	matrix4_perspective(projection, settings.fov * M_PI / 180.0f, aspect, near, far);
	setup_framebuffer(width, height);
	update_ui();
}