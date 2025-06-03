#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint inFaceTexData;
layout (location = 2) in vec2 inSize;
layout (location = 3) in uint inLightData;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out int faceID;
flat out int texID;
out vec2 size;
flat out uint lightData[64];

void createCircleLightPattern(out uint lightData[64]) {
	for (int i = 0; i < 64; i++) {
		uint packedLights = 0u;
		
		for (int j = 0; j < 4; j++) {
			int index = i * 4 + j;
			int x = index % 16;
			int y = index / 16;
			vec2 pos = vec2(float(x)/15.0, float(y)/15.0);
			vec2 center = vec2(0.5, 0.5);
			float dist = distance(pos, center);
			float radius = 0.6;
			float falloff = 1.0 - smoothstep(0.0, radius, dist);
			uint lightValue = uint(floor(falloff * 15.0));
			packedLights |= (lightValue & 0xFFu) << (j * 8);
		}
		
		lightData[i] = packedLights;
	}
}

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	faceID = int(inFaceTexData >> 8);
	texID = int(inFaceTexData & 0xFFu);
	size = inSize;
	if (texID == 106) {
		for (int i = 0; i < 64; i++) {
			lightData[i] = uint(2147483647);
		}
	}
	else if (texID == 5) {
		createCircleLightPattern(lightData);
	}
	else {
		for (int i = 0; i < 64; i++) {
			lightData[i] = uint(0);
		}
	}
}