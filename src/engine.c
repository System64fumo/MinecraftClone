#include "main.h"
#include "world.h"
#include "config.h"
#include "gui.h"
#include "skybox.h"
#include "textures.h"
#include "framebuffer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

unsigned short screen_center_x = 640;
unsigned short screen_center_y = 360;

uint8_t hotbar_slot = 0;

mat4 model, view, projection;

// Type, Translucent, Face textures
// t,	t,		   f,f,f,f,f,f
uint8_t block_data[MAX_BLOCK_TYPES][8] = {
	[0 ... 255] = {0, 0, 31, 31, 31, 31, 31, 31},	// Null
	[0] =  {0, 1, 0,   0,   0,   0,   0,   0  },	// Air
	[1] =  {0, 0, 3,   3,   3,   3,   3,   3  },	// Dirt
	[2] =  {0, 0, 4,   4,   4,   4,   3,   1  },	// Grass
	[3] =  {0, 0, 2,   2,   2,   2,   2,   2  },	// Stone
	[4] =  {0, 0, 17,  17,  17,  17,  17,  17 },	// Cobblestone
	[5] =  {0, 0, 5,   5,   5,   5,   5,   5  },	// Planks
	[6] =  {2, 1, 16,  16,  16,  16,  16,  16 },	// Sapling
	[7] =  {0, 0, 18,  18,  18,  18,  18,  18 },	// Bedrock
	[8] =  {0, 1, 206, 206, 206, 206, 206, 206},	// Flowing water
	[9] =  {0, 1, 206, 206, 206, 206, 206, 206},	// Stationary water
	[10] = {0, 0, 238, 238, 238, 238, 238, 238},	// Flowing lava
	[11] = {0, 0, 238, 238, 238, 238, 238, 238},	// Stationary lava
	[12] = {0, 0, 19,  19,  19,  19,  19,  19 },	// Sand
	[13] = {0, 0, 20,  20,  20,  20,  20,  20 },	// Gravel
	[14] = {0, 0, 33,  33,  33,  33,  33,  33 },	// Gold Ore
	[15] = {0, 0, 34,  34,  34,  34,  34,  34 },	// Iron Ore
	[16] = {0, 0, 35,  35,  35,  35,  35,  35 },	// Coal Ore
	[17] = {0, 0, 21,  21,  21,  21,  22,  22 },	// Wood
	[18] = {0, 1, 53,  53,  53,  53,  53,  53 },	// Leaves
	[19] = {0, 0, 49,  49,  49,  49,  49,  49 },	// Sponge
	[20] = {0, 1, 50,  50,  50,  50,  50,  50 },	// Glass
	[21] = {0, 0, 161, 161, 161, 161, 161, 161},	// Lapis ore
	[22] = {0, 0, 145, 145, 145, 145, 145, 145},	// Lapis block
	[23] = {0, 0, 47,  46,  46,  46,  63 , 63 },	// Dispenser
	[24] = {0, 0, 193, 193, 193, 193, 209, 177},	// Sandstone
	[25] = {0, 0, 75,  75,  75,  75,  75,  76 },	// Noteblock
	[26] = {1, 1, 151, 0,   151, 150, 5,   135},	// Bed (Part 1)
	[44] = {1, 1, 6,   6,   6,   6,   7,   7  },	// Slab
	[89] = {0, 0, 106, 106, 106, 106, 106, 106},	// Glowstone
};

Chunk*** chunks = NULL;
Entity global_entities[MAX_ENTITIES_PER_CHUNK];
unsigned int model_uniform_location = -1;
unsigned int atlas_uniform_location = -1;
unsigned int view_uniform_location = -1;
unsigned int projection_uniform_location = -1;
unsigned int ui_projection_uniform_location = -1;
unsigned int ui_state_uniform_location = -1;
unsigned int screen_texture_uniform_location = -1;
unsigned int texture_fb_depth_uniform_location = -1;
unsigned int near_uniform_location = -1;
unsigned int far_uniform_location = -1;

int initialize() {
	initialize_config();
	if (initialize_window() == -1)
		return -1;
	load_textures();
	load_shaders();
	setup_framebuffer(settings.window_width, settings.window_height);
	init_fullscreen_quad();
	init_ui();
	init_gl_buffers();
	skybox_init();
	start_world_gen_thread();
	cache_uniform_locations();

	chunks = allocate_chunks();

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_MULTISAMPLE);

	// Initialize player
	global_entities[0] = create_entity(0);
	global_entities[0].pos.y = 73;
	global_entities[0].yaw = 90;

	framebuffer_size_callback(window, settings.window_width, settings.window_height);
	return 0;
}

void run() {
	update_frustum();
	load_around_entity(&global_entities[0]);

	while (!glfwWindowShouldClose(window)) {
		do_time_stuff();
		process_input(window);

		render_to_framebuffer();
		render_to_screen();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void shutdown() {
	cleanup_framebuffer();
	cleanup_ui();
	cleanup_renderer();

	stop_world_gen_thread();

	pthread_mutex_lock(&chunks_mutex);
	for(uint8_t cx = 0; cx < RENDER_DISTANCE; cx++) {
		for(uint8_t cy = 0; cy < WORLD_HEIGHT; cy++) {
			for(uint8_t cz = 0; cz < RENDER_DISTANCE; cz++) {
				Chunk* chunk = &chunks[cx][cy][cz];
				unload_chunk(chunk);
			}
		}
	}
	pthread_mutex_unlock(&chunks_mutex);

	free_chunks(chunks);
	glDeleteProgram(world_shader);
	glfwDestroyWindow(window);
	glfwTerminate();
}