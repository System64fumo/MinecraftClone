typedef struct {
	int x;
	int y;
	int z;
	float distance;
} chunk_load_item_t;

#define GET_DIGIT(num, exp) ((int)(num / exp) % 10)

extern int last_cx;
extern int last_cy;
extern int last_cz;

extern const int SEA_LEVEL;
extern const float continent_scale;
extern const float mountain_scale;
extern const float flatness_scale;
extern const float cave_scale;
extern const float cave_simplex_scale;

uint32_t serialize(uint8_t a, uint8_t b, uint8_t c);
void deserialize(uint32_t serialized, uint8_t *a, uint8_t *b, uint8_t *c);
void counting_sort(chunk_load_item_t* arr, int size, int exp);
void radix_sort(chunk_load_item_t* arr, int size);
void load_around_entity(Entity* entity);
int save_chunk_to_file(const char *filename, const Chunk *chunk);
int load_chunk_from_file(const char *filename, Chunk *chunk);
void load_chunk_data(Chunk* chunk, unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz);
void load_chunk(unsigned char ci_x, unsigned char ci_y, unsigned char ci_z, int cx, int cy, int cz);
void unload_chunk(Chunk* chunk);
void generate_chunk_terrain(Chunk* chunk, int chunk_x, int chunk_y, int chunk_z);