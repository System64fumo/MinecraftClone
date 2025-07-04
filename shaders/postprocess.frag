#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform sampler2D u_texture_fb_depth;
uniform int ui_state; // 0 = running, 1 = paused

uniform float u_far;
uniform mat4 u_inv_projection;
uniform mat4 u_inv_view;

const float SKYBOX_DEPTH = 0.9999999;
const float CLOUDS_HEIGHT = 192.0;

// Convert screen coordinates and depth to world position
vec3 getWorldPosition(vec2 texCoords, float depth) {
	// Convert to NDC coordinates
	vec4 ndc = vec4(texCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	
	// Transform to view space
	vec4 viewPos = u_inv_projection * ndc;
	viewPos /= viewPos.w;
	
	// Transform to world space
	vec4 worldPos = u_inv_view * viewPos;
	return worldPos.xyz;
}

void main() {
	vec3 color = texture(screenTexture, TexCoords).rgb;
	float depth = texture(u_texture_fb_depth, TexCoords).r;

	// Apply distance-based fog
	if (depth < SKYBOX_DEPTH && depth > 0.0) {
		vec3 worldPos = getWorldPosition(TexCoords, depth);

		vec3 cameraPos = vec3(u_inv_view[3][0], u_inv_view[3][1], u_inv_view[3][2]);

		float distance = length(worldPos - cameraPos);

		bool isCloud = abs(worldPos.y - CLOUDS_HEIGHT) < 50.0;

		float fogStart = u_far / 10.0;
		float fogEnd = u_far / 2.0;
		float fogDensity = 100.0;

		if (isCloud) {
			fogDensity /= 10.0;
			fogStart *= 4.0;
		}

		if (distance > fogStart) {
			float fogFactor = smoothstep(fogStart, fogEnd, distance);
			fogFactor = pow(fogFactor, 3.0) * fogDensity;
			fogFactor = clamp(fogFactor, 0.0, 1.0);
			
			vec3 fogColor = vec3(0.753, 0.847, 1.0);
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