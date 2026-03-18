#include "main.h"
#include "world.h"
#include "stb_perlin.h"
#include <math.h>

void generate_chunk_terrain(Chunk *chunk, int chunk_x, int chunk_y, int chunk_z) {
	int  base_y    = chunk_y * CHUNK_SIZE;
	int  world_x0  = chunk_x * CHUNK_SIZE;
	int  world_z0  = chunk_z * CHUNK_SIZE;
	bool empty     = true;

	if (flat_world_gen) {
		for (int x = 0; x < CHUNK_SIZE; x++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				for (int y = 0; y < CHUNK_SIZE; y++) {
					int ay = base_y + y;
					uint8_t id = 0;
					if      (ay == 0)           id = 7;
					else if (ay == 4)           id = 2;
					else if (ay >= 4 - 2 && ay < 4) id = 1;
					else if (ay > 0 && ay < 4 - 2)  id = 3;
					chunk->blocks[x][y][z] = (Block){.id = id};
					if (id) empty = false;
				}
			}
		}
		if (empty) chunk->needs_update = false;
		return;
	}

	static const int MAX_TERRAIN_HEIGHT = (int)(64 + 64 + 4 * CHUNK_SIZE);
	if (base_y > MAX_TERRAIN_HEIGHT) {
		chunk->needs_update = false;
		return;
	}

	float cont[CHUNK_SIZE][CHUNK_SIZE];
	float flat[CHUNK_SIZE][CHUNK_SIZE];
	float mnt [CHUNK_SIZE][CHUNK_SIZE];
	float hmap[CHUNK_SIZE][CHUNK_SIZE];
	bool  beach[CHUNK_SIZE][CHUNK_SIZE];
	bool  ocean[CHUNK_SIZE][CHUNK_SIZE];

	for (int x = 0; x < CHUNK_SIZE; x++) {
		float wx = world_x0 + x;
		for (int z = 0; z < CHUNK_SIZE; z++) {
			float wz = world_z0 + z;
			cont[x][z] = (stb_perlin_noise3(wx * continent_scale, 0.f, wz * continent_scale, 0,0,0) + 1.f) * 0.5f;
			float ch   = (cont[x][z] - 0.5f) * 128.f;
			flat[x][z] = powf((stb_perlin_noise3(wx * flatness_scale, 0.f, wz * flatness_scale, 0,0,0) + 1.f) * 0.5f, 2.f) * 20.f;
			mnt [x][z] = powf((stb_perlin_noise3(wx * mountain_scale, 0.f, wz * mountain_scale, 0,0,0) + 1.f) * 0.5f, 2.5f) * 64.f;
			hmap[x][z] = (ch + mnt[x][z] - flat[x][z]) + (4 * CHUNK_SIZE);
			int h      = (int)hmap[x][z];
			ocean[x][z]= h < SEA_LEVEL;
			beach[x][z]= h >= SEA_LEVEL - 3 && h <= SEA_LEVEL + 2;
		}
	}

	for (int x = 0; x < CHUNK_SIZE; x++) {
		float wx = world_x0 + x;
		for (int z = 0; z < CHUNK_SIZE; z++) {
			float wz   = world_z0 + z;
			int   h    = (int)hmap[x][z];
			bool  isb  = beach[x][z];
			bool  iso  = ocean[x][z];
			for (int y = 0; y < CHUNK_SIZE; y++) {
				int ay = base_y + y;
				bool cave = false;
				if (ay < h - 5) {
					float cv = stb_perlin_fbm_noise3(wx * cave_scale, ay * cave_scale, wz * cave_scale, 2.f, 0.5f, 2);
					float df = powf(1.f - (float)ay / (float)h, 3.f);
					cave     = cv > 0.3f - 0.15f * df;
				}
				uint8_t id = 0;
				if (ay == 0) {
					id = 7;
				} else if (ay > h) {
					id = ay <= SEA_LEVEL ? 9 : 0;
				} else if (cave) {
					id = 0;
				} else if (ay == h) {
					id = (isb || iso) ? 12 : 2;
				} else if (ay >= h - 3) {
					id = (isb || (iso && ay >= SEA_LEVEL - 3)) ? 12 : 1;
				} else {
					id = 3;
				}
				chunk->blocks[x][y][z] = (Block){.id = id};
				if (id) empty = false;
			}
		}
	}

	int grid  = 8;
	int sx_min = chunk_x * CHUNK_SIZE - tree_structure.width  / 2;
	int sx_max = chunk_x * CHUNK_SIZE + CHUNK_SIZE - 1 + tree_structure.width  / 2;
	int sz_min = chunk_z * CHUNK_SIZE - tree_structure.depth  / 2;
	int sz_max = chunk_z * CHUNK_SIZE + CHUNK_SIZE - 1 + tree_structure.depth  / 2;
	int gx0    = (int)floorf((float)sx_min / grid) * grid;
	int gz0    = (int)floorf((float)sz_min / grid) * grid;

	for (int swx = gx0; swx <= sx_max; swx += grid) {
		for (int swz = gz0; swz <= sz_max; swz += grid) {
			float cn = (stb_perlin_noise3(swx * continent_scale, 0.f, swz * continent_scale, 0,0,0) + 1.f) * 0.5f;
			float ch = (cn - 0.5f) * 128.f;
			float fn = powf((stb_perlin_noise3(swx * flatness_scale, 0.f, swz * flatness_scale, 0,0,0) + 1.f) * 0.5f, 2.f) * 20.f;
			float mn = powf((stb_perlin_noise3(swx * mountain_scale, 0.f, swz * mountain_scale, 0,0,0) + 1.f) * 0.5f, 2.5f) * 64.f;
			int   sy = (int)((ch + mn - fn) + 4 * CHUNK_SIZE);
			bool  is_grass = sy > SEA_LEVEL && sy < SEA_LEVEL + 50;
			if (can_place_tree(swx, sy, swz, is_grass))
				generate_structure_in_chunk(chunk, chunk_x, chunk_y, chunk_z,
				                            &tree_structure, swx, sy + 1, swz, &empty);
		}
	}

	if (empty) chunk->needs_update = false;
}
