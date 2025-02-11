#include "main.h"

int screen_width = 1280;
int screen_height = 720;

float fov = 70.0f;
float near = 0.1f;
float far = 300.0f;

float screen_center_x;
float screen_center_y;

uint32_t lastTime;
uint32_t lastFpsUpdate;
float deltaTime;
int frameCount;
float averageFps;
int fpsIndex;
float fpsHistory[FPS_HISTORY_SIZE];

GLuint buffer;

Chunk chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];
Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDERR_DISTANCE * CHUNK_SIZE];

int main(int argc, char* argv[]) {
	// Initialize GLUT
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(screen_width, screen_height);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Minecraft Clone");
	glewInit();

	// Set up OpenGL viewport and projection
	reshape(1280, 720);

	// Enable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	// Initialize mouse capture
	glutSetCursor(GLUT_CURSOR_NONE);
	glutWarpPointer(screen_width/2, screen_height/2);

	// Get current time
	lastTime = glutGet(GLUT_ELAPSED_TIME);
	
	// Initialize FPS tracking
	memset(fpsHistory, 0, sizeof(fpsHistory));
	fpsIndex = 0;
	averageFps = 0.0f;
	lastFpsUpdate = lastTime;
	frameCount = 0;

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

	// Set up GLUT callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutSpecialUpFunc(specialUp);
	glutKeyboardFunc(keyboard);
	glutKeyboardUpFunc(keyboardUp);
	glutPassiveMotionFunc(mouse);
	glutIdleFunc(idle);

	// Start the main loop
	glutMainLoop();

	cleanup();
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
