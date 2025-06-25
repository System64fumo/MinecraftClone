#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform sampler2D u_texture_fb_depth;
uniform int ui_state; // 0 = running, 1 = paused

uniform float u_near;
uniform float u_far;

const float SKYBOX_DEPTH = 0.9999999;

float linearizeDepth(float depth) {
	return (2.0 * u_near) / (u_far + u_near - depth * (u_far - u_near));
}

void main() {
	vec3 color = texture(screenTexture, TexCoords).rgb;
	float depth = texture(u_texture_fb_depth, TexCoords).r;

	// Apply fog to the final color
	if (depth < SKYBOX_DEPTH && depth > 0.0) {
		float fogStart = u_near * 6.0;
		float fogEnd = 1.0 * u_far;
		float fogDensity = 4.0;
		
		float linearDepth = linearizeDepth(depth) * u_far;

		if (linearDepth > fogStart) {
			float fogFactor = smoothstep(fogStart, fogEnd, linearDepth);
			fogFactor = pow(fogFactor, 3.0) * fogDensity;
			fogFactor = clamp(fogFactor, 0.0, 1.0);
			
			vec3 fogColor = vec3(0.753,0.847, 1.0);
			color = mix(color, fogColor, fogFactor);
		}
	}

	// Apply vignette
	vec2 uv = TexCoords * 2.0 - 1.0;
	float dist = length(uv);
	float vignette = 1.0 - smoothstep(0.5, 2.0, dist) * 0.15;
	color *= vignette;

	// Pause dimming
	if (ui_state == 1) {
		color *= 0.5;
	}

	FragColor = vec4(color, 1.0);
}