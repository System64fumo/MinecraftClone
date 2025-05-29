#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in int inFaceID;
layout (location = 2) in int inTexID;
layout (location = 3) in vec2 inSize;
layout (location = 4) in int inLightData;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out int faceID;
flat out int texID;
out vec2 size;
flat out int lightData;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	faceID = inFaceID;
	texID = inTexID;
	size = inSize;
	lightData = inLightData;
}