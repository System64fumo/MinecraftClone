#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in int inFaceID;
layout (location = 2) in int inTexID;
layout (location = 3) in vec2 inSize;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out int texID;
flat out int faceID;
out vec2 size;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	faceID = inFaceID;
	texID = inTexID;
	size = inSize;
}