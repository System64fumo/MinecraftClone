#include "main.h"
#include "world.h"
#include "gui.h"
#include "framebuffer.h"

unsigned short screen_width = 1280;
unsigned short screen_height = 720;
unsigned short screen_center_x = 640;
unsigned short screen_center_y = 360;

uint8_t hotbar_slot = 0;
char game_dir[255];

float fov = 70.0f;
float near = 0.1f;
float far = 300.0f;
float aspect = 1.7f;

float model[16], view[16], projection[16];
unsigned int block_textures, ui_textures;

// Type, Translucent, Face textures
// t,    t,           f,f,f,f,f,f
uint8_t block_data[MAX_BLOCK_TYPES][8] = {
	[0] =  {0, 1, 0,   0,   0,   0,   0,   0  },	// Air
	[1] =  {0, 0, 3,   3,   3,   3,   3,   3  },	// Dirt
	[2] =  {0, 0, 4,   4,   4,   4,   3,   1  },	// Grass
	[3] =  {0, 0, 2,   2,   2,   2,   2,   2  },	// Stone
	[4] =  {0, 0, 17,  17,  17,  17,  17,  17 },	// Cobblestone
	[5] =  {0, 0, 5,   5,   5,   5,   5,   5  },	// Planks
	[6] =  {2, 1, 16,  16,  16,  16,  16,  16 },	// Sapling
	[7] =  {0, 0, 18,  18,  18,  18,  18,  18 },	// Bedrock
	[8] =  {0, 1, 208, 208, 208, 208, 208, 208},	// Flowing water
	[9] =  {0, 1, 208, 208, 208, 208, 208, 208},	// Stationary water
	[10] = {0, 0, 224, 224, 224, 224, 224, 224},	// Flowing lava
	[11] = {0, 0, 224, 224, 224, 224, 224, 224},	// Stationary lava
	[12] = {0, 0, 19,  19,  19,  19,  19,  19 },	// Sand
	[13] = {0, 0, 20,  20,  20,  20,  20,  20 },	// Gravel
	[14] = {0, 0, 33,  33,  33,  33,  33,  33 },	// Gold Ore
	[15] = {0, 0, 34,  34,  34,  34,  34,  34 },	// Iron Ore
	[16] = {0, 0, 35,  35,  35,  35,  35,  35 },	// Coal Ore
	[17] = {0, 0, 21,  21,  21,  21,  22,  22 },	// Wood
	[18] = {0, 1, 53,  53,  53,  53,  53,  53 },	// Leaves
	[19] = {0, 0, 49,  49,  49,  49,  49,  49 },	// Sponge
	[20] = {0, 1, 50,  50,  50,  50,  50,  50 },	// Glass
	[44] = {1, 1, 6,   6,   6,   6,   7,   7  },	// Slab
	[89] = {0, 0, 106, 106, 106, 106, 106, 106},	// Glowstone
};

Chunk*** chunks = NULL;
Entity global_entities[MAX_ENTITIES_PER_CHUNK];
unsigned int model_uniform_location = -1;
GLFWwindow* window = NULL;

int initialize() {
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	/*glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);*/

	window = glfwCreateWindow(screen_width, screen_height, "Minecraft Clone", NULL, NULL);
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
	framebuffer_size_callback(window, screen_width, screen_height);

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
	profiler_create("World");
	#endif

	// Textures
	block_textures = loadTexture("./assets/atlas.png");
	ui_textures = loadTexture("./assets/gui.png");

	// Initialization
	load_shaders();
	init_framebuffer();
	init_fullscreen_quad();
	init_ui();
	init_highlight();
	init_gl_buffers();
	model_uniform_location = glGetUniformLocation(shaderProgram, "model");
	chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);
	init_chunk_loader();
	init_chunk_processor();

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_MULTISAMPLE);

	// Initialize player
	Entity player = {
		.x = 0.0f,
		.y = 100.0f,
		.z = 0.0f,
		.yaw = 90.0f,
		.pitch = 0.0f,
		.width = 0.9375f,
		.height = 1.875f,
		.eye_level = 1.625f,
		.speed = 10
	};
	global_entities[0] = player;

	/*snprintf(game_dir, sizeof(game_dir), "%s/.ccraft", getenv("HOME"));
	mkdir(game_dir, 0766);
	char saves_dir[255];
	snprintf(saves_dir, sizeof(saves_dir), "%s/saves", game_dir);
	mkdir(saves_dir, 0766);

	char chunks_dir[255];
	snprintf(chunks_dir, sizeof(chunks_dir), "%s/chunks", saves_dir);
	mkdir(chunks_dir, 0766);*/

	return 0;
}

void run() {
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
	destroy_chunk_loader();

	for(uint8_t cx = 0; cx < RENDER_DISTANCE; cx++) {
		for(uint8_t cy = 0; cy < WORLD_HEIGHT; cy++) {
			for(uint8_t cz = 0; cz < RENDER_DISTANCE; cz++) {
				Chunk* chunk = &chunks[cx][cy][cz];
				unload_chunk(chunk);
			}
		}
	}

	free_chunks(chunks, RENDER_DISTANCE, WORLD_HEIGHT);
	glDeleteProgram(shaderProgram);
	glfwDestroyWindow(window);
	glfwTerminate();
}