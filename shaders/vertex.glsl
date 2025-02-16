#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in int inBlockID;
layout (location = 2) in int inFaceID;
layout (location = 3) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out int blockID;
flat out int faceID;
out vec2 TexCoord;

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	blockID = inBlockID;
	faceID = inFaceID;
	TexCoord = aTexCoord;
}