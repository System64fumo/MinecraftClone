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
	static char debug_text[64];
	
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
	
	const float center_x = screen_width/2;
	const float center_y = screen_height/2;
	
	glBegin(GL_LINES);
	glVertex2f(center_x - 10, center_y);
	glVertex2f(center_x + 10, center_y);
	glVertex2f(center_x, center_y - 10);
	glVertex2f(center_x, center_y + 10);
	glEnd();
	
	glDisable(GL_COLOR_LOGIC_OP);

	if (DEBUG)
		snprintf(debug_text, sizeof(debug_text), "FPS: %.1f, X: %.1f, Y: %.1f Z: %.1f, Yaw: %.1f, Pitch: %.1f", fps, player->x, player->y, player->z, player->yaw, player->pitch);
	else
		snprintf(debug_text, sizeof(debug_text), "FPS: %.1f", fps);

	draw_text(debug_text, strlen(debug_text), 10, 20);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}