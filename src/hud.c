#include "main.h"

void draw_hud(float fps) {
	// Save current matrices and attributes
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	// Switch to 2D orthographic projection
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, screen_width, screen_height, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	// Disable depth testing for HUD
	glDisable(GL_DEPTH_TEST);
	
	// Draw crosshair
	glEnable(GL_COLOR_LOGIC_OP);
	glLogicOp(GL_INVERT);
	
	glBegin(GL_LINES);
	// Horizontal line
	glVertex2f(screen_width/2 - 10, screen_height/2);
	glVertex2f(screen_width/2 + 10, screen_height/2);
	// Vertical line
	glVertex2f(screen_width/2, screen_height/2 - 10);
	glVertex2f(screen_width/2, screen_height/2 + 10);
	glEnd();
	
	glDisable(GL_COLOR_LOGIC_OP);
	
	// Restore previous state
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}