#include "main.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	bool running;
	bool should_process;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool chunks_ready_to_combine;
	pthread_t worker_threads[1];
	int worker_count;
	volatile bool shutdown;
} chunk_processor_t;

typedef struct {
	uint8_t start_x, end_x;
	uint8_t start_y, end_y;
	uint8_t start_z, end_z;
} work_range_t;

typedef struct {
	work_range_t range;
	bool* chunks_updated;
	pthread_mutex_t* result_mutex;
} work_item_t;

chunk_processor_t chunk_processor = {
	.running = false,
	.should_process = false,
	.thread = 0,
	.chunks_ready_to_combine = false,
	.worker_count = 1,
	.shutdown = false
};

pthread_mutex_t mesh_mutex;
pthread_mutex_t work_queue_mutex;
pthread_cond_t work_available;
work_item_t work_queue[16];
int work_queue_head = 0;
int work_queue_tail = 0;
int work_queue_count = 0;

ChunkRenderData chunk_render_data[RENDER_DISTANCE][WORLD_HEIGHT][RENDER_DISTANCE];

// Memory pools for vertex/index data to reduce allocations
typedef struct {
	Vertex* vertices;
	uint32_t* indices;
	uint32_t vertex_capacity;
	uint32_t index_capacity;
} mesh_pool_t;

static mesh_pool_t opaque_pool = {0};
static mesh_pool_t transparent_pool = {0};

// Worker thread function for parallel chunk processing
void* worker_thread(void* arg) {
	while (!chunk_processor.shutdown) {
		work_item_t work;
		bool has_work = false;
		
		// Get work from queue
		pthread_mutex_lock(&work_queue_mutex);
		while (work_queue_count == 0 && !chunk_processor.shutdown) {
			pthread_cond_wait(&work_available, &work_queue_mutex);
		}
		
		if (work_queue_count > 0) {
			work = work_queue[work_queue_head];
			work_queue_head = (work_queue_head + 1) % 16;
			work_queue_count--;
			has_work = true;
		}
		pthread_mutex_unlock(&work_queue_mutex);
		
		if (!has_work) continue;
		
		// Process assigned chunk range
		bool local_updated = false;
		for (uint8_t x = work.range.start_x; x < work.range.end_x; x++) {
			for (uint8_t y = work.range.start_y; y < work.range.end_y; y++) {
				for (uint8_t z = work.range.start_z; z < work.range.end_z; z++) {
					Chunk* chunk = &chunks[x][y][z];
					
					pthread_mutex_lock(&mesh_mutex);
					bool needs_update = chunk->needs_update;
					if (needs_update) {
						set_chunk_lighting(chunk);
						generate_chunk_mesh(chunk);
						chunk->needs_update = false;
						local_updated = true;
					}
					pthread_mutex_unlock(&mesh_mutex);
				}
			}
		}
		
		// Update result
		pthread_mutex_lock(work.result_mutex);
		*work.chunks_updated |= local_updated;
		pthread_mutex_unlock(work.result_mutex);
	}
	return NULL;
}

void* chunk_processor_thread(void* arg) {
	chunk_processor_t* processor = (chunk_processor_t*)arg;
	
	while (!processor->shutdown) {
		bool should_process = false;

		// Wait for process request
		pthread_mutex_lock(&processor->mutex);
		while (!processor->should_process && !processor->shutdown) {
			pthread_cond_wait(&processor->cond, &processor->mutex);
		}

		should_process = processor->should_process;
		processor->should_process = false;
		pthread_mutex_unlock(&processor->mutex);

		if (should_process && !processor->shutdown) {
			bool chunks_updated = false;
			pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
			
			// Divide work among worker threads
			const uint8_t chunks_per_worker = RENDER_DISTANCE / processor->worker_count;
			
			pthread_mutex_lock(&work_queue_mutex);
			for (int i = 0; i < processor->worker_count; i++) {
				uint8_t start_x = i * chunks_per_worker;
				uint8_t end_x = (i == processor->worker_count - 1) ? 
					RENDER_DISTANCE : (i + 1) * chunks_per_worker;
				
				work_item_t work = {
					.range = {start_x, end_x, 0, WORLD_HEIGHT, 0, RENDER_DISTANCE},
					.chunks_updated = &chunks_updated,
					.result_mutex = &result_mutex
				};
				
				work_queue[work_queue_tail] = work;
				work_queue_tail = (work_queue_tail + 1) % 16;
				work_queue_count++;
			}
			pthread_cond_broadcast(&work_available);
			pthread_mutex_unlock(&work_queue_mutex);
			
			// Wait for all work to complete
			bool work_remaining = true;
			while (work_remaining) {
				pthread_mutex_lock(&work_queue_mutex);
				work_remaining = (work_queue_count > 0);
				pthread_mutex_unlock(&work_queue_mutex);
				
				if (work_remaining) {
					usleep(100); // Small delay to avoid busy waiting
				}
			}

			pthread_mutex_lock(&processor->mutex);
			processor->chunks_ready_to_combine = chunks_updated;
			pthread_mutex_unlock(&processor->mutex);
			
			pthread_mutex_destroy(&result_mutex);
		}
	}

	return NULL;
}

void init_chunk_processor() {
	pthread_mutex_init(&chunk_processor.mutex, NULL);
	pthread_cond_init(&chunk_processor.cond, NULL);
	pthread_mutex_init(&mesh_mutex, NULL);
	pthread_mutex_init(&work_queue_mutex, NULL);
	pthread_cond_init(&work_available, NULL);
	
	// Initialize memory pools
	opaque_pool.vertex_capacity = RENDER_DISTANCE * WORLD_HEIGHT * RENDER_DISTANCE * 512;
	opaque_pool.index_capacity = opaque_pool.vertex_capacity * 2;
	opaque_pool.vertices = malloc(opaque_pool.vertex_capacity * sizeof(Vertex));
	opaque_pool.indices = malloc(opaque_pool.index_capacity * sizeof(uint32_t));
	
	transparent_pool.vertex_capacity = opaque_pool.vertex_capacity / 4; // Assume less transparent geometry
	transparent_pool.index_capacity = transparent_pool.vertex_capacity * 2;
	transparent_pool.vertices = malloc(transparent_pool.vertex_capacity * sizeof(Vertex));
	transparent_pool.indices = malloc(transparent_pool.index_capacity * sizeof(uint32_t));
}

void destroy_chunk_processor() {
	chunk_processor.shutdown = true;
	
	if (chunk_processor.running) {
		pthread_cond_broadcast(&chunk_processor.cond);
		pthread_cond_broadcast(&work_available);
		
		pthread_join(chunk_processor.thread, NULL);
		for (int i = 0; i < chunk_processor.worker_count; i++) {
			pthread_join(chunk_processor.worker_threads[i], NULL);
		}
		chunk_processor.running = false;
	}
	
	pthread_mutex_destroy(&chunk_processor.mutex);
	pthread_cond_destroy(&chunk_processor.cond);
	pthread_mutex_destroy(&mesh_mutex);
	pthread_mutex_destroy(&work_queue_mutex);
	pthread_cond_destroy(&work_available);
	
	// Free memory pools
	free(opaque_pool.vertices);
	free(opaque_pool.indices);
	free(transparent_pool.vertices);
	free(transparent_pool.indices);
}

// Separate function for GL buffer updates to reduce mutex hold time
void update_gl_buffers() {
	// Update opaque buffers
	glBindVertexArray(combined_VAO);
	
	if (combined_mesh.vertex_count > 0) {
		size_t required_vbo_size = combined_mesh.vertex_count * sizeof(Vertex);
		size_t required_ebo_size = combined_mesh.index_count * sizeof(uint32_t);
		
		// Use persistent mapping for better performance
		glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
		glBufferData(GL_ARRAY_BUFFER, required_vbo_size, 
			combined_mesh.vertices, GL_STREAM_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, required_ebo_size, 
			combined_mesh.indices, GL_STREAM_DRAW);
	}
	
	// Update transparent buffers
	glBindVertexArray(transparent_VAO);
	
	if (transparent_mesh.vertex_count > 0) {
		size_t required_vbo_size = transparent_mesh.vertex_count * sizeof(Vertex);
		size_t required_ebo_size = transparent_mesh.index_count * sizeof(uint32_t);
		
		glBindBuffer(GL_ARRAY_BUFFER, transparent_VBO);
		glBufferData(GL_ARRAY_BUFFER, required_vbo_size, 
			transparent_mesh.vertices, GL_STREAM_DRAW);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, transparent_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, required_ebo_size, 
			transparent_mesh.indices, GL_STREAM_DRAW);
	}
	
	glBindVertexArray(0);
}

void combine_meshes() {
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MESH, false);
	#endif
	
	pthread_mutex_lock(&mesh_mutex);
	
	uint32_t total_vertices = 0;
	uint32_t total_indices = 0;
	uint32_t total_transparent_vertices = 0;
	uint32_t total_transparent_indices = 0;

	// Calculate total sizes in a single pass
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				total_vertices += chunk->vertex_count;
				total_indices += chunk->index_count;
				total_transparent_vertices += chunk->transparent_vertex_count;
				total_transparent_indices += chunk->transparent_index_count;
			}
		}
	}

	// Handle opaque mesh with memory pool
	if (total_vertices == 0 || total_indices == 0) {
		combined_mesh.vertex_count = 0;
		combined_mesh.index_count = 0;
	} else {
		// Use memory pool if possible, otherwise allocate
		if (total_vertices <= opaque_pool.vertex_capacity && 
			total_indices <= opaque_pool.index_capacity) {
			
			if (combined_mesh.vertices != opaque_pool.vertices) {
				free(combined_mesh.vertices);
				combined_mesh.vertices = opaque_pool.vertices;
				combined_mesh.capacity_vertices = opaque_pool.vertex_capacity;
			}
			
			if (combined_mesh.indices != opaque_pool.indices) {
				free(combined_mesh.indices);
				combined_mesh.indices = opaque_pool.indices;
				combined_mesh.capacity_indices = opaque_pool.index_capacity;
			}
		} else {
			// Fallback to dynamic allocation
			if (combined_mesh.capacity_vertices < total_vertices) {
				void* new_vertices = realloc(combined_mesh.vertices, 
					total_vertices * sizeof(Vertex));
				if (new_vertices) {
					combined_mesh.vertices = new_vertices;
					combined_mesh.capacity_vertices = total_vertices;
				} else {
					pthread_mutex_unlock(&mesh_mutex);
					fprintf(stderr, "Failed to allocate vertex memory\n");
					return;
				}
			}

			if (combined_mesh.capacity_indices < total_indices) {
				void* new_indices = realloc(combined_mesh.indices,
					total_indices * sizeof(uint32_t));
				if (new_indices) {
					combined_mesh.indices = new_indices;
					combined_mesh.capacity_indices = total_indices;
				} else {
					pthread_mutex_unlock(&mesh_mutex);
					fprintf(stderr, "Failed to allocate index memory\n");
					return;
				}
			}
		}

		// Combine meshes with optimized copying
		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;
		uint32_t base_vertex = 0;

		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (chunk->vertex_count == 0 || chunk->vertices == NULL) {
						chunk_render_data[x][y][z].index_count = 0;
						chunk_render_data[x][y][z].visible = false;
						continue;
					}

					chunk_render_data[x][y][z].start_index = index_offset;
					chunk_render_data[x][y][z].index_count = chunk->index_count;
					chunk_render_data[x][y][z].visible = true;

					memcpy(combined_mesh.vertices + vertex_offset, chunk->vertices,
						   chunk->vertex_count * sizeof(Vertex));

					uint32_t* dst_indices = combined_mesh.indices + index_offset;
					uint32_t* src_indices = chunk->indices;
					uint32_t count = chunk->index_count;

					uint32_t unrolled_count = count & ~3;
					for (uint32_t i = 0; i < unrolled_count; i += 4) {
						dst_indices[i] = src_indices[i] + base_vertex;
						dst_indices[i + 1] = src_indices[i + 1] + base_vertex;
						dst_indices[i + 2] = src_indices[i + 2] + base_vertex;
						dst_indices[i + 3] = src_indices[i + 3] + base_vertex;
					}
					
					// Handle remaining indices
					for (uint32_t i = unrolled_count; i < count; i++) {
						dst_indices[i] = src_indices[i] + base_vertex;
					}

					vertex_offset += chunk->vertex_count;
					index_offset += chunk->index_count;
					base_vertex += chunk->vertex_count;
				}
			}
		}

		combined_mesh.vertex_count = vertex_offset;
		combined_mesh.index_count = index_offset;
	}

	// Similar optimization for transparent mesh (using transparent_pool)
	if (total_transparent_vertices == 0 || total_transparent_indices == 0) {
		transparent_mesh.vertex_count = 0;
		transparent_mesh.index_count = 0;
	} else {
		// Use transparent memory pool
		if (total_transparent_vertices <= transparent_pool.vertex_capacity && 
			total_transparent_indices <= transparent_pool.index_capacity) {
			
			if (transparent_mesh.vertices != transparent_pool.vertices) {
				free(transparent_mesh.vertices);
				transparent_mesh.vertices = transparent_pool.vertices;
				transparent_mesh.capacity_vertices = transparent_pool.vertex_capacity;
			}
			
			if (transparent_mesh.indices != transparent_pool.indices) {
				free(transparent_mesh.indices);
				transparent_mesh.indices = transparent_pool.indices;
				transparent_mesh.capacity_indices = transparent_pool.index_capacity;
			}
		} else {
			// Fallback allocation for transparent mesh
			if (transparent_mesh.capacity_vertices < total_transparent_vertices) {
				void* new_vertices = realloc(transparent_mesh.vertices,
					total_transparent_vertices * sizeof(Vertex));
				if (new_vertices) {
					transparent_mesh.vertices = new_vertices;
					transparent_mesh.capacity_vertices = total_transparent_vertices;
				} else {
					pthread_mutex_unlock(&mesh_mutex);
					fprintf(stderr, "Failed to allocate transparent vertex memory\n");
					return;
				}
			}

			if (transparent_mesh.capacity_indices < total_transparent_indices) {
				void* new_indices = realloc(transparent_mesh.indices,
					total_transparent_indices * sizeof(uint32_t));
				if (new_indices) {
					transparent_mesh.indices = new_indices;
					transparent_mesh.capacity_indices = total_transparent_indices;
				} else {
					pthread_mutex_unlock(&mesh_mutex);
					fprintf(stderr, "Failed to allocate transparent index memory\n");
					return;
				}
			}
		}

		// Combine transparent meshes
		uint32_t vertex_offset = 0;
		uint32_t index_offset = 0;
		uint32_t base_vertex = 0;

		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
				for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
					Chunk* chunk = &chunks[x][y][z];
					if (chunk->transparent_vertex_count == 0 || 
						chunk->transparent_vertices == NULL) {
						continue;
					}

					memcpy(transparent_mesh.vertices + vertex_offset, 
						   chunk->transparent_vertices,
						   chunk->transparent_vertex_count * sizeof(Vertex));

					// Optimized transparent index adjustment with loop unrolling
					uint32_t* dst_indices = transparent_mesh.indices + index_offset;
					uint32_t* src_indices = chunk->transparent_indices;
					uint32_t count = chunk->transparent_index_count;
					
					// Process in groups of 4 for better cache utilization
					uint32_t unrolled_count = count & ~3;
					for (uint32_t i = 0; i < unrolled_count; i += 4) {
						dst_indices[i] = src_indices[i] + base_vertex;
						dst_indices[i + 1] = src_indices[i + 1] + base_vertex;
						dst_indices[i + 2] = src_indices[i + 2] + base_vertex;
						dst_indices[i + 3] = src_indices[i + 3] + base_vertex;
					}
					
					for (uint32_t i = unrolled_count; i < count; i++) {
						dst_indices[i] = src_indices[i] + base_vertex;
					}

					vertex_offset += chunk->transparent_vertex_count;
					index_offset += chunk->transparent_index_count;
					base_vertex += chunk->transparent_vertex_count;
				}
			}
		}

		transparent_mesh.vertex_count = vertex_offset;
		transparent_mesh.index_count = index_offset;
	}
	
	pthread_mutex_unlock(&mesh_mutex);

	update_gl_buffers();
	
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MESH, false);
	#endif
}

void process_chunks() {
	static pthread_mutex_t processing_mutex = PTHREAD_MUTEX_INITIALIZER;
	static bool processing = false;
	
	// Prevent multiple threads from entering this section
	pthread_mutex_lock(&processing_mutex);
	if (processing) {
		pthread_mutex_unlock(&processing_mutex);
		return;
	}
	#ifdef DEBUG
	profiler_start(PROFILER_ID_MERGE, false);
	#endif
	processing = true;
	pthread_mutex_unlock(&processing_mutex);

	static bool initialized = false;
	if (!initialized) {
		init_chunk_processor();
		
		// Start worker threads
		for (int i = 0; i < chunk_processor.worker_count; i++) {
			pthread_create(&chunk_processor.worker_threads[i], NULL, worker_thread, NULL);
		}
		initialized = true;
	}

	pthread_mutex_lock(&chunk_processor.mutex);
	if (!chunk_processor.running) {
		chunk_processor.running = true;
		pthread_create(&chunk_processor.thread, NULL, chunk_processor_thread, &chunk_processor);
	}

	chunk_processor.should_process = true;
	pthread_cond_signal(&chunk_processor.cond);

	bool chunks_ready = chunk_processor.chunks_ready_to_combine;
	chunk_processor.chunks_ready_to_combine = false;
	pthread_mutex_unlock(&chunk_processor.mutex);

	if (chunks_ready)
		combine_meshes();

	pthread_mutex_lock(&processing_mutex);
	processing = false;
	pthread_mutex_unlock(&processing_mutex);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_MERGE, false);
	#endif
}