#version 300 es
precision highp float;
precision highp int;

layout(location = 0) in int   aPosX;
layout(location = 1) in uint  aPosY;
layout(location = 2) in int   aPosZ;
layout(location = 3) in uint  inPackedData;
layout(location = 4) in uint  packed_size;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out uint  packedID;
out      vec2  size;
out      vec2  textureBase;
out      float texelSize;
flat out vec2  lightLevel;

const float TEX_SIZE    = 16.0 / 256.0;
const uint  ATLAS_WIDTH = 16u;

void main() {
	vec3 pos    = vec3(float(aPosX), float(aPosY), float(aPosZ)) / 16.0;
	gl_Position = projection * view * model * vec4(pos, 1.0);

	uint faceID = inPackedData & 0xFFu;
	uint texID  = (inPackedData >> 8) & 0xFFu;
	packedID    = (texID << 16) | faceID;

	size = vec2(
		float( packed_size        & 0x1FFu),
		float((packed_size >>  9) & 0x1FFu)
	) / 16.0;

	texelSize = TEX_SIZE;

	uint ti    = texID - 1u;
	textureBase = vec2(
		float(ti % ATLAS_WIDTH) * TEX_SIZE,
		float(ti / ATLAS_WIDTH) * TEX_SIZE
	);

	uint sky_raw   = (packed_size >> 18u) & 0xFu;
	uint block_raw = (packed_size >> 22u) & 0xFu;
	lightLevel = vec2(float(sky_raw) / 15.0, float(block_raw) / 15.0);
}
