#include "main.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
	bool running;
	bool should_process;
	pthread_t thread;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool chunks_ready_to_combine;
} chunk_processor_t;

chunk_processor_t chunk_processor = {
	.running = false,
	.should_process = false,
	.thread = 0,
	.chunks_ready_to_combine = false
};

pthread_mutex_t mesh_mutex;

void* chunk_processor_thread(void* arg) {
	chunk_processor_t* processor = (chunk_processor_t*)arg;
	
	while (true) {
		bool should_process = false;

		// Wait for process request
		pthread_mutex_lock(&processor->mutex);
		while (!processor->should_process) {
			pthread_cond_wait(&processor->cond, &processor->mutex);
		}

		should_process = processor->should_process;
		processor->should_process = false;
		pthread_mutex_unlock(&processor->mutex);

		if (should_process) {
			bool chunks_updated = false;

			// Process chunks that need updating
			for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
				for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
					for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
						bool needs_update = false;
						Chunk* chunk = NULL;
						
						// Lock both mutexes in specific order to prevent deadlocks
						pthread_mutex_lock(&processor->mutex);
						pthread_mutex_lock(&mesh_mutex);
						
						chunk = &chunks[x][y][z];
						needs_update = chunk->needs_update;
						
						if (needs_update) {
							generate_chunk_mesh(chunk);
							chunk->needs_update = false;
							chunks_updated = true;
						}
						
						pthread_mutex_unlock(&mesh_mutex);
						pthread_mutex_unlock(&processor->mutex);
					}
				}
			}

			pthread_mutex_lock(&processor->mutex);
			processor->chunks_ready_to_combine = chunks_updated;
			pthread_mutex_unlock(&processor->mutex);
		}
	}

	return NULL;
}

void init_chunk_processor() {
	pthread_mutex_init(&chunk_processor.mutex, NULL);
	pthread_cond_init(&chunk_processor.cond, NULL);
	pthread_mutex_init(&mesh_mutex, NULL);
}

void destroy_chunk_processor() {
	if (chunk_processor.running) {
		pthread_cancel(chunk_processor.thread);
		chunk_processor.running = false;
	}
	
	pthread_mutex_destroy(&chunk_processor.mutex);
	pthread_cond_destroy(&chunk_processor.cond);
	pthread_mutex_destroy(&mesh_mutex);
}

void combine_meshes() {

	pthread_mutex_lock(&mesh_mutex);
	
	uint32_t total_vertices = 0;
	uint32_t total_indices = 0;

	// Calculate total sizes in a single pass
	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				total_vertices += chunk->vertex_count;
				total_indices += chunk->index_count;
			}
		}
	}

	// Skip if there's nothing to render
	if (total_vertices == 0 || total_indices == 0) {
		combined_mesh.vertex_count = 0;
		combined_mesh.index_count = 0;
		pthread_mutex_unlock(&mesh_mutex);
		return;
	}

	// Reuse existing buffers if they're large enough
	if (combined_mesh.capacity_vertices < total_vertices) {
		void* new_vertices = malloc(total_vertices * sizeof(Vertex));
		if (new_vertices) {
			free(combined_mesh.vertices);
			combined_mesh.vertices = new_vertices;
			combined_mesh.capacity_vertices = total_vertices;
		}
		else {
			pthread_mutex_unlock(&mesh_mutex);
			fprintf(stderr, "Failed to allocate vertex memory\n");
			return;
		}
	}

	if (combined_mesh.capacity_indices < total_indices) {
		void* new_indices = malloc(total_indices * sizeof(uint32_t));
		if (new_indices) {
			free(combined_mesh.indices);
			combined_mesh.indices = new_indices;
			combined_mesh.capacity_indices = total_indices;
		}
		else {
			pthread_mutex_unlock(&mesh_mutex);
			fprintf(stderr, "Failed to allocate index memory\n");
			return;
		}
	}

	// Combine all meshes
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;
	uint32_t base_vertex = 0;

	for (uint8_t x = 0; x < RENDER_DISTANCE; x++) {
		for (uint8_t y = 0; y < WORLD_HEIGHT; y++) {
			for (uint8_t z = 0; z < RENDER_DISTANCE; z++) {
				Chunk* chunk = &chunks[x][y][z];
				if (chunk->vertex_count == 0 || chunk->vertices == NULL)
					continue;

				// Copy vertices
				memcpy(combined_mesh.vertices + vertex_offset, chunk->vertices,
					   chunk->vertex_count * sizeof(Vertex));

				// Copy and adjust indices
				for (uint32_t i = 0; i < chunk->index_count; i++) {
					combined_mesh.indices[index_offset + i] = chunk->indices[i] + base_vertex;
				}

				vertex_offset += chunk->vertex_count;
				index_offset += chunk->index_count;
				base_vertex += chunk->vertex_count;
			}
		}
	}

	combined_mesh.vertex_count = vertex_offset;
	combined_mesh.index_count = index_offset;
	
	pthread_mutex_unlock(&mesh_mutex);

	// Update buffer data using glBufferSubData for better performance
	glBindVertexArray(combined_VAO);
	
	// Check if buffer sizes need to be increased
	GLint current_vbo_size = 0;
	GLint current_ebo_size = 0;
	glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
	glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_vbo_size);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
	glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &current_ebo_size);
	
	size_t required_vbo_size = combined_mesh.vertex_count * sizeof(Vertex);
	size_t required_ebo_size = combined_mesh.index_count * sizeof(uint32_t);
	
	// Resize buffers if needed
	if (current_vbo_size < required_vbo_size) {
		glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
		glBufferData(GL_ARRAY_BUFFER, required_vbo_size, NULL, GL_DYNAMIC_DRAW);
	}
	
	if (current_ebo_size < required_ebo_size) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, required_ebo_size, NULL, GL_DYNAMIC_DRAW);
	}
	
	// Update buffer contents with glBufferSubData
	glBindBuffer(GL_ARRAY_BUFFER, combined_VBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, required_vbo_size, combined_mesh.vertices);
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, combined_EBO);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, required_ebo_size, combined_mesh.indices);
	
	glBindVertexArray(0);
}

void process_chunks() {
	static bool processing = false;
	if (processing) return;

	processing = true;

	static bool initialized = false;
	if (!initialized) {
		init_chunk_processor();
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

	processing = false;
}