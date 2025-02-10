#include "main.h"
#include <GL/glut.h>

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
	const int map_size = 100;
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
	
	// Draw player position
	float player_x = ((player->x - chunks[0][0][0].x * CHUNK_SIZE) / (float)(CHUNK_SIZE * RENDERR_DISTANCE)) * map_size;
	float player_z = ((player->z - chunks[0][0][0].z * CHUNK_SIZE) / (float)(CHUNK_SIZE * RENDERR_DISTANCE)) * map_size;	
	glColor3f(0.75f, 0.75f, 0.75f);
	glPointSize(6.0f);
	glBegin(GL_POINTS);
	glVertex2f(map_x + player_x, map_y + player_z);
	glEnd();
	
	// Draw player direction line
	float base_length = 15.0f;
	float pitch_factor = cosf(player->pitch * M_PI / 180.0f);  // Shorter when looking up/down
	float direction_length = base_length * pitch_factor;
	float dx = sinf(player->yaw * M_PI / 180.0f) * direction_length;
	float dz = cosf(player->yaw * M_PI / 180.0f) * direction_length;
	glColor3f(1.0f, 0.0f, 0.0f);  // Red direction line
	glBegin(GL_LINES);
	glVertex2f(map_x + player_x, map_y + player_z);
	glVertex2f(map_x + player_x + dx, map_y + player_z - dz);
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
	
	// Raycast to find block player is looking at
	int block_x, block_y, block_z;
	Chunk* chunk;
	float hit_distance;
	bool hit = raycast(player, 64.0f, &block_x, &block_y, &block_z, &hit_distance);

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
	draw_minimap(player);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glPopAttrib();

	if (hit) {
		draw_block_highlight(block_x + 1, block_y + 1, block_z + 1);
	}
}