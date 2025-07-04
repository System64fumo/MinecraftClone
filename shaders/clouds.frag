#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

in vec3 Normal;

out vec4 FragColor;

void main() {
	vec3 cloudColor = vec3(1.0);
	vec3 norm = normalize(Normal);

	float faceShading = 1.0;
	if (abs(norm.y) > 0.9) {
		faceShading = 1.0;
	} else if (abs(norm.x) > 0.9) {
		faceShading = 0.85;
	} else {
		faceShading = 0.7;
	}

	cloudColor *= faceShading;

	FragColor = vec4(cloudColor, 0.75);
}