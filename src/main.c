#include "main.h"

int main(int argc, char* argv[]) {
	// Initialize player
	Player player = {
		.x = -16.0f,
		.y = 64.0f,
		.z = -16.0f,
		.yaw = 135.0f,
		.pitch = 20.0f,
		.speed = 20
	};

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

	// Load VAO function pointers
	gl_gen_vertex_arrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays");
	gl_bind_vertex_array = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray");
	gl_delete_vertex_arrays = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays");
	gl_vertex_attrib_pointer = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
	gl_enable_vertex_attrib_array = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");

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

	while (main_loop(&player) == 1);

	cleanup();
	return 0;
}

int main_loop(Player* player) {
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
			}
			// Mouse movement
			if (event.type == SDL_MOUSEMOTION) {
				player->yaw += event.motion.xrel * 0.2f;
				player->pitch += event.motion.yrel * 0.2f;

				// Clamp pitch to prevent camera flipping
				player->pitch = fmaxf(-89.0f, fminf(89.0f, player->pitch));
			}
		}

		// Get keyboard state
		const Uint8* keyboard = SDL_GetKeyboardState(NULL);
		process_keyboard_movement(keyboard, player, deltaTime);

		// Check for R key to mark all chunks for update
		static int rKeyWasPressed = 0;
		if (keyboard[SDL_SCANCODE_R] && !rKeyWasPressed) {
			printf("Re-rendering all chunks\n");
			for(int cx = 0; cx < CHUNKS_X; cx++) {
				for(int cy = 0; cy < CHUNKS_Y; cy++) {
					for(int cz = 0; cz < CHUNKS_Z; cz++) {
						if (chunks[cx][cy][cz].vbo) {
							chunks[cx][cy][cz].needs_update = true;
						}
					}
				}
			}
		}
		rKeyWasPressed = keyboard[SDL_SCANCODE_R];

		// Check player position and generate new chunks if needed - once every 3 seconds
		static Uint32 lastChunkCheck = 0;
		if (currentTime - lastChunkCheck >= 3000) {
			int center_cx = (int)floorf(player->x / (CHUNK_SIZE * 1.0f));
			int center_cy = (int)floorf(player->y / (CHUNK_SIZE * 1.0f));
			int center_cz = (int)floorf(player->z / (CHUNK_SIZE * 1.0f));

			// First, unload chunks that are too far from the player
			for(int cx = 0; cx < CHUNKS_X; cx++) {
				for(int cy = 0; cy < CHUNKS_Y; cy++) {
					for(int cz = 0; cz < CHUNKS_Z; cz++) {
						if (chunks[cx][cy][cz].vbo) {
							// Check if chunk is outside the radius
							if (abs(cx - center_cx) > chunk_radius ||
								abs(cy - center_cy) > chunk_radius ||
								abs(cz - center_cz) > chunk_radius) {
								unload_chunk(&chunks[cx][cy][cz]);
							}
						}
					}
				}
			}

			// Loop through NxNxN chunk area around player
			for(int dx = -chunk_radius; dx <= chunk_radius; dx++) {
				for(int dy = -chunk_radius; dy <= chunk_radius; dy++) {
					for(int dz = -chunk_radius; dz <= chunk_radius; dz++) {
						int cx = center_cx + dx;
						int cy = center_cy + dy;
						int cz = center_cz + dz;

						// Ensure chunk coordinates are within bounds without wrapping
						cx = cx < 0 ? 0 : (cx >= CHUNKS_X ? CHUNKS_X - 1 : cx);
						cy = cy < 0 ? 0 : (cy >= CHUNKS_Y ? CHUNKS_Y - 1 : cy);
						cz = cz < 0 ? 0 : (cz >= CHUNKS_Z ? CHUNKS_Z - 1 : cz);

						// Load chunk if it's not already loaded
						if (!chunks[cx][cy][cz].vbo) {
							load_chunk(cx, cy, cz);
						}
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

		// Render all chunks
		for(int cx = 0; cx < CHUNKS_X; cx++) {
			for(int cy = 0; cy < CHUNKS_Y; cy++) {
				for(int cz = 0; cz < CHUNKS_Z; cz++) {
					if (chunks[cx][cy][cz].needs_update) {
						bake_chunk(&chunks[cx][cy][cz]);

						// TODO: This causes lag, Should probably be done in a separate thread
						// Re-Render neighboring chunks
						if (cz > 0 && chunks[cx][cy][cz-1].vbo != 0)
							bake_chunk(&chunks[cx][cy][cz-1]);
						if (cz < CHUNKS_Z-1 && chunks[cx][cy][cz+1].vbo != 0)
							bake_chunk(&chunks[cx][cy][cz+1]);
							
						if (cx > 0 && chunks[cx-1][cy][cz].vbo != 0)
							bake_chunk(&chunks[cx-1][cy][cz]);
						if (cx < CHUNKS_X-1 && chunks[cx+1][cy][cz].vbo != 0)
							bake_chunk(&chunks[cx+1][cy][cz]);

						if (cy > 0 && chunks[cx][cy-1][cz].vbo != 0)
							bake_chunk(&chunks[cx][cy-1][cz]);
						if (cy < CHUNKS_Y-1 && chunks[cx][cy+1][cz].vbo != 0)
							bake_chunk(&chunks[cx][cy+1][cz]);
					}

					glPushMatrix();
					glTranslatef(chunks[cx][cy][cz].x, chunks[cx][cy][cz].y, chunks[cx][cy][cz].z);

					gl_bind_vertex_array(chunks[cx][cy][cz].vao);
					glDrawArrays(GL_QUADS, 0, chunks[cx][cy][cz].vertex_count);
					gl_bind_vertex_array(0);
					
					glPopMatrix();
				}
			}
		}

		// Draw HUD with average FPS
		draw_hud(averageFps, player);

		// Swap buffers
		SDL_GL_SwapWindow(window);
	return 1;
}

void change_resolution() {
	float aspect = (float)screen_width / (float)screen_height;
	float fovRad = fovRad = (fov * M_PI) / 180.0f;
	float tanHalf = tanf(fovRad / 2.0f);
	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-near * aspect * tanHalf, near * aspect * tanHalf, -near * tanHalf, near * tanHalf, near, far);
	glMatrixMode(GL_MODELVIEW);
}

void cleanup() {
	for(int cx = 0; cx < CHUNKS_X; cx++) {
		for(int cy = 0; cy < CHUNKS_Y; cy++) {
			for(int cz = 0; cz < CHUNKS_Z; cz++) {
				unload_chunk(&chunks[cx][cy][cz]);
			}
		}
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}