#include "main.h"
#include "entity.h"
#include "world.h"
#include "views.h"
#include "gui.h"
#include "shaders.h"
#include "config.h"
#include <math.h>

float last_cursor_x = 0;
float last_cursor_y = 0;
static float  last_touch_x       = 0;
static float  last_touch_y       = 0;
static bool   touch_active        = false;
static bool   touch_held          = false;
static float  initial_touch_x     = 0;
static float  initial_touch_y     = 0;
static double touch_start_time    = 0;
static bool   touch_moved         = false;

const double touch_tap_threshold   = 0.3;
const double touch_hold_threshold  = 0.5;
const float  touch_move_threshold  = 10.0f;

static double last_break_time      = 0;
static double last_place_time      = 0;
const  double block_action_interval= 0.5;
const  float  touch_sensitivity    = 0.5f;

int last_pitch, last_yaw;
int last_player_pos_x, last_player_pos_y, last_player_pos_z;
static bool   sprint_key_held      = false;
static double last_space_press_time= -1.0;
static const double DOUBLE_TAP_WINDOW = 0.35;

void check_touch_hold() {
	if (ui_state != UI_STATE_RUNNING || !touch_active || touch_moved || touch_held) return;
	if (time_current - touch_start_time >= touch_hold_threshold)
		touch_held = true;
}

static void update_view_rotation(float xoff, float yoff) {
	global_entities[0].yaw   += xoff;
	global_entities[0].pitch += yoff;
	if (global_entities[0].pitch >  89.0f) global_entities[0].pitch =  89.0f;
	if (global_entities[0].pitch < -89.0f) global_entities[0].pitch = -89.0f;
	if (global_entities[0].yaw >= 360.0f)  global_entities[0].yaw  -= 360.0f;
	if (global_entities[0].yaw <    0.0f)  global_entities[0].yaw  += 360.0f;

	int pitch = (int)floorf(global_entities[0].pitch);
	int yaw   = (int)floorf(global_entities[0].yaw);
	if (pitch != last_pitch) { last_pitch = pitch; frustum_changed = true; }
	if (yaw   != last_yaw)   { last_yaw   = yaw;   frustum_changed = true; }
}

bool check_step_up(Entity entity, float dx, float dz) {
	float len = sqrtf(dx*dx + dz*dz);
	if (len > 0) { dx /= len; dz /= len; }
	float dist = entity.sprinting ? 1.75f : 1.25f;
	float cx   = entity.pos.x + dx * dist;
	float cz   = entity.pos.z + dz * dist;
	int   by   = (int)floorf(entity.pos.y);
	if (is_block_solid(chunks, (int)floorf(cx), by,   (int)floorf(cz)) &&
	   !is_block_solid(chunks, (int)floorf(cx), by+1, (int)floorf(cz)) &&
	   !is_block_solid(chunks, (int)floorf(cx), by+2, (int)floorf(cz)))
		return true;
	return false;
}

static void set_block(bool directional, uint8_t block_id);

void set_hotbar_slot(uint8_t slot) {
	hotbar_slot = slot;
	update_ui();
}

void mouse_button_callback(GLFWwindow *win, int button, int action, int mods) {
	(void)mods;
	if (ui_state == UI_STATE_PAUSED) {
		if (action == GLFW_PRESS) return;
		if (check_hit(last_cursor_x, last_cursor_y, 0)) {
			ui_state = UI_STATE_RUNNING;
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			update_ui();
		} else if (check_hit(last_cursor_x, last_cursor_y, 1)) {
			glfwSetWindowShouldClose(win, true);
		}
	} else if (ui_state == UI_STATE_RUNNING) {
		if (button == GLFW_MOUSE_BUTTON_LEFT || button == GLFW_MOUSE_BUTTON_RIGHT) {
			last_break_time = 0;
			last_place_time = 0;
		} else if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
			char face;
			vec3 pos;
			Block *b = get_targeted_block(global_entities[0], &pos, &face);
			if (b) { hotbar_slot = b->id - 1; update_ui(); }
		}
	}
}

void cursor_position_callback(GLFWwindow *win, double xpos, double ypos) {
	(void)win;
	float xoff = xpos - last_cursor_x;
	float yoff = last_cursor_y - ypos;
	last_cursor_x = xpos;
	last_cursor_y = ypos;
	if (ui_state == UI_STATE_PAUSED)
		view_pause_hover((uint16_t)xpos, (uint16_t)ypos);
	else if (ui_state == UI_STATE_RUNNING)
		update_view_rotation(xoff * 0.1f, yoff * 0.1f);
}

void scroll_callback(GLFWwindow *win, double xoff, double yoff) {
	(void)win; (void)xoff;
	if (ui_state != UI_STATE_RUNNING) return;
	hotbar_slot -= (int8_t)yoff;
	set_hotbar_slot(hotbar_slot);
}

void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
	(void)win; (void)scancode; (void)mods;

	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_LEFT_CONTROL) { sprint_key_held = true; return; }
		if (key == GLFW_KEY_L)  { load_around_entity(&global_entities[0]); return; }
		if (key == GLFW_KEY_F2) { mesh_mode  = !mesh_mode;  return; }
		if (key == GLFW_KEY_F3) { debug_view = !debug_view; update_ui(); return; }
		if (key == GLFW_KEY_R)  { load_shaders(); return; }

		if (key == GLFW_KEY_MINUS) { settings.sky_brightness = 0.05f; return; }
		if (key == GLFW_KEY_EQUAL) { settings.sky_brightness = 1.0f;  return; }

		if (key == GLFW_KEY_SPACE && ui_state == UI_STATE_RUNNING) {
			double now = glfwGetTime();
			if (now - last_space_press_time < DOUBLE_TAP_WINDOW) {
				global_entities[0].flying = !global_entities[0].flying;
				global_entities[0].vertical_velocity = 0.0f;
				global_entities[0].sprinting = false;
				last_space_press_time = -1.0;
			} else {
				last_space_press_time = now;
			}
			return;
		}

		if (key == GLFW_KEY_ESCAPE) {
			if (ui_state == UI_STATE_RUNNING) {
				ui_state = UI_STATE_PAUSED;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				ui_state = UI_STATE_RUNNING;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
			update_ui();
			return;
		}

		if (key >= 49 && key <= 57) { set_hotbar_slot(key - 49); return; }
	}

	if (action == GLFW_RELEASE && key == GLFW_KEY_LEFT_CONTROL)
		sprint_key_held = false;
}

static void set_block(bool directional, uint8_t block_id) {
	vec3 block_pos;
	char face;
	get_targeted_block(global_entities[0], &block_pos, &face);

	if (directional) {
		switch (face) {
			case 'R': block_pos.x--; break;
			case 'L': block_pos.x++; break;
			case 'F': block_pos.z++; break;
			case 'K': block_pos.z--; break;
			case 'T': block_pos.y++; break;
			case 'B': block_pos.y--; break;
			case 'N': return;
		}

		AABB block_aabb = {
			floorf(block_pos.x), floorf(block_pos.x) + 1.0f,
			floorf(block_pos.y), floorf(block_pos.y) + 1.0f,
			floorf(block_pos.z), floorf(block_pos.z) + 1.0f
		};
		AABB player_aabb = get_entity_aabb(
			global_entities[0].pos.x, global_entities[0].pos.y, global_entities[0].pos.z,
			global_entities[0].width, global_entities[0].height);
		if (aabb_intersect(block_aabb, player_aabb)) return;
	}

	Block *block = get_block_at(chunks, block_pos.x, block_pos.y, block_pos.z);
	if (!block) return;

	uint8_t old_id = block->id;

	int chunk_x, chunk_z, block_x, block_z;
	calculate_chunk_and_block(block_pos.x, &chunk_x, &block_x);
	calculate_chunk_and_block(block_pos.z, &chunk_z, &block_z);
	int chunk_y = (int)block_pos.y / CHUNK_SIZE;
	int block_y = (((int)block_pos.y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	int lx      = chunk_x - world_offset_x;
	int lz      = chunk_z - world_offset_z;
	Chunk *chunk= &chunks[lx][chunk_y][lz];

	pthread_mutex_lock(&chunks_mutex);
	block->id = block_id;
	chunk->needs_update = true;
	update_block_lighting((int)block_pos.x, (int)block_pos.y, (int)block_pos.z, old_id, block_id);
	update_adjacent_chunks(chunks, lx, chunk_y, lz, block_x, block_y, block_z);
	pthread_mutex_unlock(&chunks_mutex);

	// Push the edited chunk (and its affected neighbours) to the front of the
	// mesh queue so player edits render immediately even during world generation.
	enqueue_chunk_update_priority(lx, chunk_y, lz);
	if (block_x == 0 && lx > 0)
		enqueue_chunk_update_priority(lx-1, chunk_y, lz);
	else if (block_x == CHUNK_SIZE-1 && lx < settings.render_distance-1)
		enqueue_chunk_update_priority(lx+1, chunk_y, lz);
	if (block_y == 0 && chunk_y > 0)
		enqueue_chunk_update_priority(lx, chunk_y-1, lz);
	else if (block_y == CHUNK_SIZE-1 && chunk_y < WORLD_HEIGHT-1)
		enqueue_chunk_update_priority(lx, chunk_y+1, lz);
	if (block_z == 0 && lz > 0)
		enqueue_chunk_update_priority(lx, chunk_y, lz-1);
	else if (block_z == CHUNK_SIZE-1 && lz < settings.render_distance-1)
		enqueue_chunk_update_priority(lx, chunk_y, lz+1);
}

void process_input(GLFWwindow *win, Chunk ***ch) {
	(void)ch;
	if (ui_state != UI_STATE_RUNNING) return;

	check_touch_hold();

	const bool break_pressed = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS;
	const bool place_pressed = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

	if ((break_pressed || touch_held) && time_current - last_break_time >= block_action_interval) {
		set_block(false, 0);
		last_break_time = time_current;
	} else if (place_pressed && time_current - last_place_time >= block_action_interval) {
		set_block(true, hotbar_slot + 1);
		last_place_time = time_current;
	}

	int player_cx = (int)(global_entities[0].pos.x / CHUNK_SIZE);
	int player_cy = (int)(global_entities[0].pos.y / CHUNK_SIZE);
	int player_cz = (int)(global_entities[0].pos.z / CHUNK_SIZE);
	int lx        = player_cx - world_offset_x;
	int lz        = player_cz - world_offset_z;

	if (player_cy > 0 && player_cy < WORLD_HEIGHT &&
	    is_chunk_in_bounds(lx, player_cy, lz) &&
	    !chunks[lx][player_cy][lz].is_loaded)
		return;

	float move_speed = global_entities[0].speed;
	bool  flying     = global_entities[0].flying;

	if (global_entities[0].sprinting) {
		move_speed *= flying ? 4.0f : 1.3f;
	} else if (sprint_key_held && glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
		global_entities[0].sprinting = true;
		move_speed *= flying ? 4.0f : 1.3f;
	}

	float fov_target = global_entities[0].sprinting
	    ? settings.fov_desired * (flying ? 1.2f : 1.1f)
	    : settings.fov_desired;
	float fov_speed = 10.0f * (float)delta_time;
	if (fov_speed > 1.0f) fov_speed = 1.0f;
	settings.fov += (fov_target - settings.fov) * fov_speed;

	move_speed *= delta_time;
	float yaw = global_entities[0].yaw * DEG_TO_RAD;
	float dx = 0.f, dy = 0.f, dz = 0.f;

	if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS) {
		dx += cosf(yaw) * move_speed;
		dz += sinf(yaw) * move_speed;
		if (!flying && settings.auto_jump && global_entities[0].is_grounded &&
		    check_step_up(global_entities[0], dx, dz)) {
			global_entities[0].vertical_velocity = 10.0f;
			global_entities[0].is_grounded = false;
		}
	} else if (glfwGetKey(win, GLFW_KEY_W) == GLFW_RELEASE) {
		global_entities[0].sprinting = false;
	}
	if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS) { dx -= cosf(yaw)*move_speed; dz -= sinf(yaw)*move_speed; }
	if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS) { dx += sinf(yaw)*move_speed; dz -= cosf(yaw)*move_speed; }
	if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS) { dx -= sinf(yaw)*move_speed; dz += cosf(yaw)*move_speed; }

	if (flying) {
		if (glfwGetKey(win, GLFW_KEY_SPACE)      == GLFW_PRESS) dy += move_speed;
		if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) dy -= move_speed;
		move_entity_with_collision(&global_entities[0], dx, 0.f, dz);
		global_entities[0].pos.y += dy;
	} else {
		if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS && global_entities[0].is_grounded) {
			global_entities[0].vertical_velocity = 10.0f;
			global_entities[0].is_grounded = false;
		}
		if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) dy -= move_speed;
		move_entity_with_collision(&global_entities[0], dx, dy, dz);
	}
	update_entity_physics(&global_entities[0], delta_time);

	int x = (int)floorf(global_entities[0].pos.x);
	int y = (int)floorf(global_entities[0].pos.y);
	int z = (int)floorf(global_entities[0].pos.z);
	if (x == last_player_pos_x && y == last_player_pos_y && z == last_player_pos_z) return;
	last_player_pos_x = x;
	last_player_pos_y = y;
	last_player_pos_z = z;
	frustum_changed = true;
}

void touch_down(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time,
                struct wl_surface *surface, int32_t id, wl_fixed_t x, wl_fixed_t y) {
	(void)data; (void)wl_touch; (void)serial; (void)time; (void)surface; (void)id;
	initial_touch_x = last_touch_x = wl_fixed_to_double(x);
	initial_touch_y = last_touch_y = wl_fixed_to_double(y);
	touch_active     = true;
	touch_moved      = false;
	touch_start_time = time_current;
	touch_held       = false;
}

void touch_up(void *data, struct wl_touch *wl_touch, uint32_t serial, uint32_t time, int32_t id) {
	(void)data; (void)wl_touch; (void)serial; (void)time; (void)id;
	if (!touch_moved && !touch_held) {
		double dur = time_current - touch_start_time;
		if (dur < touch_tap_threshold && time_current - last_place_time >= block_action_interval) {
			set_block(true, hotbar_slot + 1);
			last_place_time = time_current;
		}
	}
	touch_active = false;
	touch_held   = false;
}

void touch_motion(void *data, struct wl_touch *wl_touch, uint32_t time, int32_t id,
                  wl_fixed_t x, wl_fixed_t y) {
	(void)data; (void)wl_touch; (void)time; (void)id;
	float cx = wl_fixed_to_double(x);
	float cy = wl_fixed_to_double(y);
	if (!touch_moved) {
		if (fabsf(cx - initial_touch_x) > touch_move_threshold ||
		    fabsf(cy - initial_touch_y) > touch_move_threshold)
			touch_moved = true;
	}
	if (ui_state == UI_STATE_RUNNING && touch_moved)
		update_view_rotation((cx - last_touch_x) * touch_sensitivity,
		                     (last_touch_y - cy)  * touch_sensitivity);
	last_touch_x = cx;
	last_touch_y = cy;
}