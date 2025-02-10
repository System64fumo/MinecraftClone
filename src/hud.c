#include "main.h"

void draw_text(const char* text, int length, int x, int y) {
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, screen_width, screen_height, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	
	glRasterPos2i(x, y);
	for (int i = 0; i < length; i++) {
		glutBitmapCharacter(GLUT_BITMAP_9_BY_15, text[i]);
	}
	
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void draw_hud(float fps, Player* player) {
	static char debug_text[128];
	
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, screen_width, screen_height, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);
	
	// Draw crosshair
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	
	// Raycast to find block player is looking at
	/*int block_x, block_y, block_z;
	Chunk* chunk;
	float hit_distance;
	bool hit = raycast(player, 15.0f, &block_x, &block_y, &block_z, &chunk, &hit_distance);*/

	// Draw crosshair lines
	glBegin(GL_LINES);
	glVertex2f(screen_center_x - 10, screen_center_y);
	glVertex2f(screen_center_x + 10, screen_center_y);
	glVertex2f(screen_center_x, screen_center_y - 10);
	glVertex2f(screen_center_x, screen_center_y + 10);
	glEnd();
	
	glDisable(GL_COLOR_LOGIC_OP);
	
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();

	#ifdef DEBUG
		const char* direction = 
			(player->yaw < 45 || player->yaw >= 315) ? "North" :
			(player->yaw < 135) ? "East" :
			(player->yaw < 225) ? "South" : "West";
			
		const char* pitch = 
			(player->pitch > 45) ? "Down" :
			(player->pitch < -45) ? "Up" : "";
			
		snprintf(debug_text, sizeof(debug_text), 
			"FPS: %.1f, X: %.1f, Y: %.1f Z: %.1f, Direction: %s %s", 
			fps, -(((WORLD_SIZE * CHUNK_SIZE) / 2) - player->x), player->y, -(((WORLD_SIZE * CHUNK_SIZE) / 2) - player->z), direction, pitch);
	#else
		snprintf(debug_text, sizeof(debug_text), "FPS: %.1f", fps);
	#endif

	draw_text(debug_text, strlen(debug_text), 10, 20);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	/*if (hit) {
		draw_block_highlight((chunk->ci_x * CHUNK_SIZE) + block_x + 1, (chunk->ci_y * CHUNK_SIZE) + block_y + 1, (chunk->ci_z * CHUNK_SIZE) + block_z + 1);
	}*/
}