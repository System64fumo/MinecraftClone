#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D cloudsTexture;
uniform vec2 texOffset;

void main() {
	float cloud = texture(cloudsTexture, TexCoord + texOffset).r;
	if (cloud == 0.0) {
		discard;
	}
	FragColor = vec4(1.0, 1.0, 1.0, 0.5);
}