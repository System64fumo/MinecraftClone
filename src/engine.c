#include "main.h"
#include "world.h"
#include "config.h"
#include "gui.h"
#include "framebuffer.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

unsigned short screen_center_x = 640;
unsigned short screen_center_y = 360;

uint8_t hotbar_slot = 0;

mat4 model, view, projection;
unsigned int block_textures, ui_textures, font_textures;

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
GLFWwindow* window = NULL;

int initialize() {
	initialize_config();

	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
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
	framebuffer_size_callback(window, settings.window_width, settings.window_height);

	// Debugging
	#ifdef DEBUG
	glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
	printf("OpenGL Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
	printf("OpenGL Renderer: %s\n", (const char*)glGetString(GL_RENDERER));
	printf("OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));

	profiler_init();
	profiler_create("Shaders");
	profiler_create("Mesh");
	profiler_create("Merge");
	profiler_create("Render");
	profiler_create("GUI");
	profiler_create("World");
	#endif

	// Textures
	char exec_path[1024];
	ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1);
	if (len == -1) {
		perror("readlink failed");
		exit(EXIT_FAILURE);
	}
	exec_path[len] = '\0';

	char* lastSlash = strrchr(exec_path, '/');
	if (lastSlash)
		*lastSlash = '\0';

	char font_path[1024];
	if (snprintf(font_path, sizeof(font_path), "%s/%s", exec_path, "assets/font.webp") >= sizeof(font_path)) {
		fprintf(stderr, "font_path truncated\n");
		exit(EXIT_FAILURE);
	}
	font_textures = load_texture(font_path);

	char atlas_path[1024];
	if (snprintf(atlas_path, sizeof(atlas_path), "%s/%s", exec_path, "assets/atlas.webp") >= sizeof(atlas_path)) {
		fprintf(stderr, "atlas_path truncated\n");
		exit(EXIT_FAILURE);
	}
	block_textures = load_texture(atlas_path);

	char gui_path[1024];
	if (snprintf(gui_path, sizeof(gui_path), "%s/%s", exec_path, "assets/gui.webp") >= sizeof(gui_path)) {
		fprintf(stderr, "gui_path truncated\n");
		exit(EXIT_FAILURE);
	}
	ui_textures = load_texture(gui_path);

	// Initialization
	load_shaders();
	setup_framebuffer(settings.window_width, settings.window_height);
	init_fullscreen_quad();
	init_ui();
	init_gl_buffers();
	start_world_gen_thread();

	// Cache uniform locations
	model_uniform_location = glGetUniformLocation(world_shader, "model");
	atlas_uniform_location = glGetUniformLocation(world_shader, "textureAtlas");
	view_uniform_location = glGetUniformLocation(world_shader, "view");
	projection_uniform_location = glGetUniformLocation(world_shader, "projection");
	ui_projection_uniform_location = glGetUniformLocation(ui_shader, "projection");
	ui_state_uniform_location = glGetUniformLocation(post_process_shader, "ui_state");

	chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthFunc(GL_LESS);
	//glEnable(GL_MULTISAMPLE);

	// Initialize player
	global_entities[0] = create_entity(0);
	global_entities[0].pos.y = 73;
	global_entities[0].yaw = 90;

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

	free_chunks(chunks, RENDER_DISTANCE, WORLD_HEIGHT);
	glDeleteProgram(world_shader);
	glfwDestroyWindow(window);
	glfwTerminate();
}