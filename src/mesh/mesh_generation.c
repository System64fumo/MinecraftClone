#include "main.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>

static const face_vertex_t cube_faces[6][4] = {
	{{{1,1,1},{1,0}},{{0,1,1},{0,0}},{{0,0,1},{0,1}},{{1,0,1},{1,1}}},
	{{{1,1,0},{1,0}},{{1,1,1},{0,0}},{{1,0,1},{0,1}},{{1,0,0},{1,1}}},
	{{{0,1,0},{1,0}},{{1,1,0},{0,0}},{{1,0,0},{0,1}},{{0,0,0},{1,1}}},
	{{{0,1,1},{1,0}},{{0,1,0},{0,0}},{{0,0,0},{0,1}},{{0,0,1},{1,1}}},
	{{{0,0,0},{0,1}},{{1,0,0},{1,1}},{{1,0,1},{1,0}},{{0,0,1},{0,0}}},
	{{{0,1,1},{0,0}},{{1,1,1},{1,0}},{{1,1,0},{1,1}},{{0,1,0},{0,1}}}
};

static const face_vertex_t slab_faces[6][4] = {
	{{{1,.5f,1},{1,0}},{{0,.5f,1},{0,0}},{{0,0,1},{0,.5f}},{{1,0,1},{1,.5f}}},
	{{{1,.5f,0},{1,0}},{{1,.5f,1},{0,0}},{{1,0,1},{0,.5f}},{{1,0,0},{1,.5f}}},
	{{{0,.5f,0},{1,0}},{{1,.5f,0},{0,0}},{{1,0,0},{0,.5f}},{{0,0,0},{1,.5f}}},
	{{{0,.5f,1},{1,0}},{{0,.5f,0},{0,0}},{{0,0,0},{0,.5f}},{{0,0,1},{1,.5f}}},
	{{{0,0,0},{0,1}},{{1,0,0},{1,1}},{{1,0,1},{1,0}},{{0,0,1},{0,0}}},
	{{{0,.5f,1},{0,0}},{{1,.5f,1},{1,0}},{{1,.5f,0},{1,1}},{{0,.5f,0},{0,1}}}
};

static const face_vertex_t cross_faces[4][4] = {
	{{{1,1,1},{1,0}},{{0,1,0},{0,0}},{{0,0,0},{0,1}},{{1,0,1},{1,1}}},
	{{{0,1,1},{1,0}},{{1,1,0},{0,0}},{{1,0,0},{0,1}},{{0,0,1},{1,1}}},
	{{{0,1,0},{1,0}},{{1,1,1},{0,0}},{{1,0,1},{0,1}},{{0,0,0},{1,1}}},
	{{{1,1,0},{1,0}},{{0,1,1},{0,0}},{{0,0,1},{0,1}},{{1,0,0},{1,1}}}
};

static const uint32_t quad_indices[6] = {0, 1, 2, 0, 2, 3};

void add_quad(Chunk *chunk, float x, float y, float z, uint8_t normal, uint8_t texture_id,
              const face_vertex_t face_data[4], uint8_t width, uint8_t height,
              uint8_t sky_light, uint8_t block_light,
              Vertex vertices[], uint32_t indices[],
              uint32_t *vertex_count, uint32_t *index_count) {
	uint32_t base = *vertex_count;

	float wb = (normal == 1 || normal == 3) ? 1.0f : (float)width;
	float hb = (normal >= 4)               ? 1.0f : (float)height;
	float db = (normal == 0 || normal == 2) ? 1.0f : (normal >= 4 ? (float)height : (float)width);

	uint8_t block_id = 0;
	if (chunk) {
		uint8_t lx = (uint8_t)(x - chunk->x * CHUNK_SIZE);
		uint8_t ly = (uint8_t)(y - chunk->y * CHUNK_SIZE);
		uint8_t lz = (uint8_t)(z - chunk->z * CHUNK_SIZE);
		if (lx < CHUNK_SIZE && ly < CHUNK_SIZE && lz < CHUNK_SIZE)
			block_id = chunk->blocks[lx][ly][lz].id;
	}

	bool    is_liquid        = block_data[block_id][0] == BTYPE_LIQUID;
	float   liquid_adj       = is_liquid ? 0.125f : 0.0f;

	for (int i = 0; i < 4; i++) {
		float px = x + face_data[i].pos[0];
		float py = y + face_data[i].pos[1];
		float pz = z + face_data[i].pos[2];
		float uu = face_data[i].uv[0];
		float uv = face_data[i].uv[1];

		if (width > 1 || height > 1) {
			switch (normal) {
				case 0: case 2:
					if (face_data[i].pos[0] == 1.0f) px = x + width;
					if (face_data[i].pos[1] == 1.0f) py = y + height;
					uu *= wb; uv *= hb;
					break;
				case 1: case 3:
					if (face_data[i].pos[1] == 1.0f) py = y + height;
					if (face_data[i].pos[2] == 1.0f) pz = z + width;
					uu *= db; uv *= hb;
					break;
				case 4: case 5:
					if (face_data[i].pos[0] == 1.0f) px = x + width;
					if (face_data[i].pos[2] == 1.0f) pz = z + height;
					uu *= wb; uv *= db;
					break;
			}
		}

		if (is_liquid) {
			if (normal == 5)
				py -= liquid_adj;
			else if (normal != 4 && face_data[i].pos[1] == 1.0f)
				py -= liquid_adj;
		}

		vertices[(*vertex_count)++] = (Vertex){
			(int32_t)(px * 16.0f),
			(uint16_t)(py * 16.0f),
			(int32_t)(pz * 16.0f),
			PACK_VERTEX_DATA(normal, texture_id),
			(uint32_t)((uint32_t)(uu * 16)        & 0x1FF)
			| (uint32_t)(((uint32_t)(uv * 16)     & 0x1FF) << 9)
			| (uint32_t)((sky_light   & 0xF)               << 18)
			| (uint32_t)((block_light & 0xF)               << 22)
		};
	}

	for (int i = 0; i < 6; i++)
		indices[(*index_count)++] = base + quad_indices[i];
}

void clear_face_data(Mesh faces[6]) {
	for (int f = 0; f < 6; f++) {
		free(faces[f].vertices); faces[f].vertices = NULL;
		free(faces[f].indices);  faces[f].indices  = NULL;
		faces[f].vertex_count = 0;
		faces[f].index_count  = 0;
	}
}

void store_face_data(Mesh *m, Vertex *verts, uint32_t *idxs,
                     uint32_t vc, uint32_t ic) {
	if (vc == 0) return;
	Vertex   *vb = malloc(vc * sizeof(Vertex));
	uint32_t *ib = malloc(ic * sizeof(uint32_t));
	if (!vb || !ib) { free(vb); free(ib); return; }
	memcpy(vb, verts, vc * sizeof(Vertex));
	memcpy(ib, idxs,  ic * sizeof(uint32_t));
	m->vertices    = vb;
	m->indices     = ib;
	m->vertex_count= vc;
	m->index_count = ic;
}

void generate_single_block_mesh(float x, float y, float z, uint8_t block_id, Mesh faces[6]) {
	clear_face_data(faces);
	uint8_t bt = block_data[block_id][0];
	if (bt == BTYPE_REGULAR || bt == BTYPE_SLAB || bt == BTYPE_LIQUID || bt == BTYPE_LEAF) {
		const face_vertex_t (*fd)[4] = (bt == BTYPE_SLAB) ? slab_faces : cube_faces;
		for (int f = 0; f < 6; f++) {
			Vertex v[4]; uint32_t idx[6];
			uint32_t vc = 0, ic = 0;
			add_quad(NULL, x, y, z, f, block_data[block_id][2+f], fd[f], 1, 1,
			         15, 0, v, idx, &vc, &ic);
			store_face_data(&faces[f], v, idx, vc, ic);
		}
	} else if (bt == BTYPE_CROSS) {
		for (int f = 0; f < 4; f++) {
			Vertex v[4]; uint32_t idx[6];
			uint32_t vc = 0, ic = 0;
			add_quad(NULL, x, y, z, f, block_data[block_id][2+f], cross_faces[f], 1, 1,
			         15, 0, v, idx, &vc, &ic);
			store_face_data(&faces[f], v, idx, vc, ic);
		}
	}
}

static bool append_quad_to_mesh(Mesh *m, Chunk *chunk,
                                 float wx, float wy, float wz,
                                 uint8_t face, uint8_t texture_id,
                                 const face_vertex_t *fd,
                                 uint8_t sky_light, uint8_t block_light) {
	uint32_t bv = m->vertex_count;
	uint32_t bi = m->index_count;
	Vertex   *nv = realloc(m->vertices, (bv + 4) * sizeof(Vertex));
	uint32_t *ni = realloc(m->indices,  (bi + 6) * sizeof(uint32_t));
	if (!nv || !ni) {
		// On partial failure, restore whichever pointer realloc didn't touch.
		if (nv) m->vertices = nv;
		if (ni) m->indices  = ni;
		return false;
	}
	m->vertices = nv;
	m->indices  = ni;

	Vertex   tv[4];
	uint32_t ti[6];
	uint32_t vc = 0, ic = 0;
	add_quad(chunk, wx, wy, wz, face, texture_id, fd, 1, 1,
	         sky_light, block_light, tv, ti, &vc, &ic);
	memcpy(m->vertices + bv, tv, 4 * sizeof(Vertex));
	for (int i = 0; i < 6; i++) m->indices[bi + i] = ti[i] + bv;
	m->vertex_count += 4;
	m->index_count  += 6;
	return true;
}

void generate_chunk_mesh(Chunk *chunk) {
	if (!chunk) return;

	clear_face_data(chunk->faces);
	clear_face_data(chunk->transparent_faces);

	float wx0 = chunk->x * CHUNK_SIZE;
	float wy0 = chunk->y * CHUNK_SIZE;
	float wz0 = chunk->z * CHUNK_SIZE;

	bool mask[CHUNK_SIZE][CHUNK_SIZE];

	Vertex   *fv  = malloc((MAX_VERTICES / 6) * sizeof(Vertex));
	uint32_t *fi  = malloc((MAX_VERTICES / 6) * sizeof(uint32_t));
	Vertex   *tv  = malloc((MAX_VERTICES / 6) * sizeof(Vertex));
	uint32_t *ti  = malloc((MAX_VERTICES / 6) * sizeof(uint32_t));
	if (!fv || !fi || !tv || !ti) {
		free(fv); free(fi); free(tv); free(ti);
		return;
	}

	for (int face = 0; face < 6; face++) {
		uint32_t fvc = 0, fic = 0, tvc = 0, tic = 0;

		for (int d = 0; d < CHUNK_SIZE; d++) {
			memset(mask, 0, sizeof(mask));

			for (int v = 0; v < CHUNK_SIZE; v++) {
				for (int u = 0; u < CHUNK_SIZE; u++) {
					if (mask[v][u]) continue;

					uint8_t x, y, z;
					map_coordinates(face, u, v, d, &x, &y, &z);
					if (x >= CHUNK_SIZE || y >= CHUNK_SIZE || z >= CHUNK_SIZE) continue;

					Block *blk = &chunk->blocks[x][y][z];
					if (!blk->id) continue;
					uint8_t bt = block_data[blk->id][0];
					if (bt != BTYPE_REGULAR && bt != BTYPE_LIQUID && bt != BTYPE_LEAF) continue;
					if (!(bt == BTYPE_LEAF && settings.fancy_graphics) &&
					    !is_face_visible(chunk, x, y, z, face)) continue;

					uint8_t w = find_width(chunk,  face, u, v, x, y, z, mask, blk);
					uint8_t h = find_height(chunk, face, u, v, x, y, z, mask, blk, w);

					for (int dy = 0; dy < h && v + dy < CHUNK_SIZE; dy++) {
						int end = u + w < CHUNK_SIZE ? u + w : CHUNK_SIZE;
						memset(&mask[v + dy][u], true, (end - u) * sizeof(bool));
					}

					uint8_t pl  = get_face_light(chunk, x, y, z, face);
					uint8_t sl  = (pl >> 4) & 0xF;
					uint8_t bl2 = pl & 0xF;
					uint8_t tid = block_data[blk->id][2 + face];
					bool    trans = block_data[blk->id][1] != 0;

					if (trans)
						add_quad(chunk, x + wx0, y + wy0, z + wz0, face, tid,
						         cube_faces[face], w, h, sl, bl2, tv, ti, &tvc, &tic);
					else
						add_quad(chunk, x + wx0, y + wy0, z + wz0, face, tid,
						         cube_faces[face], w, h, sl, bl2, fv, fi, &fvc, &fic);
				}
			}
		}

		store_face_data(&chunk->faces[face],             fv, fi, fvc, fic);
		store_face_data(&chunk->transparent_faces[face], tv, ti, tvc, tic);
		fvc = fic = tvc = tic = 0;
	}

	for (int x = 0; x < CHUNK_SIZE; x++) {
		for (int y = 0; y < CHUNK_SIZE; y++) {
			for (int z = 0; z < CHUNK_SIZE; z++) {
				Block *blk = &chunk->blocks[x][y][z];
				if (!blk->id) continue;
				uint8_t bt = block_data[blk->id][0];
				if (bt != BTYPE_SLAB && bt != BTYPE_CROSS) continue;

				bool   trans       = block_data[blk->id][1] != 0;
				Mesh  *tgt         = trans ? chunk->transparent_faces : chunk->faces;
				int    face_count  = (bt == BTYPE_CROSS) ? 4 : 6;
				uint8_t pl  = blk->light_level;
				uint8_t sl  = (pl >> 4) & 0xF;
				uint8_t bl2 = pl & 0xF;

				for (int f = 0; f < face_count; f++) {
					const face_vertex_t *fd = (bt == BTYPE_CROSS) ? cross_faces[f]
					                        : (bt == BTYPE_SLAB)  ? slab_faces[f]
					                                               : cube_faces[f];
					append_quad_to_mesh(&tgt[f], chunk,
					                    x + wx0, y + wy0, z + wz0,
					                    f, block_data[blk->id][2 + f], fd, sl, bl2);
				}
			}
		}
	}

	free(fv); free(fi); free(tv); free(ti);
	chunk->needs_update = false;
}
