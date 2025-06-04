#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint inFaceID;
layout (location = 2) in uint inTexID;
layout (location = 3) in vec2 inSize;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out uint faceID;
flat out uint texID;
out vec2 size;
flat out uint lightData[32];

void createCircleLightPattern(out uint lightData[32]) {
	for (int i = 0; i < 32; i++) {
		uint packedLights = 0u;
		
		for (int j = 0; j < 8; j++) {
			int index = i * 8 + j;
			int x = index % 16;
			int y = index / 16;
			vec2 pos = vec2(float(x)/15.0, float(y)/15.0);
			vec2 center = vec2(0.5, 0.5);
			float dist = distance(pos, center);
			float radius = 0.6;
			float falloff = 1.0 - smoothstep(0.0, radius, dist);
			uint lightValue = uint(floor(falloff * 15.0)) & 0xFu;
			packedLights |= lightValue << (j * 4);
		}
		
		lightData[i] = packedLights;
	}
}

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	faceID = inFaceID;
	texID = inTexID;
	size = inSize;
	for (int i = 0; i < 32; i++) {
		lightData[i] = 0xFFFFFFFFu;
	}

	/*if (texID == 106) {
		for (int i = 0; i < 32; i++) {
			lightData[i] = 0xFFFFFFFFu;
		}
	}
	else if (texID == 5) {
		createCircleLightPattern(lightData);
	}
	else {
		for (int i = 0; i < 32; i++) {
			lightData[i] = 0u;
		}
	}*/
}