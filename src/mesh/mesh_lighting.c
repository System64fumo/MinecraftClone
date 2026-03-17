#include "main.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// SKY_LIGHT / BLOCK_LIGHT / PACK_LIGHT are defined in world.h

#define MAX_LIGHT_LEVEL 15

static uint8_t get_block_emission(uint8_t id) {
	switch (id) {
		case 10: return 15; // Lava (flowing)
		case 11: return 15; // Lava (stationary)
		case 89: return 15; // Glowstone
		default: return 0;
	}
}

static uint8_t get_sky_opacity(uint8_t id) {
	if (id == 0) return 0;
	if (block_data[id][1] == 0) return 15;
	switch (id) {
		case 18: return 3; // Leaves
		case 8:
		case 9:  return 2; // Water
		default: return 0;
	}
}

static uint8_t get_block_opacity(uint8_t id) {
	if (id == 0) return 0;
	if (block_data[id][1] == 0) return 15;
	switch (id) {
		case 18: return 2; // Leaves
		case 8:
		case 9:  return 1; // Water
		default: return 0;
	}
}

static bool world_to_chunk(int wx, int wy, int wz,
							int *ci_x, int *ci_y, int *ci_z,
							int *lx,   int *ly,   int *lz) {
	int cx = (wx < 0) ? ((wx + 1) / CHUNK_SIZE - 1) : (wx / CHUNK_SIZE);
	int cy = wy / CHUNK_SIZE;
	int cz = (wz < 0) ? ((wz + 1) / CHUNK_SIZE - 1) : (wz / CHUNK_SIZE);

	*ci_x = cx - (int)atomic_load(&world_offset_x);
	*ci_y = cy;
	*ci_z = cz - (int)atomic_load(&world_offset_z);

	if (*ci_x < 0 || *ci_x >= settings.render_distance) return false;
	if (*ci_y < 0 || *ci_y >= WORLD_HEIGHT)			  return false;
	if (*ci_z < 0 || *ci_z >= settings.render_distance)  return false;

	*lx = ((wx % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	*ly = ((wy % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	*lz = ((wz % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
	return true;
}

static uint8_t get_light(int wx, int wy, int wz) {
	int ci_x, ci_y, ci_z, lx, ly, lz;
	if (!world_to_chunk(wx, wy, wz, &ci_x, &ci_y, &ci_z, &lx, &ly, &lz)) return 0;
	Chunk *c = &chunks[ci_x][ci_y][ci_z];
	if (!c->is_loaded) return 0;
	return c->blocks[lx][ly][lz].light_level;
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
	if (!c->is_loaded) return 1;
	return c->blocks[lx][ly][lz].id;
}

typedef struct {
	int32_t x, y, z;
	uint8_t sky;
	uint8_t blk;
} wlight_node_t;

#define WLIGHT_QUEUE_SIZE 65536
static wlight_node_t add_queue[WLIGHT_QUEUE_SIZE];
static wlight_node_t rem_queue[WLIGHT_QUEUE_SIZE];

static const int8_t ddx6[] = { 1,-1, 0, 0, 0, 0 };
static const int8_t ddy6[] = { 0, 0, 1,-1, 0, 0 };
static const int8_t ddz6[] = { 0, 0, 0, 0, 1,-1 };

static void add_bfs(int *a_head, int *a_tail) {
	while (*a_head < *a_tail) {
		wlight_node_t node = add_queue[(*a_head)++];

		for (int d = 0; d < 6; d++) {
			int nx = node.x + ddx6[d];
			int ny = node.y + ddy6[d];
			int nz = node.z + ddz6[d];

			uint8_t nid	= get_id(nx, ny, nz);
			uint8_t sky_op = get_sky_opacity(nid);
			uint8_t blk_op = get_block_opacity(nid);
			if (sky_op == 15) continue;

			uint8_t cur	  = get_light(nx, ny, nz);
			uint8_t cur_sky  = SKY_LIGHT(cur);
			uint8_t cur_blk  = BLOCK_LIGHT(cur);
			bool changed = false;

			uint8_t next_sky = node.sky > (1 + sky_op) ? node.sky - 1 - sky_op : 0;
			uint8_t next_blk = node.blk > (1 + blk_op) ? node.blk - 1 - blk_op : 0;

			if (next_sky > cur_sky) { cur_sky = next_sky; changed = true; }
			if (next_blk > cur_blk) { cur_blk = next_blk; changed = true; }

			if (changed) {
				set_light(nx, ny, nz, PACK_LIGHT(cur_sky, cur_blk));
				if (*a_tail < WLIGHT_QUEUE_SIZE)
					add_queue[(*a_tail)++] = (wlight_node_t){nx, ny, nz, cur_sky, cur_blk};
			}
		}
	}
}

static void remove_bfs(int *r_head, int *r_tail, int *a_head, int *a_tail) {
	while (*r_head < *r_tail) {
		wlight_node_t node = rem_queue[(*r_head)++];

		for (int d = 0; d < 6; d++) {
			int nx = node.x + ddx6[d];
			int ny = node.y + ddy6[d];
			int nz = node.z + ddz6[d];

			uint8_t cur	 = get_light(nx, ny, nz);
			uint8_t cur_sky = SKY_LIGHT(cur);
			uint8_t cur_blk = BLOCK_LIGHT(cur);
			if (cur_sky == 0 && cur_blk == 0) continue;

			uint8_t nid	= get_id(nx, ny, nz);
			uint8_t sky_op = get_sky_opacity(nid);
			uint8_t blk_op = get_block_opacity(nid);
			if (sky_op == 15) continue;

			bool rem_sky = false, rem_blk = false;

			if (cur_sky > 0 && node.sky > 0) {
				uint8_t expected = node.sky > (1 + sky_op) ? node.sky - 1 - sky_op : 0;
				if (cur_sky <= expected) rem_sky = true;
			}
			if (cur_blk > 0 && node.blk > 0) {
				uint8_t expected = node.blk > (1 + blk_op) ? node.blk - 1 - blk_op : 0;
				if (cur_blk <= expected) rem_blk = true;
			}

			if (!rem_sky && !rem_blk) {
				if (*a_tail < WLIGHT_QUEUE_SIZE)
					add_queue[(*a_tail)++] = (wlight_node_t){nx, ny, nz, cur_sky, cur_blk};
				continue;
			}

			uint8_t new_sky = rem_sky ? 0 : cur_sky;
			uint8_t new_blk = rem_blk ? 0 : cur_blk;

			set_light(nx, ny, nz, PACK_LIGHT(new_sky, new_blk));

			if (*r_tail < WLIGHT_QUEUE_SIZE)
				rem_queue[(*r_tail)++] = (wlight_node_t){nx, ny, nz,
					rem_sky ? cur_sky : 0,
					rem_blk ? cur_blk : 0};

			uint8_t emit = get_block_emission(nid);
			uint8_t reseed_sky = rem_sky ? 0 : cur_sky;
			uint8_t reseed_blk = rem_blk ? 0 : cur_blk;
			if (emit > reseed_blk) reseed_blk = emit;
			if ((reseed_sky > 0 || reseed_blk > 0) && *a_tail < WLIGHT_QUEUE_SIZE)
				add_queue[(*a_tail)++] = (wlight_node_t){nx, ny, nz, reseed_sky, reseed_blk};
		}
	}
}

static void seed_sky_column_down(int wx, int wy, int wz, uint8_t sky_in,
								 int *a_head, int *a_tail) {
	(void)a_head;
	for (int y = wy; y >= 0; y--) {
		uint8_t id  = get_id(wx, y, wz);
		uint8_t op  = get_sky_opacity(id);
		if (op == 15) break;
		if (op > 0) {
			if (sky_in <= op) { sky_in = 0; break; }
			sky_in -= op;
		}
		uint8_t cur_sky = SKY_LIGHT(get_light(wx, y, wz));
		if (sky_in > cur_sky) {
			uint8_t cur_blk = BLOCK_LIGHT(get_light(wx, y, wz));
			set_light(wx, y, wz, PACK_LIGHT(sky_in, cur_blk));
			if (*a_tail < WLIGHT_QUEUE_SIZE)
				add_queue[(*a_tail)++] = (wlight_node_t){wx, y, wz, sky_in, cur_blk};
		}
	}
}

static void remove_sky_column_down(int wx, int wy, int wz,
								   int *r_head, int *r_tail,
								   int *a_head, int *a_tail) {
	(void)r_head; (void)a_head;
	for (int y = wy; y >= 0; y--) {
		uint8_t id  = get_id(wx, y, wz);
		uint8_t op  = get_sky_opacity(id);
		if (op == 15) break;
		uint8_t cur_light = get_light(wx, y, wz);
		uint8_t cur_sky   = SKY_LIGHT(cur_light);
		uint8_t cur_blk   = BLOCK_LIGHT(cur_light);
		if (cur_sky == 0) break;
		set_light(wx, y, wz, PACK_LIGHT(0, cur_blk));
		if (*r_tail < WLIGHT_QUEUE_SIZE)
			rem_queue[(*r_tail)++] = (wlight_node_t){wx, y, wz, cur_sky, 0};
	}
}

static uint8_t sky_above(int wx, int wy, int wz) {
	uint8_t sky = MAX_LIGHT_LEVEL;
	for (int y = wy + 1; y < WORLD_HEIGHT * CHUNK_SIZE; y++) {
		uint8_t op = get_sky_opacity(get_id(wx, y, wz));
		if (op == 15) return 0;
		if (op > 0) {
			if (sky <= op) return 0;
			sky -= op;
		}
	}
	return sky;
}

void update_block_lighting(int wx, int wy, int wz, uint8_t old_id, uint8_t new_id) {
	int a_head = 0, a_tail = 0;
	int r_head = 0, r_tail = 0;

	uint8_t old_emit   = get_block_emission(old_id);
	uint8_t new_emit   = get_block_emission(new_id);
	uint8_t old_sky_op = get_sky_opacity(old_id);
	uint8_t new_sky_op = get_sky_opacity(new_id);

	if (old_emit > 0) {
		uint8_t cur_blk = BLOCK_LIGHT(get_light(wx, wy, wz));
		if (cur_blk > 0) {
			set_light(wx, wy, wz, PACK_LIGHT(SKY_LIGHT(get_light(wx, wy, wz)), 0));
			rem_queue[r_tail++] = (wlight_node_t){wx, wy, wz, 0, cur_blk};
			remove_bfs(&r_head, &r_tail, &a_head, &a_tail);
		}
	}

	if (old_sky_op != new_sky_op) {
		if (new_sky_op > old_sky_op) {
			uint8_t cur = get_light(wx, wy, wz);
			if (SKY_LIGHT(cur) > 0) {
				set_light(wx, wy, wz, PACK_LIGHT(0, BLOCK_LIGHT(cur)));
				rem_queue[r_tail++] = (wlight_node_t){wx, wy, wz, SKY_LIGHT(cur), 0};
			}
			remove_sky_column_down(wx, wy - 1, wz, &r_head, &r_tail, &a_head, &a_tail);
			remove_bfs(&r_head, &r_tail, &a_head, &a_tail);
		} else {
			uint8_t sky_in = sky_above(wx, wy, wz);
			if (sky_in > 0) {
				seed_sky_column_down(wx, wy, wz, sky_in, &a_head, &a_tail);
			}
			for (int d = 0; d < 6; d++) {
				int nx = wx + ddx6[d];
				int ny = wy + ddy6[d];
				int nz = wz + ddz6[d];
				uint8_t nb_sky = SKY_LIGHT(get_light(nx, ny, nz));
				uint8_t nb_blk = BLOCK_LIGHT(get_light(nx, ny, nz));
				if ((nb_sky > 0 || nb_blk > 0) && a_tail < WLIGHT_QUEUE_SIZE)
					add_queue[a_tail++] = (wlight_node_t){nx, ny, nz, nb_sky, nb_blk};
			}
		}
	} else if (new_sky_op == 15) {
		uint8_t cur = get_light(wx, wy, wz);
		if (cur > 0) {
			set_light(wx, wy, wz, 0);
			rem_queue[r_tail++] = (wlight_node_t){wx, wy, wz, SKY_LIGHT(cur), BLOCK_LIGHT(cur)};
			remove_bfs(&r_head, &r_tail, &a_head, &a_tail);
		}
	}

	if (new_emit > 0) {
		uint8_t cur_sky = SKY_LIGHT(get_light(wx, wy, wz));
		set_light(wx, wy, wz, PACK_LIGHT(cur_sky, new_emit));
		if (a_tail < WLIGHT_QUEUE_SIZE)
			add_queue[a_tail++] = (wlight_node_t){wx, wy, wz, cur_sky, new_emit};
	}

	add_bfs(&a_head, &a_tail);
}

typedef struct {
	int16_t x, y, z;
	uint8_t sky;
	uint8_t blk;
} local_node_t;

#define LOCAL_QUEUE_SIZE (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * 4)
static local_node_t local_queue[LOCAL_QUEUE_SIZE];

void init_chunk_lighting(Chunk* chunk) {
	for (int x = 0; x < CHUNK_SIZE; x++)
		for (int y = 0; y < CHUNK_SIZE; y++)
			for (int z = 0; z < CHUNK_SIZE; z++)
				chunk->blocks[x][y][z].light_level = 0;

	int q_head = 0, q_tail = 0;

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int z = 0; z < CHUNK_SIZE; z++) {
			uint8_t sky = MAX_LIGHT_LEVEL;
			for (int cy = WORLD_HEIGHT - 1; cy > chunk->ci_y; cy--) {
				Chunk *above = &chunks[chunk->ci_x][cy][chunk->ci_z];
				if (!above->is_loaded) { sky = MAX_LIGHT_LEVEL; break; }
				for (int by = CHUNK_SIZE - 1; by >= 0; by--) {
					uint8_t op = get_sky_opacity(above->blocks[x][by][z].id);
					if (op == 15) { sky = 0; goto next_column; }
					if (op > 0) {
						if (sky <= op) { sky = 0; goto next_column; }
						sky -= op;
					}
				}
			}

			if (sky > 0) {
				for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
					Block *b = &chunk->blocks[x][y][z];
					uint8_t op = get_sky_opacity(b->id);
					if (op == 15) break;
					if (op > 0) {
						if (sky <= op) { sky = 0; break; }
						sky -= op;
					}
					if (sky > SKY_LIGHT(b->light_level)) {
						b->light_level = PACK_LIGHT(sky, BLOCK_LIGHT(b->light_level));
						if (q_tail < LOCAL_QUEUE_SIZE)
							local_queue[q_tail++] = (local_node_t){x, y, z, sky, BLOCK_LIGHT(b->light_level)};
					}
				}
			}
			next_column:;
		}
	}

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				uint8_t emit = get_block_emission(chunk->blocks[x][y][z].id);
				if (emit == 0) continue;
				if (emit > BLOCK_LIGHT(chunk->blocks[x][y][z].light_level)) {
					chunk->blocks[x][y][z].light_level =
						PACK_LIGHT(SKY_LIGHT(chunk->blocks[x][y][z].light_level), emit);
					if (q_tail < LOCAL_QUEUE_SIZE)
						local_queue[q_tail++] = (local_node_t){x, y, z,
							SKY_LIGHT(chunk->blocks[x][y][z].light_level), emit};
				}
			}
		}
	}

	static const int8_t dcx[] = { 1,-1, 0, 0, 0, 0 };
	static const int8_t dcy[] = { 0, 0, 1,-1, 0, 0 };
	static const int8_t dcz[] = { 0, 0, 0, 0, 1,-1 };

	for (int dir = 0; dir < 6; dir++) {
		int ncx = (int)chunk->ci_x + dcx[dir];
		int ncy = (int)chunk->ci_y + dcy[dir];
		int ncz = (int)chunk->ci_z + dcz[dir];
		if (ncx < 0 || ncx >= settings.render_distance) continue;
		if (ncy < 0 || ncy >= WORLD_HEIGHT) continue;
		if (ncz < 0 || ncz >= settings.render_distance) continue;
		Chunk *nb = &chunks[ncx][ncy][ncz];
		if (!nb->is_loaded) continue;

		for (int a = 0; a < CHUNK_SIZE; a++) {
			for (int b = 0; b < CHUNK_SIZE; b++) {
				int lx, ly, lz, nx, ny, nz;
				switch (dir) {
					case 0: lx=CHUNK_SIZE-1; ly=a; lz=b; nx=0;		   ny=a; nz=b; break;
					case 1: lx=0;			ly=a; lz=b; nx=CHUNK_SIZE-1; ny=a; nz=b; break;
					case 2: lx=a; ly=CHUNK_SIZE-1; lz=b; nx=a; ny=0;		   nz=b; break;
					case 3: lx=a; ly=0;			lz=b; nx=a; ny=CHUNK_SIZE-1; nz=b; break;
					case 4: lx=a; ly=b; lz=CHUNK_SIZE-1; nx=a; ny=b; nz=0;		   break;
					case 5: lx=a; ly=b; lz=0;			nx=a; ny=b; nz=CHUNK_SIZE-1; break;
					default: continue;
				}

				Block *border = &chunk->blocks[lx][ly][lz];
				uint8_t sky_op = get_sky_opacity(border->id);
				if (sky_op == 15) continue;
				uint8_t blk_op = get_block_opacity(border->id);

				uint8_t nl	 = nb->blocks[nx][ny][nz].light_level;
				uint8_t nb_sky = SKY_LIGHT(nl);
				uint8_t nb_blk = BLOCK_LIGHT(nl);

				uint8_t in_sky = nb_sky > (1 + sky_op) ? nb_sky - 1 - sky_op : 0;
				uint8_t in_blk = nb_blk > (1 + blk_op) ? nb_blk - 1 - blk_op : 0;
				if (in_sky == 0 && in_blk == 0) continue;

				uint8_t cur_sky = SKY_LIGHT(border->light_level);
				uint8_t cur_blk = BLOCK_LIGHT(border->light_level);
				bool changed = false;
				if (in_sky > cur_sky) { cur_sky = in_sky; changed = true; }
				if (in_blk > cur_blk) { cur_blk = in_blk; changed = true; }
				if (changed) {
					border->light_level = PACK_LIGHT(cur_sky, cur_blk);
					if (q_tail < LOCAL_QUEUE_SIZE)
						local_queue[q_tail++] = (local_node_t){lx, ly, lz, cur_sky, cur_blk};
				}
			}
		}
	}

	while (q_head < q_tail) {
		local_node_t node = local_queue[q_head++];

		for (int d = 0; d < 6; d++) {
			int nx = node.x + ddx6[d];
			int ny = node.y + ddy6[d];
			int nz = node.z + ddz6[d];

			if (nx < 0 || nx >= CHUNK_SIZE ||
				ny < 0 || ny >= CHUNK_SIZE ||
				nz < 0 || nz >= CHUNK_SIZE) continue;

			Block *nb = &chunk->blocks[nx][ny][nz];
			uint8_t sky_op = get_sky_opacity(nb->id);
			uint8_t blk_op = get_block_opacity(nb->id);
			if (sky_op == 15) continue;

			uint8_t cur_sky = SKY_LIGHT(nb->light_level);
			uint8_t cur_blk = BLOCK_LIGHT(nb->light_level);
			bool changed = false;

			uint8_t next_sky = node.sky > (1 + sky_op) ? node.sky - 1 - sky_op : 0;
			uint8_t next_blk = node.blk > (1 + blk_op) ? node.blk - 1 - blk_op : 0;

			if (next_sky > cur_sky) { cur_sky = next_sky; changed = true; }
			if (next_blk > cur_blk) { cur_blk = next_blk; changed = true; }

			if (changed) {
				nb->light_level = PACK_LIGHT(cur_sky, cur_blk);
				if (q_tail < LOCAL_QUEUE_SIZE)
					local_queue[q_tail++] = (local_node_t){nx, ny, nz, cur_sky, cur_blk};
			}
		}
	}
}

unsigned char* generate_light_texture() {
	return NULL;
}