#include "misc.h"
#include <stdbool.h>

typedef struct {
	float min_x, max_x;
	float min_y, max_y;
	float min_z, max_z;
} AABB;

typedef struct {
	uint8_t id;
	uint8_t quantity;
	uint16_t data;
} Item;

typedef struct {
	vec3 pos;
	float yaw, pitch;
	float width, height;
	float eye_level;
	uint8_t speed;
	bool is_grounded;
	bool sprinting;
	float vertical_velocity;
	uint8_t inventory_size;
	Item* inventory_data; // TODO: Implement proper inventory struct
} Entity;

extern Entity global_entities[MAX_ENTITIES_PER_CHUNK];

vec3 get_direction(float pitch, float yaw);
void get_targeted_block(Entity entity, vec3 direction, float reach, vec3* pos_out, char* out_face, uint8_t* block_id);
void move_entity_with_collision(Entity* entity, float dx, float dy, float dz);
void update_entity_physics(Entity* player, float delta_time);
bool check_entity_collision(float x, float y, float z, float width, float height);
Entity create_entity(uint8_t id);
AABB get_entity_aabb(float x, float y, float z, float width, float height);
bool aabb_intersect(AABB a, AABB b);