#include "main.h"
#include "engine.h"
#include "framebuffer.h"

int main() {
	if (initialize() != 0) {
		return -1;
	}

	// TODO: Move this to the engine file (Currently moving it causes segfaults in debug builds for whatever reason but works fine when ran through GDB??)
	while (!glfwWindowShouldClose(window)) {
		do_time_stuff();
		processInput(window);
	
		render_to_framebuffer();
		render_to_screen();
	
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	shutdown();

	return 0;
}