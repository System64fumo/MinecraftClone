#include "main.h"

void process_keyboard_movement(const Uint8* key_state, Player* player, float delta_time) {
	const float PI = 3.14159265358979323846f;
	if (key_state[SDL_SCANCODE_W]) {
		player->z -= cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->x -= -sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_S]) {
		player->z += cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->x += -sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_A]) {
		player->x -= cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->z -= sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_D]) {
		player->x += cos(player->yaw * PI / 180.0f) * player->speed * delta_time;
		player->z += sin(player->yaw * PI / 180.0f) * player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_SPACE]) {
		player->y += player->speed * delta_time;
	}
	if (key_state[SDL_SCANCODE_LSHIFT] || key_state[SDL_SCANCODE_RSHIFT]) {
		player->y -= player->speed * delta_time;
	}
}