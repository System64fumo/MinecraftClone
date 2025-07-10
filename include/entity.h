#ifndef ENTITY_H
#define ENTITY_H

#include "misc.h"
#include <stdbool.h>

typedef struct {
	uint8_t id;
	uint8_t quantity;
	uint16_t data;
} Item;

typedef struct Entity {
	vec3 pos;
	float yaw, pitch;
	float width, height;
	float eye_level;
	float reach;
	uint8_t speed;
	bool is_grounded;
	bool sprinting;
	float vertical_velocity;
	uint8_t inventory_size;
	Item* inventory_data;
} Entity;

typedef struct Block Block;

typedef struct {
	float min_x, max_x;
	float min_y, max_y;
	float min_z, max_z;
} AABB;

extern Entity global_entities[MAX_ENTITIES_PER_CHUNK];

vec3 get_direction(float pitch, float yaw);
Block* get_targeted_block(Entity entity, vec3* pos_out, char* out_face);
void move_entity_with_collision(Entity* entity, float dx, float dy, float dz);
void update_entity_physics(Entity* player, float delta_time);
bool check_entity_collision(float x, float y, float z, float width, float height);
Entity create_entity(uint8_t id);
AABB get_entity_aabb(float x, float y, float z, float width, float height);
bool aabb_intersect(AABB a, AABB b);

#endif