#version 310 es
precision mediump float;
out vec4 FragColor;
flat in uint faceID;
flat in uint texID;
in vec2 size;
flat in uint lightData[32];

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

float lightLevelToBrightness(float lightLevel) {
	return clamp(lightLevel / 15.0 * 0.8 + 0.2, 0.2, 1.0);
}

float getLightValueFromData(int x, int y) {
	int index = y * 16 + x; // 0-255
	int dataIndex = index / 8; // Which uint (0-31)
	int component = index % 8; // Which nibble (0-7)
	return float((lightData[dataIndex] >> (component * 4)) & 0xFu);
}

float getSharpLightLevel(vec2 uv) {
	ivec2 pixel = ivec2(mod(uv, 16.0)); // Direct 16x16 grid
	return getLightValueFromData(pixel.x, pixel.y);
}

void main() {
	// Highlight
	if (texID == 0u) {
		FragColor = vec4(0, 0, 0, 1.0f);
		return;
	}

	vec4 textureColor = texture(textureAtlas, getTextureCoords());
	vec3 faceShade = faceShades[faceID];

	float brightness = lightLevelToBrightness(getSharpLightLevel(size));
	vec3 litColor = textureColor.rgb * faceShade * brightness;

	if (texID == 1u || texID == 53u) {
		litColor *= vec3(0.569, 0.741, 0.349); // Biome tint
	}

	FragColor = vec4(litColor, textureColor.a);
}