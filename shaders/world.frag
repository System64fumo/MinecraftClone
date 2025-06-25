#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 AccumColor;
layout (location = 2) out float Revealage;

flat in uint packedID; // Packed faceID and texID
in vec2 size;

uniform sampler2D textureAtlas;

const vec3 faceShades[6] = vec3[6](
	vec3(1.0, 1.0, 1.0),	// Front
	vec3(0.75, 0.75, 0.75),	// Left
	vec3(1.0, 1.0, 1.0),	// Back
	vec3(0.75, 0.75, 0.75),	// Right
	vec3(0.5, 0.5, 0.5),	// Bottom
	vec3(1.0, 1.0, 1.0)		// Top
);

vec2 getTextureCoords(uint texID) {
	const float texSize = 16.0 / 256.0;
	const uint atlasWidth = 16u;

	uint textureIndex = texID - 1u;
	uint xOffset = textureIndex % atlasWidth;
	uint yOffset = textureIndex / atlasWidth;

	float x = float(xOffset) * texSize;
	float y = float(yOffset) * texSize;

	vec2 tiledCoord = mod(size, 1.0);

	return vec2(x + tiledCoord.x * texSize, y + tiledCoord.y * texSize);
}

void main() {
	// Unpack once at start
	uint faceID = packedID & 0xFFFFu;
	uint texID = packedID >> 16;

	// Highlight
	if (texID == 0u) {
		FragColor = vec4(0, 0, 0, 1.0f);
		AccumColor = vec4(0.0);
		Revealage = 1.0;
		return;
	}

	vec4 textureColor = texture(textureAtlas, getTextureCoords(texID));
	if (textureColor.a == 0.0) discard;

	vec3 faceShade = faceShades[faceID];
	vec3 litColor = textureColor.rgb * faceShade;

	if (texID == 1u || texID == 53u)
		litColor *= vec3(0.569, 0.741, 0.349); // Biome tint

	if (textureColor.a == 1.0) {
		AccumColor = vec4(0.0);
		Revealage = 1.0;
		FragColor = vec4(litColor, 1.0);
	}
	else {
		AccumColor = vec4(litColor, 1.0);
		Revealage = 1.0 - textureColor.a;
		FragColor = vec4(0.0);
	}
}