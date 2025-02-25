#include "main.h"
#include <stdlib.h>
#include <sys/stat.h>

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

double currentFrame = 0.0f;
double lastFrame = 0.0f;
double deltaTime = 0.0f;

float model[16], view[16], projection[16];
unsigned int block_textures, ui_textures;

// Type, Translucent, Face textures
// t,    t,           f,f,f,f,f,f
uint8_t block_data[MAX_BLOCK_TYPES][8] = {
	[0] =  {0, 1, 0,   0,   0,   0,   0,   0},		// Air
	[1] =  {0, 0, 3,   3,   3,   3,   3,   3},		// Dirt
	[2] =  {0, 0, 4,   4,   4,   4,   3,   1},		// Grass
	[3] =  {0, 0, 2,   2,   2,   2,   2,   2},		// Stone
	[4] =  {0, 0, 17,  17,  17,  17,  17,  17},		// Cobblestone
	[5] =  {0, 0, 5,   5,   5,   5,   5,   5},		// Planks
	[6] =  {2, 1, 16,  16,  16,  16,  16,  16},		// Sapling
	[7] =  {0, 0, 18,  18,  18,  18,  18,  18},		// Bedrock
	[8] =  {0, 1, 208, 208, 208, 208, 208, 208},	// Flowing water
	[9] =  {0, 1, 208, 208, 208, 208, 208, 208},	// Stationary water
	[10] = {0, 0, 224, 224, 224, 224, 224, 224},	// Flowing lava
	[11] = {0, 0, 224, 224, 224, 224, 224, 224},	// Stationary lava
	[12] = {0, 0, 19,  19,  19,  19,  19,  19},		// Sand
	[13] = {0, 0, 20,  20,  20,  20,  20,  20},		// Gravel
	[14] = {0, 0, 33,  33,  33,  33,  33,  33},		// Gold Ore
	[15] = {0, 0, 34,  34,  34,  34,  34,  34},		// Iron Ore
	[16] = {0, 0, 35,  35,  35,  35,  35,  35},		// Coal Ore
	[17] = {0, 0, 21,  21,  21,  21,  22,  22},		// Wood
	[18] = {0, 1, 53,  53,  53,  53,  53,  53},		// Leaves
	[19] = {0, 0, 49,  49,  49,  49,  49,  49},		// Sponge
	[20] = {0, 1, 50,  50,  50,  50,  50,  50},		// Glass
	[44] = {1, 1, 6,   6,   6,   6,   7,   7},		// Slab
};

Chunk*** chunks = NULL;
Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDER_DISTANCE * CHUNK_SIZE];

int main() {
	snprintf(game_dir, sizeof(game_dir), "%s/.ccraft", getenv("HOME"));
	mkdir(game_dir, 0766);
	char saves_dir[255];
	snprintf(saves_dir, sizeof(saves_dir), "%s/saves", game_dir);
	mkdir(saves_dir, 0766);

	char chunks_dir[255];
	snprintf(chunks_dir, sizeof(chunks_dir), "%s/chunks", saves_dir);
	mkdir(chunks_dir, 0766);

	// Initialize GLFW
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	// Configure GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//glfwWindowHint(GLFW_SAMPLES, 8);

	// Create window
	GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Minecraft Clone", NULL, NULL);
	if (!window) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, key_callback);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Debugging
	#ifdef DEBUG
	printf("OpenGL Vendor: %s\n", (const char*)glGetString(GL_VENDOR));
	printf("OpenGL Renderer: %s\n", (const char*)glGetString(GL_RENDERER));
	printf("OpenGL Version: %s\n", (const char*)glGetString(GL_VERSION));

	profiler_init();
	profiler_create("Shaders");
	profiler_create("Baking");
	profiler_create("Merge");
	profiler_create("Rendering");
	profiler_create("World Gen");
	#endif

	// Textures
	block_textures = loadTexture("./atlas.png");
	ui_textures = loadTexture("./gui.png");

	// Initialization
	load_shaders();
	initFramebuffer();
	initQuad();
	init_ui();
	chunks = allocate_chunks(RENDER_DISTANCE, WORLD_HEIGHT);

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
		.y = 74.0f,
		.z = 0.0f,
		.yaw = 90.0f,
		.pitch = 0.0f,
		.speed = 20
	};
	global_entities[0] = player;

	load_around_entity(&player);

	// Main render loop
	while (!glfwWindowShouldClose(window)) {
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		do_time_stuff();

		// Process input
		processInput(window);

		renderSceneToFramebuffer();
		renderFramebufferToScreen();

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void cleanup() {
	cleanupFramebuffer();
	cleanup_ui();
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
}
