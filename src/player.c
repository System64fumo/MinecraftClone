#include "main.h"

int lastX;
int lastY;
int firstMouse = 1;
unsigned char keys[256] = {0};

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.5f, 0.6f, 1.0f, 1.0f);
	glLoadIdentity();

	// Apply camera transformations based on player position
	glRotatef(global_entities[0].pitch, 1.0f, 0.0f, 0.0f);
	glRotatef(global_entities[0].yaw, 0.0f, 1.0f, 0.0f);
	glTranslatef(-global_entities[0].x, -global_entities[0].y, -global_entities[0].z);

	// Render world
	render_chunks();

	// Draw HUD with average FPS
	#ifdef DEBUG
	profiler_start(PROFILER_ID_HUD);
	#endif
	draw_hud(averageFps, &global_entities[0]);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_HUD);
	#endif

	glutSwapBuffers();
}

void reshape(int w, int h) {
	screen_width = w;
	screen_height = h;

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

void keyboard(unsigned char key, int x, int y) {
	keys[key] = 1;
	if (key == 27) {  // ESC key
		exit(0);
	}
	if (keys['r']) {
		#ifdef DEBUG
		profiler_start(PROFILER_ID_WORLD_GEN);
		#endif
		int center_cx = fmaxf(0, fminf(WORLD_SIZE, (int)floorf(global_entities[0].x / (CHUNK_SIZE * 1.0f)) - (RENDERR_DISTANCE / 2)));
		int center_cy = fmaxf(0, fminf(WORLD_HEIGHT, -((int)floorf(global_entities[0].y / (CHUNK_SIZE * 1.0f)) - WORLD_HEIGHT)));
		int center_cz = fmaxf(0, fminf(WORLD_SIZE, (int)floorf(global_entities[0].z / (CHUNK_SIZE * 1.0f)) - (RENDERR_DISTANCE / 2)));

		for(int cx = 0; cx < RENDERR_DISTANCE; cx++) {
			for(int cy = 0; cy < WORLD_HEIGHT; cy++) {
				for(int cz = 0; cz < RENDERR_DISTANCE; cz++) {
					if (chunks[cx][cy][cz].vbo) {
						unload_chunk(&chunks[cx][cy][cz]);
					}
				}
			}
		}

		for(int x = 0; x < RENDERR_DISTANCE; x++) {
			for(int y = 0; y < WORLD_HEIGHT; y++) {
				for(int z = 0; z < RENDERR_DISTANCE; z++) {
					load_chunk(x, y, z, x + center_cx, y, z + center_cz);
				}
			}
		}
		#ifdef DEBUG
		profiler_stop(PROFILER_ID_WORLD_GEN);
		#endif
	}
}

void keyboardUp(unsigned char key, int x, int y) {
	keys[key] = 0;
}

void special(int key, int x, int y) {
	if (key == 112) {
		keys[112] = 1;
	}
}

void specialUp(int key, int x, int y) {
	if (key == 112) {
		keys[112] = 0;
	}
}

void idle() {
	// Calculate delta time
	uint32_t currentTime = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = (currentTime - lastTime) / 1000.0f;
	lastTime = currentTime;
	
	// Handle continuous movement based on key states
	float moveSpeed = global_entities[0].speed * deltaTime;
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);

	if (keys['w']) {
		global_entities[0].x -= sinf(-yaw) * moveSpeed;
		global_entities[0].z -= cosf(-yaw) * moveSpeed;
	}
	else if (keys['s']) {
		global_entities[0].x += sinf(-yaw) * moveSpeed;
		global_entities[0].z += cosf(-yaw) * moveSpeed;
	}
	if (keys['a']) {
		global_entities[0].x -= cosf(yaw) * moveSpeed;
		global_entities[0].z -= sinf(yaw) * moveSpeed;
	}
	else if (keys['d']) {
		global_entities[0].x += cosf(yaw) * moveSpeed;
		global_entities[0].z += sinf(yaw) * moveSpeed;
	}
	if (keys[' ']) {
		global_entities[0].y += moveSpeed;
	}
	else if (keys[112]) {  // Shift key
		global_entities[0].y -= moveSpeed;
	}

	frameCount++;

	// Update FPS calculation
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

		#ifdef DEBUG
		profiler_print_all();
		#endif
	}



	glutPostRedisplay();
	
	// Ensure mouse stays centered
	glutWarpPointer(screen_width/2, screen_height/2);
}

void mouse(int x, int y) {
	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = 0;
		return;
	}

	float sensitivity = -0.1f;
	float deltaX = (x - screen_width/2) * sensitivity;
	float deltaY = (y - screen_height/2) * sensitivity;

	global_entities[0].yaw -= deltaX;
	global_entities[0].pitch -= deltaY;

	// Clamp pitch
	if (global_entities[0].pitch > 89.0f) {
		global_entities[0].pitch = 89.0f;
	}
	if (global_entities[0].pitch < -89.0f) {
		global_entities[0].pitch = -89.0f;
	}

	// Wrap yaw
	if (global_entities[0].yaw >= 360.0f) {
		global_entities[0].yaw -= 360.0f;
	}
	if (global_entities[0].yaw < 0.0f) {
		global_entities[0].yaw += 360.0f;
	}
}