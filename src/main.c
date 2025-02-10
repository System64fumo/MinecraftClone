#include "main.h"

int main(int argc, char* argv[]) {
	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
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

	if (!window) {
		printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
		return 1;
	}

	// Create OpenGL context
	glContext = SDL_GL_CreateContext(window);
	if (!glContext) {
		printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

	// Load VBO function pointers
	gl_gen_buffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
	gl_bind_buffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
	gl_buffer_data = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
	gl_delete_buffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");

	// Set up OpenGL viewport and projection
	change_resolution();

	// Enable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	SDL_SetRelativeMouseMode(SDL_TRUE);

	lastTime = SDL_GetTicks();
	
	// Initialize FPS tracking
	memset(fpsHistory, 0, sizeof(fpsHistory));
	fpsIndex = 0;
	averageFps = 0.0f;
	lastFpsUpdate = lastTime;
	frameCount = 0;

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

	int center_cx = fmaxf(0, fminf(WORLD_SIZE, (int)floorf(player.x / (CHUNK_SIZE * 1.0f)) - (render_distance / 2)));
	int center_cy = fmaxf(0, fminf(WORLD_HEIGHT, (int)floorf(player.y / (CHUNK_SIZE * 1.0f))));
	int center_cz = fmaxf(0, fminf(WORLD_SIZE, (int)floorf(player.z / (CHUNK_SIZE * 1.0f)) - (render_distance / 2)));

	int x = (render_distance / 2);
	int y = (render_distance / 2);
	int z = (render_distance / 2);

	// Load spawn chunk
	load_chunk(x, 2, y, center_cx + x, 2, center_cz + y);

	// Run the game
	while (main_loop(&player) == 1);

	cleanup();
	return 0;
}

int main_loop(Entity* player) {
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
				return 0;
			}

			// Handle window resize event
			if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_RESIZED) {
				screen_width = event.window.data1;
				screen_height = event.window.data2;
				change_resolution();
			}

			// Mouse movement
			if (event.type == SDL_MOUSEMOTION) {
				player->yaw += event.motion.xrel * 0.2f;
				if (player->yaw >= 360.0f) {
					player->yaw -= 360.0f;
				}
				else if (player->yaw < 0.0f) {
					player->yaw += 360.0f;
				}
				player->pitch += event.motion.yrel * 0.2f;

				// Clamp pitch to prevent camera flipping
				player->pitch = fmaxf(-89.0f, fminf(89.0f, player->pitch));
			}
		}

		int center_cx = fmaxf(0, fminf(WORLD_SIZE, (int)floorf(player->x / (CHUNK_SIZE * 1.0f)) - (render_distance / 2)));
		int center_cy = fmaxf(0, fminf(WORLD_HEIGHT, (int)floorf(player->y / (CHUNK_SIZE * 1.0f))));
		int center_cz = fmaxf(0, fminf(WORLD_SIZE, (int)floorf(player->z / (CHUNK_SIZE * 1.0f)) - (render_distance / 2)));

		// Get keyboard state
		const Uint8* keyboard = SDL_GetKeyboardState(NULL);
		process_keyboard_movement(keyboard, player, deltaTime);

		// Check for R key to mark all chunks for update
		static int rKeyWasPressed = 0;
		if (keyboard[SDL_SCANCODE_R] && !rKeyWasPressed) {
			printf("Re-rendering all chunks\n");
			for(int cx = 0; cx < render_distance; cx++) {
				for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
					for(int cz = 0; cz < render_distance; cz++) {
						if (chunks[cx][cy][cz].vbo) {
							chunks[cx][cy][cz].needs_update = true;
						}
					}
				}
			}
		}
		rKeyWasPressed = keyboard[SDL_SCANCODE_R];

		static Uint32 lastChunkCheck = 0;
		if (currentTime - lastChunkCheck >= 3000) {
			for(int cx = 0; cx < render_distance; cx++) {
				for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
					for(int cz = 0; cz < render_distance; cz++) {
						if (chunks[cx][cy][cz].vbo) {
							unload_chunk(&chunks[cx][cy][cz]);
						}
					}
				}
			}

			for(int x = 0; x < render_distance; x++) {
				for(int y = 0; y < WORLD_HEIGHT; y++) {
					for(int z = 0; z < render_distance; z++) {
						load_chunk(x, y, z, x + center_cx, y, z + center_cz);
					}
				}
			}
			lastChunkCheck = currentTime;
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.5f, 0.6f, 1.0f, 1.0f);

		// Reset modelview matrix
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		// Apply player position and rotation
		glRotatef(player->pitch, 1.0f, 0.0f, 0.0f);
		glRotatef(player->yaw, 0.0f, 1.0f, 0.0f);
		glTranslatef(-player->x, -player->y, -player->z);

		// Render chunks
		render_chunks();

		// Draw HUD with average FPS
		draw_hud(averageFps, player);

		// Swap buffers
		SDL_GL_SwapWindow(window);
	return 1;
}

void change_resolution() {
	float aspect = (float)screen_width / (float)screen_height;
	float fovRad = (fov * M_PI) / 180.0f;
	float tanHalf = tanf(fovRad / 2.0f);
	screen_center_x = screen_width / 2.0f;
	screen_center_y = screen_height / 2.0f;

	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-near * aspect * tanHalf, near * aspect * tanHalf, -near * tanHalf, near * tanHalf, near, far);
	glMatrixMode(GL_MODELVIEW);
}

void cleanup() {
	for(int cx = 0; cx < render_distance; cx++) {
		for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
			for(int cz = 0; cz < render_distance; cz++) {
				unload_chunk(&chunks[cx][cy][cz]);
			}
		}
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}