#include "main.h"
#include <stdio.h>
#include <GL/glut.h>

int world_block_x;
int world_block_y;
int world_block_z;

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

void draw_minimap(Entity* player) {
	const int map_size = 160;
	const int map_x = screen_width - map_size - 10;
	const int map_y = 10;
	const int chunk_size = map_size / RENDERR_DISTANCE;
	
	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, screen_width, screen_height, 0, -1, 1);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);
	
	// Draw map background
	glColor4f(0.8f, 0.8f, 0.8f, 0.5f);
	glBegin(GL_QUADS);
	glVertex2f(map_x, map_y);
	glVertex2f(map_x + map_size, map_y);
	glVertex2f(map_x + map_size, map_y + map_size);
	glVertex2f(map_x, map_y + map_size);
	glEnd();
	
	// Draw chunks
	for(int x = 0; x < RENDERR_DISTANCE; x++) {
		for(int z = 0; z < RENDERR_DISTANCE; z++) {
			if(chunks[x][0][z].vbo) {
				glColor4f(0.0f, 0.6f, 0.2f, 0.7f);
				glBegin(GL_QUADS);
				glVertex2f(map_x + x * chunk_size, map_y + z * chunk_size);
				glVertex2f(map_x + (x + 1) * chunk_size, map_y + z * chunk_size);
				glVertex2f(map_x + (x + 1) * chunk_size, map_y + (z + 1) * chunk_size);
				glVertex2f(map_x + x * chunk_size, map_y + (z + 1) * chunk_size);
				glEnd();
			}
		}
	}
	
	// Draw grid
	glColor3f(0.0f, 0.55f, 0.15f);
	glBegin(GL_LINES);
	for(int i = 0; i <= RENDERR_DISTANCE; i++) {
		// Vertical lines
		glVertex2f(map_x + i * chunk_size, map_y);
		glVertex2f(map_x + i * chunk_size, map_y + map_size);
		// Horizontal lines
		glVertex2f(map_x, map_y + i * chunk_size);
		glVertex2f(map_x + map_size, map_y + i * chunk_size);
	}
	glEnd();
	
	// Draw player position
	float player_x = ((player->x - chunks[0][0][0].x * CHUNK_SIZE) / (float)(CHUNK_SIZE * RENDERR_DISTANCE)) * map_size;
	float player_z = ((player->z - chunks[0][0][0].z * CHUNK_SIZE) / (float)(CHUNK_SIZE * RENDERR_DISTANCE)) * map_size;
	glColor3f(1.0f, 0.75f, 0.25f);
	glPointSize(6.0f);
	glBegin(GL_POINTS);
	glVertex2f(map_x + player_x, map_y + player_z);
	glEnd();

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();
}

void draw_hud(float fps, Entity* player) {
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

	get_targeted_block(player, &world_block_x, &world_block_y, &world_block_z);

	#ifdef DEBUG
		const char* direction = 
			(player->yaw < 45 || player->yaw >= 315) ? "South" :
			(player->yaw < 135) ? "West" :
			(player->yaw < 225) ? "North" : "East";
			
		const char* pitch = 
			(player->pitch > 45) ? "Up" :
			(player->pitch < -45) ? "Down" : "";
			
		snprintf(debug_text, sizeof(debug_text), 
			"FPS: %.1f, X: %.1f, Y: %.1f Z: %.1f, Direction: %s %s", 
			fps, -(((WORLD_SIZE * CHUNK_SIZE) / 2) - player->x), player->y, -(((WORLD_SIZE * CHUNK_SIZE) / 2) - player->z), direction, pitch);
	#else
		snprintf(debug_text, sizeof(debug_text), "FPS: %.1f", fps);
	#endif

	draw_text(debug_text, strlen(debug_text), 10, 20);
	#ifdef DEBUG
	draw_minimap(player);
	char block_pos[64];
	if (world_block_x != -1 && world_block_y != -1 && world_block_z != -1) {
		snprintf(block_pos, sizeof(block_pos), "Looking at block: %d, %d, %d", world_block_x, world_block_y, world_block_z);
		draw_text(block_pos, strlen(block_pos), 10, 40);
	}
	#endif

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	if (world_block_x != -1 && world_block_y != -1 && world_block_z != -1) {
		draw_block_highlight(world_block_x + 1, world_block_y + 1, world_block_z + 1);
	}
}