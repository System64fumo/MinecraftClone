#version 300 es
precision highp float;
precision highp int;

layout (location = 0) in vec3 aPos;
layout (location = 1) in uint inPackedData;
layout (location = 2) in uint packed_size;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out uint packedID;
out vec2 size;
out vec2 textureBase;
out float texelSize;

const float TEX_SIZE = 16.0 / 256.0;
const uint ATLAS_WIDTH = 16u;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);

	uint faceID = inPackedData & 0xFFu;
	uint texID = (inPackedData >> 8) & 0xFFu;

	packedID = (texID << 16) | faceID;
	size = vec2(
		float((packed_size & 0x1FFu)),	   // size_u (9 bits)
		float((packed_size >> 9) & 0x1FFu)   // size_v (9 bits)
	) / 16.0f;
	texelSize = TEX_SIZE;

	textureBase = vec2(0.0);
	uint textureIndex = texID - 1u;
	textureBase = vec2(
		float(textureIndex % ATLAS_WIDTH) * TEX_SIZE,
		float(textureIndex / ATLAS_WIDTH) * TEX_SIZE
	);
}