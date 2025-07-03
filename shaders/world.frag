#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) out vec4 FragColor;

flat in uint packedID;
in vec2 size;
in vec2 textureBase;
in float texelSize;

uniform sampler2D textureAtlas;
uniform int highlight;

const vec3 faceShades[6] = vec3[6](
	vec3(1.0),	// Front
	vec3(0.75),	// Left
	vec3(1.0),	// Back
	vec3(0.75),	// Right
	vec3(0.5),	// Bottom
	vec3(1.0)	// Top
);

const vec3 BIOME_TINT = vec3(0.569, 0.741, 0.349);

void main() {
	uint faceID = packedID & 0xFFFFu;
	uint texID = packedID >> 16;

	if (highlight == 1) {
		FragColor = vec4(0.0, 0.0, 0.0, 0.5);
		return;
	}

	vec2 finalTexCoords = textureBase + mod(size, 1.0) * texelSize;
	vec4 textureColor = texture(textureAtlas, finalTexCoords);
	
	if (textureColor.a == 0.0) discard;

	vec3 litColor = textureColor.rgb * faceShades[faceID];

	vec3 tintFactor = mix(vec3(1.0), BIOME_TINT, float(texID == 1u || texID == 53u));
	litColor *= tintFactor;

	FragColor = vec4(litColor, textureColor.a);
}