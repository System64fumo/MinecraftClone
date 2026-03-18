#version 300 es
precision highp float;

in vec3 Normal;
out vec4 FragColor;

uniform float u_sky_brightness;

void main() {
	vec3  norm        = normalize(Normal);
	float faceShading = abs(norm.y) > 0.9 ? 1.0 : (abs(norm.x) > 0.9 ? 0.85 : 0.7);

	// Day: white clouds. Night: very dark blue-grey.
	vec3 dayColor   = vec3(1.0);
	vec3 nightColor = vec3(0.04, 0.045, 0.06);
	vec3 cloudColor = mix(nightColor, dayColor, u_sky_brightness) * faceShading;

	FragColor = vec4(cloudColor, 0.75);
}
