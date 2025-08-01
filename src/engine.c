#include "main.h"
#include "entity.h"
#include "world.h"
#include "shaders.h"
#include "config.h"
#include "gui.h"
#include "skybox.h"
#include "textures.h"
#include "framebuffer.h"

uint8_t hotbar_slot = 0;
Chunk*** chunks = NULL;
Entity global_entities[MAX_ENTITIES_PER_CHUNK];

int initialize() {
	initialize_config();
	if (initialize_window() == -1)
		return -1;
	load_textures();
	load_shaders();
	setup_framebuffer(settings.window_width, settings.window_height);
	init_ui();
	init_gl_buffers();
	skybox_init();
	start_world_gen_thread();
	init_mesh_thread();
	cache_uniform_locations();

	chunks = allocate_chunks();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(2.0f);
	glfwSwapInterval(settings.vsync);

	// Initialize player
	global_entities[0] = create_entity(0);
	global_entities[0].pos.y = 73;
	global_entities[0].yaw = 90;

	framebuffer_size_callback(window, settings.window_width, settings.window_height);
	return 0;
}

void run() {
	while (!glfwWindowShouldClose(window)) {
		do_time_stuff();
		process_input(window, chunks);

		render_to_framebuffer();
		render_to_screen();

		glfwSwapBuffers(window);
		glfwPollEvents();
		limit_fps();
	}
}

void shutdown() {
	cleanup_framebuffer();
	cleanup_ui();
	cleanup_renderer();
	stop_world_gen_thread();
	glDeleteProgram(world_shader);
}