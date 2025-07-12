#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
	// Generate positions and UVs from gl_VertexID (0-3)
	float x = float(gl_VertexID & 1) * 2.0 - 1.0;  // -1 or +1
	float y = float((gl_VertexID >> 1) & 1) * 2.0 - 1.0;  // -1 or +1
	TexCoords = vec2((x + 1.0) * 0.5, (y + 1.0) * 0.5);  // Convert to 0-1 UVs
	gl_Position = vec4(x, y, 0.0, 1.0);  // Direct clip-space
}