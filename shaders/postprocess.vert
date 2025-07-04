#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
	TexCoords = aTexCoords;
	gl_Position = vec4(aPos, 0.0, 1.0);
}