#include "main.h"
#include "framebuffer.h"
#include "gui.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;

	aspect = (float)screen_width / (float)screen_height;
	screen_center_x = screen_width / 2;
	screen_center_y = screen_height / 2;

	glViewport(0, 0, screen_width, screen_height);
	matrix4_identity(projection);
	matrix4_perspective(projection, fov * M_PI / 180.0f, aspect, near, far);
	setup_framebuffer(width, height);
	update_ui();
}