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
	
	const float center_x = screen_width / 2;
	const float center_y = screen_height / 2;
	
	// Raycast to find block player is looking at
	int block_x, block_y, block_z;
	Chunk* chunk;
	float hit_distance;
	bool hit = raycast(player, 15.0f, &block_x, &block_y, &block_z, &chunk, &hit_distance);

	// Draw crosshair lines
	glBegin(GL_LINES);
	glVertex2f(center_x - 10, center_y);
	glVertex2f(center_x + 10, center_y);
	glVertex2f(center_x, center_y - 10);
	glVertex2f(center_x, center_y + 10);
	glEnd();
	
	glDisable(GL_COLOR_LOGIC_OP);
	
	glEnable(GL_TEXTURE_2D);
	glPopMatrix();

	if (DEBUG) {
		if (hit) {
			snprintf(debug_text, sizeof(debug_text), 
				"FPS: %.1f, X: %.1f, Y: %.1f Z: %.1f, Yaw: %.1f, Pitch: %.1f, Looking at block: %d,%d,%d (ID: %d)", 
				fps, player->x, player->y, player->z, player->yaw, player->pitch,
				block_x, block_y, block_z, chunk->blocks[block_x][block_y][block_z].id);
		} else {
			snprintf(debug_text, sizeof(debug_text), 
				"FPS: %.1f, X: %.1f, Y: %.1f Z: %.1f, Yaw: %.1f, Pitch: %.1f, No block in range", 
				fps, player->x, player->y, player->z, player->yaw, player->pitch);
		}
	}
	else {
		snprintf(debug_text, sizeof(debug_text), "FPS: %.1f", fps);
	}
	draw_text(debug_text, strlen(debug_text), 10, 20);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	if (hit) {
		draw_block_highlight((chunk->x * CHUNK_SIZE) + block_x + 1, (chunk->y * CHUNK_SIZE) + block_y + 1, (chunk->z * CHUNK_SIZE) + block_z + 1);
	}
}