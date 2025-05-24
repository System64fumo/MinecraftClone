#version 300 es
precision mediump float;
out vec4 FragColor;
flat in int faceID;
flat in int texID;
in vec2 size;
flat in int lightData;

uniform sampler2D textureAtlas;

const vec3 faceShades[6] = vec3[6](
	vec3(1.0, 1.0, 1.0),	// Front (No shading)
	vec3(0.75, 0.75, 0.75),	// Left (Slight shading)
	vec3(1.0, 1.0, 1.0),	// Back (More shading)
	vec3(0.75, 0.75, 0.75),	// Right (Slight lightening)
	vec3(0.5, 0.5, 0.5),	// Bottom (More shading)
	vec3(1.0, 1.0, 1.0)		// Top (No shading)
);

vec2 getTextureCoords() {
	int rotation = 0;
	float texSize = 16.0 / 256.0; // Size of each texture in the atlas (16x16 pixels)
	int atlasWidth = 16; // Number of textures per row in the atlas

	int textureIndex = texID - 1;
	int xOffset = textureIndex % atlasWidth; // X offset in the atlas
	int yOffset = textureIndex / atlasWidth; // Y offset in the atlas

	// Calculate the base texture coordinates
	float x = float(xOffset) * texSize;
	float y = float(yOffset) * texSize;

	// Handle tiling by using mod on texture coordinates
	vec2 tiledCoord = mod(size, 1.0);

	// Rotate UV coordinates based on rotation value
	vec2 centeredUV = tiledCoord - vec2(0.5, 0.5);
	float angle = float(rotation) * (3.14159 / 2.0);
	float s = sin(angle);
	float c = cos(angle);
	vec2 rotatedUV = vec2(
		centeredUV.x * c - centeredUV.y * s,
		centeredUV.x * s + centeredUV.y * c
	);
	vec2 finalUV = rotatedUV + vec2(0.5, 0.5);

	// Apply the final texture coordinates
	return vec2(x + finalUV.x * texSize, y + finalUV.y * texSize);
}

float lightLevelToBrightness(int lightLevel) {
	return clamp(float(lightLevel) / 15.0, 0.0, 1.0);
}

void main() {
	if (texID == 0) {
		FragColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
		return;
	}

	vec4 textureColor = texture(textureAtlas, getTextureCoords());
	vec3 faceShade = faceShades[faceID];

	float brightness = lightLevelToBrightness(lightData);
	vec3 litColor = textureColor.rgb * faceShade * brightness;

	// Textures 1 and 53 are affected by biome colors
	if (texID == 1 || texID == 53) {
		vec3 biomeColor = vec3(0.569, 0.741, 0.349);
		litColor *= biomeColor;
	}

	FragColor = vec4(litColor, textureColor.a);
}