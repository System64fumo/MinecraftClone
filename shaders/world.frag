#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) out vec4 FragColor;

flat in uint packedID;
in vec2 size;
in vec2 textureBase;
in float texelSize;
flat in vec2 lightLevel; // x = sky light (0-1), y = block light (0-1)

uniform sampler2D textureAtlas;
uniform int highlight;
uniform float sky_brightness;

const vec3 faceShades[6] = vec3[6](
	vec3(1.0),   // Front
	vec3(0.75),  // Left
	vec3(1.0),   // Back
	vec3(0.75),  // Right
	vec3(0.5),   // Bottom
	vec3(1.0)	// Top
);

const vec3 BIOME_TINT = vec3(0.569, 0.741, 0.349);

float brightness_step(int level) {
	const float brightness[16] = float[16](
		0.05,  0.067, 0.085, 0.106,
		0.133, 0.165, 0.205, 0.256,
		0.317, 0.390, 0.473, 0.562,
		0.660, 0.762, 0.873, 1.0
	);
	return brightness[clamp(level, 0, 15)];
}

void main() {
	if (highlight == 1) {
		FragColor = vec4(0.0, 0.0, 0.0, 0.5);
		return;
	}

	uint faceID = packedID & 0xFFFFu;
	uint texID = packedID >> 16;

	vec2 finalTexCoords = textureBase + mod(size, 1.0) * texelSize;
	vec4 textureColor = texture(textureAtlas, finalTexCoords);
	
	if (textureColor.a == 0.0) discard;

	// Face directional shading
	vec3 litColor = textureColor.rgb * faceShades[faceID];

	// Biome tint
	vec3 tintFactor = mix(vec3(1.0), BIOME_TINT, float(texID == 1u || texID == 53u));
	litColor *= tintFactor;

	// Sky light is dimmed by sky_brightness (day/night)
	// Block light is always full brightness regardless of time
	int sky_level   = int(lightLevel.x * 15.0 + 0.5);
	int block_level = int(lightLevel.y * 15.0 + 0.5);

	float sky_brightness_val   = brightness_step(sky_level) * sky_brightness;
	float block_brightness_val = brightness_step(block_level);

	// Final brightness is max of sky (dimmed) and block light, with night floor
	float night_floor = 0.05;
	float brightness = max(max(sky_brightness_val, block_brightness_val), night_floor);

	litColor *= brightness;

	FragColor = vec4(litColor, textureColor.a);
}
