#include "main.h"
#include <stdio.h>

int screen_width = 1280;
int screen_height = 720;

float fov = 70.0f;
float near = 0.1f;
float far = 300.0f;

float screen_center_x = 0.0f;
float screen_center_y = 0.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

short fps_average = 0;

GLuint buffer;

Chunk chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];
Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDERR_DISTANCE * CHUNK_SIZE];

int main(int argc, char* argv[]) {
	// Initialize GLFW
	if (!glfwInit()) {
		return -1;
	}

	// Configure GLFW
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Create window
	GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Minecraft Clone", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	glewInit();
	glutInit(&argc, argv);

	// Enable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	// Initialize mouse capture
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, screen_center_x, screen_center_y);

	// Debugging
	#ifdef DEBUG
	profiler_init();
	profiler_create("HUD");
	profiler_create("Baking");
	profiler_create("Rendering");
	profiler_create("World Gen");
	#endif

	// Initialize player
	Entity player = {
		.x = (WORLD_SIZE * CHUNK_SIZE) / 2,
		.y = 35.0f,
		.z = (WORLD_SIZE * CHUNK_SIZE) / 2,
		.yaw = 0.0f,
		.pitch = 0.0f,
		.speed = 20
	};
	global_entities[0] = player;

	// Load spawn chunk
	load_around_entity(&global_entities[0]);

	// Set up GLFW callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		// Calculate FPS and maintain rolling average
		static float fpsHistory[180] = {0};  // 180 frames = ~3 seconds at 60fps
		static int fpsIndex = 0;
		static float fpsSum = 0;
		
		fpsSum -= fpsHistory[fpsIndex];
		fpsHistory[fpsIndex] = 1.0f / deltaTime;
		fpsSum += fpsHistory[fpsIndex];
		fpsIndex = (fpsIndex + 1) % 180;
		
		fps_average = (int)(fpsSum / 180);

		static float lastPrintTime = 0.0f;
		if (currentFrame - lastPrintTime >= 1.0f) {
			profiler_print_all();
			lastPrintTime = currentFrame;
		}

		processInput(window);
		display();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void cleanup() {
	for(int cx = 0; cx < RENDERR_DISTANCE; cx++) {
		for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
			for(int cz = 0; cz < RENDERR_DISTANCE; cz++) {
				unload_chunk(&chunks[cx][cy][cz]);
			}
		}
	}

	#ifdef DEBUG
	profiler_cleanup();
	#endif
}
