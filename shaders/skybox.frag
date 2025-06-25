#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

in vec3 TexCoords;
out vec4 FragColor;

uniform samplerCube skybox;

void main() {
	FragColor = texture(skybox, TexCoords);
}