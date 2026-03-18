#include "main.h"
#include "entity.h"
#include "world.h"
#include "config.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

bool frustum_changed = false;

#define MAX_RENDER_DISTANCE_SQ \
	((settings.render_distance * CHUNK_SIZE * 0.8f) * (settings.render_distance * CHUNK_SIZE * 0.8f))
#define CLOSE_RENDER_DISTANCE_SQ  ((CHUNK_SIZE * 1.0f)  * (CHUNK_SIZE * 1.0f))
#define MIN_OCCLUSION_DISTANCE_SQ ((CHUNK_SIZE * 2.5f)  * (CHUNK_SIZE * 2.5f))
#define MAX_STEP_SIZE             1.0f
#define MIN_STEP_SIZE             0.25f
#define VISIBILITY_THRESHOLD      0.05f
#define EDGE_VISIBILITY_THRESHOLD 0.01f
#define MAX_TEST_POINTS           24
#define SAMPLE_BLOCK_THRESHOLD    4

static inline vec3 v3sub(vec3 a, vec3 b)  { return (vec3){a.x-b.x, a.y-b.y, a.z-b.z}; }
static inline float v3dsq(vec3 a, vec3 b) { vec3 d = v3sub(a,b); return d.x*d.x+d.y*d.y+d.z*d.z; }
static inline float v3dot(vec3 a, vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float v3len(vec3 v)         { return sqrtf(v.x*v.x+v.y*v.y+v.z*v.z); }
static inline vec3  v3norm(vec3 v)        {
	float l = v3len(v);
	return l < 0.001f ? (vec3){0} : (vec3){v.x/l, v.y/l, v.z/l};
}

static inline vec3 chunk_center(int x, int y, int z) {
	return (vec3){
		x * CHUNK_SIZE + CHUNK_SIZE * 0.5f,
		y * CHUNK_SIZE + CHUNK_SIZE * 0.5f,
		z * CHUNK_SIZE + CHUNK_SIZE * 0.5f
	};
}

static bool is_ray_obstructed(vec3 start, vec3 end) {
	vec3  dir   = v3sub(end, start);
	float total = v3len(dir);
	if (total < 0.001f) return false;
	dir = v3norm(dir);
	float step = fminf(MAX_STEP_SIZE, fmaxf(MIN_STEP_SIZE, total * 0.01f));
	int   max  = (int)(total / step) + 1;
	int   solid = 0, air = 0;
	for (int i = 1; i < max; i++) {
		float t = (float)i * step;
		if (t >= total * 0.95f) break;
		Block *b = get_block_at(chunks,
		    start.x + dir.x * t,
		    start.y + dir.y * t,
		    start.z + dir.z * t);
		if (!b) return false;
		if (block_data[b->id][1] == 0) {
			if (++solid >= 2) return true;
			air = 0;
		} else if (++air > 5) {
			solid = 0;
		}
	}
	return false;
}

static bool chunk_has_surface_blocks(int cx, int cy, int cz) {
	int lx = cx - world_offset_x;
	int lz = cz - world_offset_z;
	if (lx < 0 || lx >= settings.render_distance ||
	    lz < 0 || lz >= settings.render_distance ||
	    cy < 0 || cy >= WORLD_HEIGHT) return false;
	if (!chunks[lx][cy][lz].is_loaded) return false;
	Chunk *c = &chunks[lx][cy][lz];
	static const int sp[][3] = {
		{0,0,0},{CHUNK_SIZE-1,0,0},{0,0,CHUNK_SIZE-1},{CHUNK_SIZE-1,0,CHUNK_SIZE-1},
		{0,CHUNK_SIZE-1,0},{CHUNK_SIZE-1,CHUNK_SIZE-1,0},{0,CHUNK_SIZE-1,CHUNK_SIZE-1},
		{CHUNK_SIZE-1,CHUNK_SIZE-1,CHUNK_SIZE-1},{CHUNK_SIZE/2,CHUNK_SIZE/2,CHUNK_SIZE/2},
		{0,CHUNK_SIZE/2,CHUNK_SIZE/2},{CHUNK_SIZE-1,CHUNK_SIZE/2,CHUNK_SIZE/2},
		{CHUNK_SIZE/2,0,CHUNK_SIZE/2},{CHUNK_SIZE/2,CHUNK_SIZE-1,CHUNK_SIZE/2},
		{CHUNK_SIZE/2,CHUNK_SIZE/2,0},{CHUNK_SIZE/2,CHUNK_SIZE/2,CHUNK_SIZE-1}
	};
	int n = 0;
	for (int i = 0; i < (int)(sizeof(sp)/sizeof(sp[0])); i++)
		if (c->blocks[sp[i][0]][sp[i][1]][sp[i][2]].id > 0) n++;
	return n >= SAMPLE_BLOCK_THRESHOLD;
}

static int generate_test_points(int cx, int cy, int cz, vec3 vp, vec3 pts[MAX_TEST_POINTS]) {
	vec3  cc  = chunk_center(cx, cy, cz);
	float d   = sqrtf(v3dsq(vp, cc));
	int   n   = 0;
	pts[n++]  = cc;

	float mnx = cx * CHUNK_SIZE, mxx = mnx + CHUNK_SIZE;
	float mny = cy * CHUNK_SIZE, mxy = mny + CHUNK_SIZE;
	float mnz = cz * CHUNK_SIZE, mxz = mnz + CHUNK_SIZE;

	if (d < CHUNK_SIZE * 4) {
		const vec3 corners[] = {
			{mnx,mny,mnz},{mxx,mny,mnz},{mnx,mxy,mnz},{mnx,mny,mxz},
			{mxx,mxy,mnz},{mxx,mny,mxz},{mnx,mxy,mxz},{mxx,mxy,mxz}
		};
		const vec3 fcenters[] = {
			{cc.x,cc.y,mnz},{cc.x,cc.y,mxz},{mnx,cc.y,cc.z},
			{mxx,cc.y,cc.z},{cc.x,mny,cc.z},{cc.x,mxy,cc.z}
		};
		const vec3 edges[] = {
			{mnx,mny,cc.z},{mxx,mny,cc.z},{mnx,mxy,cc.z},{mxx,mxy,cc.z},
			{mnx,cc.y,mnz},{mxx,cc.y,mnz},{mnx,cc.y,mxz},{mxx,cc.y,mxz},
			{cc.x,mny,mnz},{cc.x,mxy,mnz},{cc.x,mny,mxz},{cc.x,mxy,mxz}
		};
		for (int i = 0; i < 8  && n < MAX_TEST_POINTS; i++) pts[n++] = corners[i];
		for (int i = 0; i < 6  && n < MAX_TEST_POINTS; i++) pts[n++] = fcenters[i];
		for (int i = 0; i < 12 && n < MAX_TEST_POINTS; i++) pts[n++] = edges[i];
	} else if (d < CHUNK_SIZE * 8) {
		const vec3 pts12[] = {
			{mnx,mny,mnz},{mxx,mny,mnz},{mnx,mxy,mnz},{mnx,mny,mxz},
			{mxx,mxy,mnz},{mxx,mny,mxz},{mnx,mxy,mxz},{mxx,mxy,mxz},
			{mnx,cc.y,cc.z},{mxx,cc.y,cc.z},{cc.x,mny,cc.z},{cc.x,mxy,cc.z}
		};
		for (int i = 0; i < 12 && n < MAX_TEST_POINTS; i++) pts[n++] = pts12[i];
	} else {
		vec3  tc  = v3norm(v3sub(cc, vp));
		float xp  = tc.x > 0 ? mxx : mnx;
		float yp  = tc.y > 0 ? mxy : mny;
		float zp  = tc.z > 0 ? mxz : mnz;
		const vec3 p4[] = {{xp,yp,zp},{xp,cc.y,cc.z},{cc.x,yp,cc.z},{cc.x,cc.y,zp}};
		for (int i = 0; i < 4 && n < MAX_TEST_POINTS; i++) pts[n++] = p4[i];
	}
	return n;
}

static bool is_chunk_occluded(vec3 vp, int cx, int cy, int cz) {
	vec3  cc    = chunk_center(cx, cy, cz);
	float dsq   = v3dsq(vp, cc);
	if (dsq < MIN_OCCLUSION_DISTANCE_SQ || dsq > MAX_RENDER_DISTANCE_SQ) return false;

	bool  surf  = chunk_has_surface_blocks(cx, cy, cz);
	float thr   = surf ? EDGE_VISIBILITY_THRESHOLD : VISIBILITY_THRESHOLD;

	vec3 pts[MAX_TEST_POINTS];
	int  np  = generate_test_points(cx, cy, cz, vp, pts);
	int  vis = 0;
	for (int i = 0; i < np; i++) {
		if (!is_ray_obstructed(vp, pts[i])) {
			vis++;
			if ((float)vis / (i + 1) > thr && i >= 2) return false;
		}
		if ((float)(vis + (np - i - 1)) / np < thr && i >= 5) return true;
	}
	return (float)vis / np < thr;
}

bool is_chunk_in_frustum(vec3 pos, vec3 dir, int cx, int cy, int cz, float fov_angle) {
	vec3  cc  = chunk_center(cx, cy, cz);
	float dsq = v3dsq(pos, cc);
	if (dsq > MAX_RENDER_DISTANCE_SQ)    return false;
	if (dsq < CLOSE_RENDER_DISTANCE_SQ)  return true;
	float dl = v3len(dir);
	if (dl < 0.001f) return true;
	vec3  nd  = v3norm(dir);
	vec3  tc  = v3norm(v3sub(cc, pos));
	float dot = v3dot(nd, tc);
	float ang = (CHUNK_SIZE * 0.866f) / sqrtf(dsq);
	if (dot < fov_angle - ang) return false;
	if (settings.occlusion_culling) {
		pthread_mutex_lock(&chunks_mutex);
		bool occ = is_chunk_occluded(pos, cx, cy, cz);
		pthread_mutex_unlock(&chunks_mutex);
		return !occ;
	}
	return true;
}

uint8_t get_visible_faces(vec3 pos, vec3 dir, int cx, int cy, int cz, float fov_angle) {
	if (!is_chunk_in_frustum(pos, dir, cx, cy, cz, fov_angle)) return 0;

	uint8_t vf = ALL_FACES;
	if (settings.face_culling) {
		vec3 cmin = {cx * CHUNK_SIZE,            cy * CHUNK_SIZE,            cz * CHUNK_SIZE};
		vec3 cmax = {cmin.x + CHUNK_SIZE, cmin.y + CHUNK_SIZE, cmin.z + CHUNK_SIZE};
		if (pos.x > cmax.x + 0.25f) vf &= ~FACE_RIGHT;
		if (pos.x < cmin.x - 0.25f) vf &= ~FACE_LEFT;
		if (pos.y > cmax.y + 0.25f) vf &= ~FACE_TOP;
		if (pos.y < cmin.y - 0.25f) vf &= ~FACE_BOTTOM;
		if (pos.z > cmax.z + 0.25f) vf &= ~FACE_FRONT;
		if (pos.z < cmin.z - 0.25f) vf &= ~FACE_BACK;
		float dl = v3len(dir);
		if (dl > 0.001f) {
			vec3 nd = v3norm(dir);
			const float thr = 0.85f;
			if (nd.x >  thr) vf &= ~FACE_LEFT;
			if (nd.x < -thr) vf &= ~FACE_RIGHT;
			if (nd.y >  thr) vf &= ~FACE_BOTTOM;
			if (nd.y < -thr) vf &= ~FACE_TOP;
			if (nd.z >  thr) vf &= ~FACE_BACK;
			if (nd.z < -thr) vf &= ~FACE_FRONT;
		}
	}
	return vf;
}

void update_frustum() {
#ifdef DEBUG
	profiler_start(PROFILER_ID_CULLING, false);
#endif
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos = {
		global_entities[0].pos.x,
		global_entities[0].pos.y + global_entities[0].eye_level,
		global_entities[0].pos.z
	};
	float fov = cosf(settings.fov * DEG_TO_RAD);

	static uint8_t *prev = NULL;
	static int      prev_rd = 0;
	static bool     first   = true;

	int rd = settings.render_distance;
	if (prev_rd != rd) {
		free(prev);
		prev    = calloc((size_t)rd * WORLD_HEIGHT * rd, 1);
		prev_rd = rd;
		first   = true;
	}

	frustum_changed = false;

	for (int x = 0; x < rd; x++) {
		for (int y = 0; y < WORLD_HEIGHT; y++) {
			for (int z = 0; z < rd; z++) {
				uint8_t vf = settings.frustum_culling
				    ? get_visible_faces(pos, dir, world_offset_x + x, y, world_offset_z + z, fov)
				    : ALL_FACES;
				visibility_map[x][y][z] = vf;
				if (!first && !frustum_changed &&
				    prev[x * WORLD_HEIGHT * rd + y * rd + z] != vf)
					frustum_changed = true;
			}
		}
	}

	if (first) { frustum_changed = true; first = false; }

	if (prev)
		for (int x = 0; x < rd; x++)
			for (int y = 0; y < WORLD_HEIGHT; y++)
				memcpy(&prev[x * WORLD_HEIGHT * rd + y * rd], visibility_map[x][y], rd);

	if (frustum_changed) mesh_needs_rebuild = true;
#ifdef DEBUG
	profiler_stop(PROFILER_ID_CULLING, false);
#endif
}
