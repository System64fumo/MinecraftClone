#include "main.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <png.h>

#define MAX_LIGHT_LEVEL 15

void set_chunk_lighting(Chunk* chunk) {
	for (uint8_t x = 0; x < CHUNK_SIZE; x++) {
		for (uint8_t y = 0; y < CHUNK_SIZE; y++) {
			for (uint8_t z = 0; z < CHUNK_SIZE; z++) {
				if (chunk->blocks[x][y][z].id == 6)
					chunk->blocks[x][y][z].light_level = 7;
				else if (block_data[chunk->blocks[x][y][z].id][1] == 1)
					chunk->blocks[x][y][z].light_level = settings.sky_brightness * MAX_LIGHT_LEVEL;
			}
		}
	}
}

#define WORLD_WIDTH (RENDER_DISTANCE * CHUNK_SIZE)
#define WORLD_DEPTH (RENDER_DISTANCE * CHUNK_SIZE)
#define WORLD_HEIGHT_BLOCKS (WORLD_HEIGHT * CHUNK_SIZE)

unsigned char* generateLightTexture3D(int* out_width, int* out_height, int* out_depth) {
	*out_width = WORLD_WIDTH;
	*out_height = WORLD_HEIGHT_BLOCKS;
	*out_depth = WORLD_DEPTH;
	
	size_t texture_size = WORLD_WIDTH * WORLD_HEIGHT_BLOCKS * WORLD_DEPTH * 4;
	unsigned char* texture_data = (unsigned char*)malloc(texture_size);
	if (!texture_data) {
		fprintf(stderr, "Failed to allocate memory for 3D texture\n");
		return NULL;
	}
	
	memset(texture_data, 0, texture_size);
	
	for (int cx = 0; cx < RENDER_DISTANCE; cx++) {
		for (int cy = 0; cy < WORLD_HEIGHT; cy++) {
			for (int cz = 0; cz < RENDER_DISTANCE; cz++) {
				Chunk* chunk = &chunks[cx][cy][cz];
				if (!chunk->is_loaded) continue;
				
				int chunk_world_x = cx * CHUNK_SIZE;  // Use cx instead of chunk->x
				int chunk_world_y = cy * CHUNK_SIZE;  // Use cy instead of chunk->y
				int chunk_world_z = cz * CHUNK_SIZE;  // Use cz instead of chunk->z
				
				for (int bx = 0; bx < CHUNK_SIZE; bx++) {
					for (int by = 0; by < CHUNK_SIZE; by++) {
						for (int bz = 0; bz < CHUNK_SIZE; bz++) {
							uint8_t light = chunk->blocks[bx][by][bz].light_level;
							
							int tex_x = chunk_world_x + bx;
							int tex_y = chunk_world_y + by;
							int tex_z = chunk_world_z + bz;
						
							size_t idx = ((size_t)tex_z * WORLD_HEIGHT_BLOCKS * WORLD_WIDTH + 
										 (size_t)tex_y * WORLD_WIDTH + 
										 (size_t)tex_x) * 4;
							
							if (idx >= texture_size) {
								fprintf(stderr, "Index out of bounds: %zu (max: %zu)\n", idx, texture_size-1);
								continue;
							}
							
							uint8_t light_value = light * 17; // 17 = 255/15
							
							// Set RGBA values
							texture_data[idx]	 = light_value;	 // R
							texture_data[idx + 1] = light_value;	 // G
							texture_data[idx + 2] = light_value;	 // B
							texture_data[idx + 3] = 255;			 // A
						}
					}
				}
			}
		}
	}
	
	return texture_data;
}

bool saveTextureSliceAsPNG(const unsigned char* texture_data, int y_slice, const char* filename) {
	const int width = WORLD_WIDTH;
	const int height = WORLD_HEIGHT_BLOCKS;
	const int depth = WORLD_DEPTH;
	
	if (y_slice < 0 || y_slice >= height) {
		fprintf(stderr, "Invalid Y slice: %d (must be 0-%d)\n", y_slice, height-1);
		return false;
	}
	
	FILE* fp = fopen(filename, "wb");
	if (!fp) {
		fprintf(stderr, "Could not open file %s for writing\n", filename);
		return false;
	}
	
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fclose(fp);
		return false;
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		png_destroy_write_struct(&png_ptr, NULL);
		fclose(fp);
		return false;
	}
	
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fclose(fp);
		return false;
	}
	
	png_init_io(png_ptr, fp);
	
	png_set_IHDR(png_ptr, info_ptr, width, depth,
				 8, PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
				 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	
	png_write_info(png_ptr, info_ptr);
	
	png_bytep row = (png_bytep)malloc(width * 4);
	
	for (int z = 0; z < depth; z++) {
		for (int x = 0; x < width; x++) {
			int idx = (z * height * width + y_slice * width + x) * 4;
			row[x * 4]	 = texture_data[idx];	 // R
			row[x * 4 + 1] = texture_data[idx + 1]; // G
			row[x * 4 + 2] = texture_data[idx + 2]; // B
			row[x * 4 + 3] = texture_data[idx + 3]; // A
		}
		png_write_row(png_ptr, row);
	}
	
	free(row);
	png_write_end(png_ptr, NULL);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(fp);
	
	return true;
}