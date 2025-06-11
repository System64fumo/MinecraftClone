#version 300 es
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in uint aTexId;

uniform mat4 projection;

out vec2 TexCoord;
flat out uint TexId;

void main() {
	gl_Position = projection * vec4(aPos, 0.0, 1.0);
	TexCoord = aTexCoord;
	TexId = aTexId;
}