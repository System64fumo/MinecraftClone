// stb_perlin.h - v0.5 - perlin noise
// public domain single-file C implementation by Sean Barrett
// Contributors:
//	Jack Mott - additional noise functions
//	Jordan Peck - seeded noise
//

#ifndef STB_PERLIN_H
#define STB_PERLIN_H

#ifdef __cplusplus
extern "C" {
#endif

float stb_perlin_noise3(float x, float y, float z, int x_wrap, int y_wrap, int z_wrap);
float stb_perlin_noise3_seed(float x, float y, float z, int x_wrap, int y_wrap, int z_wrap, int seed);
float stb_perlin_ridge_noise3(float x, float y, float z, float lacunarity, float gain, float offset, int octaves);
float stb_perlin_fbm_noise3(float x, float y, float z, float lacunarity, float gain, int octaves);
float stb_perlin_turbulence_noise3(float x, float y, float z, float lacunarity, float gain, int octaves);
float stb_perlin_noise3_wrap_nonpow2(float x, float y, float z, int x_wrap, int y_wrap, int z_wrap, unsigned char seed);

#ifdef __cplusplus
}
#endif

#endif // STB_PERLIN_H