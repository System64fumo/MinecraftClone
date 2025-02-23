#include "main.h"
#include <stdlib.h>

Chunk*** allocate_chunks(int render_distance, int world_height) {
	Chunk ***chunks = (Chunk ***)malloc(render_distance * sizeof(Chunk **));
	for (int i = 0; i < render_distance; i++) {
		chunks[i] = (Chunk **)malloc(world_height * sizeof(Chunk *));
		for (int j = 0; j < world_height; j++) {
			chunks[i][j] = (Chunk *)malloc(render_distance * sizeof(Chunk));
		}
	}
	return chunks;
}

void free_chunks(Chunk ***chunks, int render_distance, int world_height) {
	for (int i = 0; i < render_distance; i++) {
		for (int j = 0; j < world_height; j++) {
			free(chunks[i][j]);
		}
		free(chunks[i]);
	}
	free(chunks);
}