#version 300 es
precision mediump float;
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform sampler2D u_depthTexture;
uniform int ui_state; // 0 = running, 1 = paused

uniform float u_near;
uniform float u_far;

float linearizeDepth(float depth) {
	return (2.0 * u_near) / (u_far + u_near - depth * (u_far - u_near));
}

void main() {
	vec3 color = texture(screenTexture, TexCoords).rgb;

	float depth = texture(u_depthTexture, TexCoords).r;
	float linearDepth = linearizeDepth(depth);

	float fogStart = 0.6 * u_far;
	float fogEnd = u_far;
	float fogDensity = 0.9;

	float fogFactor = (linearDepth * u_far - fogStart) / (fogEnd - fogStart);
	fogFactor = clamp(fogFactor * fogDensity, 0.0, 1.0);

	vec3 fogColor = vec3(0.471, 0.655, 1.0);
	color = mix(color, fogColor, fogFactor);

	vec2 uv = TexCoords * 2.0 - 1.0;
	float dist = length(uv);

	// Apply vignette effect
	float vignette = smoothstep(0.5, 2.0, dist);
	vignette = 1.0 - vignette * 0.25;
	color *= vignette;

	// Dim the screen when paused (ui_state == 1)
	if (ui_state == 1) {
		color *= 0.5; // Reduce brightness by 50%
	}

	FragColor = vec4(color, 1.0);
}