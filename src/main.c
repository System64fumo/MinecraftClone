#include "main.h"

int main(int argc, char* argv[]) {
	SDL_Window* window = NULL;
	SDL_GLContext glContext;
	bool quit = false;
	SDL_Event event;

	// Initialize player
	Player player = {
		.x = 0.0f,
		.y = 0.0f,
		.z = -5.0f,
		.yaw = 0.0f,
		.pitch = 0.0f,
		.speed = 5.0f
	};

	// Initialize chunks array
	#define CHUNKS_X 4
	#define CHUNKS_Y 4
	#define CHUNKS_Z 4
	Chunk chunks[CHUNKS_X][CHUNKS_Y][CHUNKS_Z];

	// Initialize all chunks
	for(int cx = 0; cx < CHUNKS_X; cx++) {
		for(int cy = 0; cy < CHUNKS_Y; cy++) {
			for(int cz = 0; cz < CHUNKS_Z; cz++) {
				chunks[cx][cy][cz] = (Chunk){0};
				chunks[cx][cy][cz].x = cx * CHUNK_SIZE / 2;
				chunks[cx][cy][cz].y = cy * CHUNK_SIZE / 2;
				chunks[cx][cy][cz].z = cz * CHUNK_SIZE / 2;
				chunks[cx][cy][cz].needs_update = true;
				chunks[cx][cy][cz].vbo = 0;
				chunks[cx][cy][cz].color_vbo = 0;
				chunks[cx][cy][cz].vertices = NULL;
				chunks[cx][cy][cz].colors = NULL;

				generate_chunk_terrain(&chunks[cx][cy][cz], cx, cy, cz);
			}
		}
	}

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	// Initialize SDL_ttf
	if (TTF_Init() == -1) {
		printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
		return 1;
	}

	glutInit(&argc, argv);

	// Set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

	// Create window
	window = SDL_CreateWindow("Minecraft Clone", 
							SDL_WINDOWPOS_UNDEFINED, 
							SDL_WINDOWPOS_UNDEFINED, 
							screen_width, screen_height, 
							SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	if (window == NULL) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	// Create OpenGL context
	glContext = SDL_GL_CreateContext(window);
	if (glContext == NULL) {
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	// Load VBO function pointers
	gl_gen_buffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
	gl_bind_buffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
	gl_buffer_data = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
	gl_delete_buffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");

	// Set up OpenGL viewport and projection
	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0, 1.0, -0.5625, 0.5625, 1.5, 100.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Enable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	// Hide and capture mouse
	SDL_SetRelativeMouseMode(SDL_TRUE);

	Uint32 lastTime = SDL_GetTicks();
	float deltaTime = 0.0f;
	
	// FPS tracking variables
	#define FPS_UPDATE_INTERVAL 500  // Update FPS every 500ms
	#define FPS_HISTORY_SIZE 10      // Store 10 samples for averaging
	float fpsHistory[FPS_HISTORY_SIZE] = {0};
	int fpsIndex = 0;
	float averageFps = 0.0f;
	Uint32 lastFpsUpdate = SDL_GetTicks();
	int frameCount = 0;

	// Main game loop
	while (!quit) {
		// Calculate delta time
		Uint32 currentTime = SDL_GetTicks();
		deltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;
		
		frameCount++;

		// Update FPS calculation every FPS_UPDATE_INTERVAL milliseconds
		if (currentTime - lastFpsUpdate >= FPS_UPDATE_INTERVAL) {
			float currentFps = frameCount / ((currentTime - lastFpsUpdate) / 1000.0f);
			fpsHistory[fpsIndex] = currentFps;
			fpsIndex = (fpsIndex + 1) % FPS_HISTORY_SIZE;

			// Calculate average FPS
			averageFps = 0.0f;
			for (int i = 0; i < FPS_HISTORY_SIZE; i++) {
				averageFps += fpsHistory[i];
			}
			averageFps /= FPS_HISTORY_SIZE;

			frameCount = 0;
			lastFpsUpdate = currentTime;
		}

		// Handle events
		while (SDL_PollEvent(&event) != 0) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}

			// Mouse movement
			if (event.type == SDL_MOUSEMOTION) {
				player.yaw += event.motion.xrel * 0.2f;
				player.pitch += event.motion.yrel * 0.2f;

				// Clamp pitch to prevent camera flipping
				player.pitch = fmaxf(-89.0f, fminf(89.0f, player.pitch));
			}
		}

		// Get keyboard state
		process_keyboard_movement(SDL_GetKeyboardState(NULL), &player, deltaTime);

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.5f, 0.6f, 1.0f, 1.0f);

		// Reset modelview matrix
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Apply camera rotation
		glRotatef(player.pitch, 1.0f, 0.0f, 0.0f);
		glRotatef(player.yaw, 0.0f, 1.0f, 0.0f);

		// Apply camera translation
		glTranslatef(player.x, player.y, player.z);

		// Render all chunks
		for(int cx = 0; cx < CHUNKS_X; cx++) {
			for(int cy = 0; cy < CHUNKS_Y; cy++) {
				for(int cz = 0; cz < CHUNKS_Z; cz++) {
					if (chunks[cx][cy][cz].needs_update) {
						bake_chunk(&chunks[cx][cy][cz]);
					}

					glPushMatrix();
					glTranslatef(chunks[cx][cy][cz].x, chunks[cx][cy][cz].y, chunks[cx][cy][cz].z);

					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_COLOR_ARRAY);

					gl_bind_buffer(GL_ARRAY_BUFFER, chunks[cx][cy][cz].vbo);
					glVertexPointer(3, GL_FLOAT, 0, 0);

					gl_bind_buffer(GL_ARRAY_BUFFER, chunks[cx][cy][cz].color_vbo);
					glColorPointer(3, GL_FLOAT, 0, 0);

					glDrawArrays(GL_QUADS, 0, chunks[cx][cy][cz].vertex_count);
					
					glDisableClientState(GL_COLOR_ARRAY);
					glDisableClientState(GL_VERTEX_ARRAY);
					
					glPopMatrix();
				}
			}
		}

		// Draw HUD with average FPS
		draw_hud(averageFps);

		// Swap buffers
		SDL_GL_SwapWindow(window);
	}

	// Cleanup
	for(int cx = 0; cx < CHUNKS_X; cx++) {
		for(int cy = 0; cy < CHUNKS_Y; cy++) {
			for(int cz = 0; cz < CHUNKS_Z; cz++) {
				if (chunks[cx][cy][cz].vertices) {
					free(chunks[cx][cy][cz].vertices);
				}
				if (chunks[cx][cy][cz].colors) {
					free(chunks[cx][cy][cz].colors);
				}
				if (chunks[cx][cy][cz].vbo) {
					gl_delete_buffers(1, &chunks[cx][cy][cz].vbo);
				}
				if (chunks[cx][cy][cz].color_vbo) {
					gl_delete_buffers(1, &chunks[cx][cy][cz].color_vbo);
				}
			}
		}
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
