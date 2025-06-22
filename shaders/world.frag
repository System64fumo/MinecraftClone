#version 300 es
precision mediump float;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 AccumColor;
layout (location = 2) out float Revealage;

flat in uint faceID;
flat in uint texID;
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

vec2 getTextureCoords() {
	int rotation = 0;
	float texSize = 16.0 / 256.0;
	uint atlasWidth = 16u;

	uint textureIndex = texID - 1u;
	uint xOffset = textureIndex % atlasWidth;
	uint yOffset = textureIndex / atlasWidth;

	float x = float(xOffset) * texSize;
	float y = float(yOffset) * texSize;

	vec2 tiledCoord = mod(size, 1.0);

	vec2 centeredUV = tiledCoord - vec2(0.5, 0.5);
	float angle = float(rotation) * (3.14159 / 2.0);
	float s = sin(angle);
	float c = cos(angle);
	vec2 rotatedUV = vec2(
		centeredUV.x * c - centeredUV.y * s,
		centeredUV.x * s + centeredUV.y * c
	);
	vec2 finalUV = rotatedUV + vec2(0.5, 0.5);

	return vec2(x + finalUV.x * texSize, y + finalUV.y * texSize);
}

void main() {
	// Highlight
	if (texID == 0u) {
		FragColor = vec4(0, 0, 0, 1.0f);
		AccumColor = vec4(0.0);
		Revealage = 1.0;
		return;
	}

	vec4 textureColor = texture(textureAtlas, getTextureCoords());
	if (textureColor.a < 0.01) discard;

	vec3 faceShade = faceShades[faceID];
	vec3 litColor = textureColor.rgb * faceShade;

	if (texID == 1u || texID == 53u) {
		litColor *= vec3(0.569, 0.741, 0.349); // Biome tint
	}

	// Opaque - full color with lighting
	if (textureColor.a == 1.0) {
		FragColor = vec4(litColor, 1.0);
		AccumColor = vec4(0.0);
		Revealage = 1.0;
	} 
	// Transparent
	else {
		float weight = 1.0;
		AccumColor = vec4(litColor, 1.0);
		Revealage = 1.0 - textureColor.a;
		FragColor = vec4(0.0);
	}
}