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

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	faceID = inFaceID;
	texID = inTexID;
	size = inSize;
}