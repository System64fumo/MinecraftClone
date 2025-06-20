#version 300 es
precision mediump float;
in vec3 TexCoords;
out vec4 FragColor;
uniform samplerCube skybox;

void main() {
	FragColor = texture(skybox, TexCoords);
}