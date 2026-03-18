#include "main.h"
#include "world.h"
#include "entity.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>

// Legacy globals kept so existing code that references them still compiles.
uint32_t opaque_VAO = 0, opaque_VBO = 0, opaque_EBO = 0, opaque_index_count = 0;
uint32_t transparent_VAO = 0, transparent_VBO = 0, transparent_EBO = 0, transparent_index_count = 0;

bool mesh_mode = false;
uint16_t draw_calls = 0;
_Atomic bool mesh_needs_rebuild = false;
uint8_t ***visibility_map = NULL;

// ---------------------------------------------------------------------------
// Per-chunk VAO helpers
// ---------------------------------------------------------------------------

static void setup_vao_attribs(uint32_t vao, uint32_t vbo, uint32_t ebo) {
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glVertexAttribIPointer(0, 1, GL_INT,           sizeof(Vertex), (void*)offsetof(Vertex, x));
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_SHORT, sizeof(Vertex), (void*)offsetof(Vertex, y));
	glEnableVertexAttribArray(1);
	glVertexAttribIPointer(2, 1, GL_INT,           sizeof(Vertex), (void*)offsetof(Vertex, z));
	glEnableVertexAttribArray(2);
	glVertexAttribIPointer(3, 1, GL_UNSIGNED_SHORT, sizeof(Vertex), (void*)offsetof(Vertex, packed_data));
	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT,   sizeof(Vertex), (void*)offsetof(Vertex, packed_size));
	glEnableVertexAttribArray(4);
	glBindVertexArray(0);
}

void chunk_alloc_gpu_buffers(Chunk *chunk) {
	if (chunk->gpu_buffers_valid) return;
	glGenVertexArrays(1, &chunk->opaque_vao);
	glGenBuffers(1, &chunk->opaque_vbo);
	glGenBuffers(1, &chunk->opaque_ebo);
	glGenVertexArrays(1, &chunk->transparent_vao);
	glGenBuffers(1, &chunk->transparent_vbo);
	glGenBuffers(1, &chunk->transparent_ebo);
	setup_vao_attribs(chunk->opaque_vao,      chunk->opaque_vbo,      chunk->opaque_ebo);
	setup_vao_attribs(chunk->transparent_vao, chunk->transparent_vbo, chunk->transparent_ebo);
	chunk->gpu_buffers_valid = true;
}

void chunk_free_gpu_buffers(Chunk *chunk) {
	if (!chunk->gpu_buffers_valid) return;
	glDeleteVertexArrays(1, &chunk->opaque_vao);
	glDeleteBuffers(1, &chunk->opaque_vbo);
	glDeleteBuffers(1, &chunk->opaque_ebo);
	glDeleteVertexArrays(1, &chunk->transparent_vao);
	glDeleteBuffers(1, &chunk->transparent_vbo);
	glDeleteBuffers(1, &chunk->transparent_ebo);
	chunk->opaque_vao = chunk->opaque_vbo = chunk->opaque_ebo = 0;
	chunk->transparent_vao = chunk->transparent_vbo = chunk->transparent_ebo = 0;
	chunk->opaque_index_count = chunk->transparent_index_count = 0;
	chunk->gpu_buffers_valid = false;
}

// Upload a chunk's CPU mesh (all 6 face sub-meshes merged) to its GPU buffers.
// Must be called from the main/GL thread. Does NOT require chunks_mutex.
void chunk_upload_mesh(Chunk *chunk) {
	chunk_alloc_gpu_buffers(chunk);

	// Merge all 6 face sub-meshes into contiguous arrays and upload.
	for (int pass = 0; pass < 2; pass++) {
		Mesh *faces = (pass == 0) ? chunk->faces : chunk->transparent_faces;
		uint32_t vao = (pass == 0) ? chunk->opaque_vao      : chunk->transparent_vao;
		uint32_t vbo = (pass == 0) ? chunk->opaque_vbo      : chunk->transparent_vbo;
		uint32_t ebo = (pass == 0) ? chunk->opaque_ebo      : chunk->transparent_ebo;
		uint32_t *idx_count = (pass == 0) ? &chunk->opaque_index_count : &chunk->transparent_index_count;

		uint32_t total_verts = 0, total_idxs = 0;
		for (int f = 0; f < 6; f++) {
			total_verts += faces[f].vertex_count;
			total_idxs  += faces[f].index_count;
		}

		*idx_count = total_idxs;

		if (total_verts == 0) {
			// Empty mesh — upload nothing but clear the buffer.
			glBindVertexArray(vao);
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_DYNAMIC_DRAW);
			continue;
		}

		Vertex   *vbuf = malloc(total_verts * sizeof(Vertex));
		uint32_t *ibuf = malloc(total_idxs  * sizeof(uint32_t));
		if (!vbuf || !ibuf) { free(vbuf); free(ibuf); continue; }

		uint32_t vo = 0, io = 0, base = 0;
		for (int f = 0; f < 6; f++) {
			if (faces[f].vertex_count == 0) continue;
			memcpy(vbuf + vo, faces[f].vertices, faces[f].vertex_count * sizeof(Vertex));
			for (uint32_t i = 0; i < faces[f].index_count; i++)
				ibuf[io + i] = faces[f].indices[i] + base;
			vo   += faces[f].vertex_count;
			io   += faces[f].index_count;
			base += faces[f].vertex_count;
		}

		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, total_verts * sizeof(Vertex), vbuf, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, total_idxs * sizeof(uint32_t), ibuf, GL_DYNAMIC_DRAW);
		free(vbuf);
		free(ibuf);
	}
	glBindVertexArray(0);
}

// ---------------------------------------------------------------------------
// init_gl_buffers — allocate visibility map (per-chunk VAOs are lazy-init)
// ---------------------------------------------------------------------------
void init_gl_buffers() {
	if (visibility_map) {
		for (int x = 0; x < settings.render_distance; x++) {
			for (int y = 0; y < WORLD_HEIGHT; y++)
				free(visibility_map[x][y]);
			free(visibility_map[x]);
		}
		free(visibility_map);
	}
	visibility_map = malloc(settings.render_distance * sizeof(uint8_t**));
	for (int x = 0; x < settings.render_distance; x++) {
		visibility_map[x] = malloc(WORLD_HEIGHT * sizeof(uint8_t*));
		for (int y = 0; y < WORLD_HEIGHT; y++)
			visibility_map[x][y] = calloc(settings.render_distance, sizeof(uint8_t));
	}
}

// ---------------------------------------------------------------------------
// Rate-limited GPU upload: max 32 dirty chunks per frame to prevent spikes
// during world generation. Mutex held only briefly to collect dirty flags.
#define MAX_UPLOADS_PER_FRAME 32
void rebuild_combined_visible_mesh() {
#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE, false);
#endif

	static uint16_t dirty_x[4096], dirty_y[4096], dirty_z[4096];
	int dirty_count = 0;

	pthread_mutex_lock(&chunks_mutex);
	for (uint8_t x = 0; x < settings.render_distance && dirty_count < 4096; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT && dirty_count < 4096; y++) {
			for (uint8_t z = 0; z < settings.render_distance && dirty_count < 4096; z++) {
				Chunk *c = &chunks[x][y][z];
				if (c->is_loaded && c->mesh_dirty) {
					if (dirty_count < MAX_UPLOADS_PER_FRAME)
						c->mesh_dirty = false;
					dirty_x[dirty_count] = x;
					dirty_y[dirty_count] = y;
					dirty_z[dirty_count] = z;
					dirty_count++;
				}
			}
		}
	}
	if (dirty_count > MAX_UPLOADS_PER_FRAME)
		atomic_store(&mesh_needs_rebuild, true);
	else
		mesh_needs_rebuild = false;
	pthread_mutex_unlock(&chunks_mutex);

	int uploads = dirty_count < MAX_UPLOADS_PER_FRAME ? dirty_count : MAX_UPLOADS_PER_FRAME;
#ifdef DEBUG
	profiler_start(PROFILER_ID_UPLOAD, false);
#endif
	for (int i = 0; i < uploads; i++) {
		Chunk *c = &chunks[dirty_x[i]][dirty_y[i]][dirty_z[i]];
		if (c->is_loaded)
			chunk_upload_mesh(c);
	}
#ifdef DEBUG
	profiler_stop(PROFILER_ID_UPLOAD, false);
#endif

#ifdef DEBUG
	profiler_stop(PROFILER_ID_MERGE, false);
#endif
}

// ---------------------------------------------------------------------------
// render_chunks — draw each visible chunk's per-chunk VAO directly.
// Two passes: opaque first, then transparent.
// ---------------------------------------------------------------------------
void render_chunks() {
	if (mesh_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	draw_calls = 0;

	// Opaque pass.
	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				if (!visibility_map[x][y][z]) continue;
				Chunk *chunk = &chunks[x][y][z];
				if (!chunk->is_loaded || !chunk->gpu_buffers_valid) continue;
				if (chunk->opaque_index_count == 0) continue;
				glBindVertexArray(chunk->opaque_vao);
				glDrawElements(GL_TRIANGLES, chunk->opaque_index_count, GL_UNSIGNED_INT, 0);
				draw_calls++;
			}
		}
	}

	// Transparent pass — sorted back-to-front by chunk centre distance to player.
	// This fixes alpha blending artifacts where nearer transparent surfaces
	// (water) would incorrectly overwrite farther ones.
	typedef struct { float dist_sq; uint8_t x, y, z; } TransChunk;
	static TransChunk trans_chunks[4096];
	int trans_count = 0;

	float px = global_entities[0].pos.x;
	float py = global_entities[0].pos.y;
	float pz = global_entities[0].pos.z;

	for (uint8_t x = 0; x < settings.render_distance; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < settings.render_distance; z++) {
				if (!visibility_map[x][y][z]) continue;
				Chunk *chunk = &chunks[x][y][z];
				if (!chunk->is_loaded || !chunk->gpu_buffers_valid) continue;
				if (chunk->transparent_index_count == 0) continue;
				float cx = (chunk->x + 0.5f) * CHUNK_SIZE - px;
				float cy = (chunk->y + 0.5f) * CHUNK_SIZE - py;
				float cz = (chunk->z + 0.5f) * CHUNK_SIZE - pz;
				if (trans_count < 4096) {
					trans_chunks[trans_count++] = (TransChunk){
						cx*cx + cy*cy + cz*cz, x, y, z
					};
				}
			}
		}
	}

	// Simple insertion sort — chunk count is small enough that it's fast.
	for (int i = 1; i < trans_count; i++) {
		TransChunk key = trans_chunks[i];
		int j = i - 1;
		while (j >= 0 && trans_chunks[j].dist_sq < key.dist_sq) {
			trans_chunks[j+1] = trans_chunks[j];
			j--;
		}
		trans_chunks[j+1] = key;
	}

	for (int i = 0; i < trans_count; i++) {
		Chunk *chunk = &chunks[trans_chunks[i].x][trans_chunks[i].y][trans_chunks[i].z];
		glBindVertexArray(chunk->transparent_vao);
		glDrawElements(GL_TRIANGLES, chunk->transparent_index_count, GL_UNSIGNED_INT, 0);
		draw_calls++;
	}

	if (mesh_mode)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void cleanup_renderer() {
	if (chunks) {
		for (int x = 0; x < settings.render_distance; x++)
			for (int y = 0; y < WORLD_HEIGHT; y++)
				for (int z = 0; z < settings.render_distance; z++)
					chunk_free_gpu_buffers(&chunks[x][y][z]);
	}
}