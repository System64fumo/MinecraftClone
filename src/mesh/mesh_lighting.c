#include "main.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MAX_LIGHT_LEVEL 15

static uint8_t get_block_emission(uint8_t id) {
	switch (id) {
		case 10: case 11: case 89: return 15;
		default: return 0;
	}
}

static uint8_t get_sky_opacity(uint8_t id) {
	if (id == 0) return 0;
	if (block_data[id][1] == 0) return 15;
	switch (id) {
		case 18:    return 1;
		case 8: case 9: return 2;
		default:    return 0;
	}
}

static uint8_t get_block_opacity(uint8_t id) {
	if (id == 0) return 0;
	if (block_data[id][1] == 0) return 15;
	switch (id) {
		case 18:    return 1;
		case 8: case 9: return 2;
		default:    return 0;
	}
}

static bool world_to_chunk(int wx, int wy, int wz,
                            int *ci_x, int *ci_y, int *ci_z,
                            int *lx,   int *ly,   int *lz) {
	int cx = (wx < 0) ? ((wx + 1) / CHUNK_SIZE - 1) : (wx / CHUNK_SIZE);
	int cy =  wy / CHUNK_SIZE;
	int cz = (wz < 0) ? ((wz + 1) / CHUNK_SIZE - 1) : (wz / CHUNK_SIZE);

	*ci_x = cx - (int)atomic_load(&world_offset_x);
	*ci_y = cy;
	*ci_z = cz - (int)atomic_load(&world_offset_z);

	if (*ci_x < 0 || *ci_x >= settings.render_distance) return false;
	if (*ci_y < 0 || *ci_y >= WORLD_HEIGHT)              return false;
	if (*ci_z < 0 || *ci_z >= settings.render_distance) return false;

	*lx = ((wx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	*ly = ((wy % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	*lz = ((wz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	return true;
}

static uint8_t get_light(int wx, int wy, int wz) {
	int ci_x, ci_y, ci_z, lx, ly, lz;
	if (!world_to_chunk(wx, wy, wz, &ci_x, &ci_y, &ci_z, &lx, &ly, &lz)) return 0;
	Chunk *c = &chunks[ci_x][ci_y][ci_z];
	return c->is_loaded ? c->blocks[lx][ly][lz].light_level : 0;
}

static bool set_light(int wx, int wy, int wz, uint8_t level) {
	int ci_x, ci_y, ci_z, lx, ly, lz;
	if (!world_to_chunk(wx, wy, wz, &ci_x, &ci_y, &ci_z, &lx, &ly, &lz)) return false;
	Chunk *c = &chunks[ci_x][ci_y][ci_z];
	if (!c->is_loaded) return false;
	c->blocks[lx][ly][lz].light_level = level;
	c->needs_update = true;
	return true;
}

static uint8_t get_id(int wx, int wy, int wz) {
	int ci_x, ci_y, ci_z, lx, ly, lz;
	if (!world_to_chunk(wx, wy, wz, &ci_x, &ci_y, &ci_z, &lx, &ly, &lz)) return 1;
	Chunk *c = &chunks[ci_x][ci_y][ci_z];
	return c->is_loaded ? c->blocks[lx][ly][lz].id : 1;
}

typedef struct { int32_t x, y, z; uint8_t sky, blk; } light_node_t;
typedef struct { light_node_t *data; int head, tail, cap; } light_queue_t;

static void lq_init(light_queue_t *q, int cap) {
	q->data = malloc(cap * sizeof(light_node_t));
	q->head = q->tail = 0;
	q->cap  = cap;
}
static void      lq_free (light_queue_t *q)              { free(q->data); q->data = NULL; }
static bool      lq_empty(const light_queue_t *q)        { return q->head >= q->tail; }
static light_node_t lq_pop(light_queue_t *q)             { return q->data[q->head++]; }
static void      lq_push (light_queue_t *q, light_node_t n) {
	if (q->tail >= q->cap) {
		q->cap *= 2;
		q->data = realloc(q->data, q->cap * sizeof(light_node_t));
	}
	q->data[q->tail++] = n;
}

static const int8_t ddx6[] = { 1,-1, 0, 0, 0, 0 };
static const int8_t ddy6[] = { 0, 0, 1,-1, 0, 0 };
static const int8_t ddz6[] = { 0, 0, 0, 0, 1,-1 };

static void add_bfs(light_queue_t *aq) {
	while (!lq_empty(aq)) {
		light_node_t node = lq_pop(aq);
		for (int d = 0; d < 6; d++) {
			int nx = node.x + ddx6[d];
			int ny = node.y + ddy6[d];
			int nz = node.z + ddz6[d];
			uint8_t nid    = get_id(nx, ny, nz);
			uint8_t sky_op = get_sky_opacity(nid);
			uint8_t blk_op = get_block_opacity(nid);
			if (sky_op == 15) continue;
			uint8_t cur = get_light(nx, ny, nz);
			uint8_t cs  = SKY_LIGHT(cur), cb = BLOCK_LIGHT(cur);
			uint8_t ns  = (d == 3 && sky_op == 0) ? node.sky
			            : (node.sky > 1 + sky_op   ? node.sky - 1 - sky_op : 0);
			uint8_t nb  = node.blk > 1 + blk_op   ? node.blk - 1 - blk_op : 0;
			bool changed = false;
			if (ns > cs) { cs = ns; changed = true; }
			if (nb > cb) { cb = nb; changed = true; }
			if (changed) {
				set_light(nx, ny, nz, PACK_LIGHT(cs, cb));
				lq_push(aq, (light_node_t){nx, ny, nz, cs, cb});
			}
		}
	}
}

static void remove_bfs(light_queue_t *rq, light_queue_t *aq) {
	while (!lq_empty(rq)) {
		light_node_t node = lq_pop(rq);
		for (int d = 0; d < 6; d++) {
			int nx = node.x + ddx6[d];
			int ny = node.y + ddy6[d];
			int nz = node.z + ddz6[d];
			uint8_t cur    = get_light(nx, ny, nz);
			uint8_t cs     = SKY_LIGHT(cur), cb = BLOCK_LIGHT(cur);
			if (cs == 0 && cb == 0) continue;
			uint8_t nid    = get_id(nx, ny, nz);
			uint8_t sky_op = get_sky_opacity(nid);
			uint8_t blk_op = get_block_opacity(nid);
			if (sky_op == 15) continue;
			bool rs = false, rb = false;
			if (cs > 0 && node.sky > 0) {
				uint8_t exp = (d == 3 && sky_op == 0) ? node.sky
				            : (node.sky > 1 + sky_op   ? node.sky - 1 - sky_op : 0);
				if (cs <= exp) rs = true;
			}
			if (cb > 0 && node.blk > 0) {
				uint8_t exp = node.blk > 1 + blk_op ? node.blk - 1 - blk_op : 0;
				if (cb <= exp) rb = true;
			}
			if (!rs && !rb) { lq_push(aq, (light_node_t){nx,ny,nz,cs,cb}); continue; }
			uint8_t ns = rs ? 0 : cs;
			uint8_t nb = rb ? 0 : cb;
			set_light(nx, ny, nz, PACK_LIGHT(ns, nb));
			lq_push(rq, (light_node_t){nx, ny, nz, rs ? cs : 0, rb ? cb : 0});
			uint8_t emit = get_block_emission(nid);
			uint8_t rs2  = rs ? 0 : cs;
			uint8_t rb2  = rb ? 0 : cb;
			if (emit > rb2) rb2 = emit;
			if (rs2 > 0 || rb2 > 0) lq_push(aq, (light_node_t){nx, ny, nz, rs2, rb2});
		}
	}
}

void init_column_lighting(Chunk col[WORLD_HEIGHT]) {
	for (int cy = 0; cy < WORLD_HEIGHT; cy++)
		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int y = 0; y < CHUNK_SIZE; y++)
				for (int z = 0; z < CHUNK_SIZE; z++)
					col[cy].blocks[x][y][z].light_level = 0;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint8_t sky = MAX_LIGHT_LEVEL;
			for (int cy = WORLD_HEIGHT - 1; cy >= 0 && sky > 0; cy--) {
				for (int y = CHUNK_SIZE - 1; y >= 0 && sky > 0; y--) {
					uint8_t op = get_sky_opacity(col[cy].blocks[x][y][z].id);
					if (op == 15) { sky = 0; break; }
					if (op > 0)   { if (sky <= op) { sky = 0; break; } sky -= op; }
					col[cy].blocks[x][y][z].light_level = PACK_LIGHT(sky, 0);
				}
			}
		}
	}

	for (int cy = 0; cy < WORLD_HEIGHT; cy++)
		for (int x = 0; x < CHUNK_SIZE; x++)
			for (int y = 0; y < CHUNK_SIZE; y++)
				for (int z = 0; z < CHUNK_SIZE; z++) {
					uint8_t emit = get_block_emission(col[cy].blocks[x][y][z].id);
					if (!emit) continue;
					uint8_t cur = col[cy].blocks[x][y][z].light_level;
					if (emit > BLOCK_LIGHT(cur))
						col[cy].blocks[x][y][z].light_level = PACK_LIGHT(SKY_LIGHT(cur), emit);
				}

	for (int cy = 0; cy < WORLD_HEIGHT; cy++)
		col[cy].lighting_changed = true;
}

void init_chunk_lighting(Chunk *chunk) { (void)chunk; }

void relight_chunk(Chunk *chunk) {
	int wx0 = chunk->x * CHUNK_SIZE;
	int wy0 = chunk->y * CHUNK_SIZE;
	int wz0 = chunk->z * CHUNK_SIZE;

	light_queue_t aq, rq;
	lq_init(&aq, 16384);
	lq_init(&rq, 8192);

	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++) {
				uint8_t lv = chunk->blocks[x][y][z].light_level;
				if (!lv) continue;
				chunk->blocks[x][y][z].light_level = 0;
				lq_push(&rq, (light_node_t){wx0+x, wy0+y, wz0+z, SKY_LIGHT(lv), BLOCK_LIGHT(lv)});
			}

	remove_bfs(&rq, &aq);
	lq_free(&rq);

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint8_t sky = MAX_LIGHT_LEVEL;
			for (int cy = WORLD_HEIGHT - 1; cy > (int)chunk->ci_y; cy--) {
				Chunk *above = &chunks[chunk->ci_x][cy][chunk->ci_z];
				if (!above->is_loaded) break;
				for (int by = CHUNK_SIZE - 1; by >= 0; by--) {
					uint8_t op = get_sky_opacity(above->blocks[x][by][z].id);
					if (op == 15) { sky = 0; goto rs_next; }
					if (op > 0)   { if (sky <= op) { sky = 0; goto rs_next; } sky -= op; }
				}
			}
			for (int y = CHUNK_SIZE - 1; y >= 0 && sky > 0; y--) {
				uint8_t op = get_sky_opacity(chunk->blocks[x][y][z].id);
				if (op == 15) break;
				if (op > 0) { if (sky <= op) { sky = 0; break; } sky -= op; }
				uint8_t cur = chunk->blocks[x][y][z].light_level;
				if (sky > SKY_LIGHT(cur)) {
					chunk->blocks[x][y][z].light_level = PACK_LIGHT(sky, BLOCK_LIGHT(cur));
					lq_push(&aq, (light_node_t){wx0+x, wy0+y, wz0+z, sky, BLOCK_LIGHT(cur)});
				}
			}
			rs_next:;
		}
	}

	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++) {
				uint8_t emit = get_block_emission(chunk->blocks[x][y][z].id);
				if (!emit) continue;
				uint8_t cur = chunk->blocks[x][y][z].light_level;
				if (emit > BLOCK_LIGHT(cur)) {
					chunk->blocks[x][y][z].light_level = PACK_LIGHT(SKY_LIGHT(cur), emit);
					lq_push(&aq, (light_node_t){wx0+x, wy0+y, wz0+z, SKY_LIGHT(cur), emit});
				}
			}

	add_bfs(&aq);
	lq_free(&aq);
}

static void seed_sky_col_down(int wx, int wy, int wz, uint8_t sky, light_queue_t *aq) {
	for (int y = wy; y >= 0; y--) {
		uint8_t op = get_sky_opacity(get_id(wx, y, wz));
		if (op == 15) break;
		if (op > 0) { if (sky <= op) { sky = 0; break; } sky -= op; }
		uint8_t cur = get_light(wx, y, wz);
		if (sky > SKY_LIGHT(cur)) {
			set_light(wx, y, wz, PACK_LIGHT(sky, BLOCK_LIGHT(cur)));
			lq_push(aq, (light_node_t){wx, y, wz, sky, BLOCK_LIGHT(cur)});
		}
	}
}

static void rem_sky_col_down(int wx, int wy, int wz, light_queue_t *rq) {
	for (int y = wy; y >= 0; y--) {
		if (get_sky_opacity(get_id(wx, y, wz)) == 15) break;
		uint8_t cl = get_light(wx, y, wz);
		uint8_t cs = SKY_LIGHT(cl), cb = BLOCK_LIGHT(cl);
		if (cs == 0) break;
		set_light(wx, y, wz, PACK_LIGHT(0, cb));
		lq_push(rq, (light_node_t){wx, y, wz, cs, 0});
	}
}

static uint8_t sky_above(int wx, int wy, int wz) {
	uint8_t sky = MAX_LIGHT_LEVEL;
	for (int y = wy + 1; y < WORLD_HEIGHT * CHUNK_SIZE; y++) {
		uint8_t op = get_sky_opacity(get_id(wx, y, wz));
		if (op == 15) return 0;
		if (op > 0) { if (sky <= op) return 0; sky -= op; }
	}
	return sky;
}

void update_block_lighting(int wx, int wy, int wz, uint8_t old_id, uint8_t new_id) {
	light_queue_t aq, rq;
	lq_init(&aq, 4096);
	lq_init(&rq, 4096);

	uint8_t old_emit   = get_block_emission(old_id);
	uint8_t new_emit   = get_block_emission(new_id);
	uint8_t old_sky_op = get_sky_opacity(old_id);
	uint8_t new_sky_op = get_sky_opacity(new_id);

	if (old_emit > 0) {
		uint8_t cur = get_light(wx, wy, wz);
		uint8_t cb  = BLOCK_LIGHT(cur);
		if (cb > 0) {
			set_light(wx, wy, wz, PACK_LIGHT(SKY_LIGHT(cur), 0));
			lq_push(&rq, (light_node_t){wx, wy, wz, 0, cb});
			remove_bfs(&rq, &aq);
			rq.head = rq.tail = 0;
		}
	}

	if (old_sky_op != new_sky_op) {
		if (new_sky_op > old_sky_op) {
			uint8_t cur = get_light(wx, wy, wz);
			uint8_t cs  = SKY_LIGHT(cur);
			if (cs > 0) {
				set_light(wx, wy, wz, PACK_LIGHT(0, BLOCK_LIGHT(cur)));
				lq_push(&rq, (light_node_t){wx, wy, wz, cs, 0});
			}
			rem_sky_col_down(wx, wy - 1, wz, &rq);
			remove_bfs(&rq, &aq);
			rq.head = rq.tail = 0;
		} else {
			uint8_t si = sky_above(wx, wy, wz);
			if (si > 0) seed_sky_col_down(wx, wy, wz, si, &aq);
			for (int d = 0; d < 6; d++) {
				int nx = wx + ddx6[d], ny = wy + ddy6[d], nz = wz + ddz6[d];
				uint8_t nb = get_light(nx, ny, nz);
				if (SKY_LIGHT(nb) || BLOCK_LIGHT(nb))
					lq_push(&aq, (light_node_t){nx, ny, nz, SKY_LIGHT(nb), BLOCK_LIGHT(nb)});
			}
		}
	} else if (new_sky_op == 15) {
		uint8_t cur = get_light(wx, wy, wz);
		if (cur > 0) {
			set_light(wx, wy, wz, 0);
			lq_push(&rq, (light_node_t){wx, wy, wz, SKY_LIGHT(cur), BLOCK_LIGHT(cur)});
			remove_bfs(&rq, &aq);
			rq.head = rq.tail = 0;
		}
	}

	if (new_emit > 0) {
		uint8_t cs = SKY_LIGHT(get_light(wx, wy, wz));
		set_light(wx, wy, wz, PACK_LIGHT(cs, new_emit));
		lq_push(&aq, (light_node_t){wx, wy, wz, cs, new_emit});
	}

	add_bfs(&aq);
	lq_free(&aq);
	lq_free(&rq);

	int ci_x, ci_y, ci_z, lx, ly, lz;
	if (world_to_chunk(wx, wy, wz, &ci_x, &ci_y, &ci_z, &lx, &ly, &lz)) {
		Chunk *c = &chunks[ci_x][ci_y][ci_z];
		if (c->is_loaded) c->lighting_changed = true;
	}
}

unsigned char* generate_light_texture() { return NULL; }
