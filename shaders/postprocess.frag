#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D u_texture_fb_depth;
uniform sampler2D u_texture_accum;
uniform sampler2D u_texture_reveal;
uniform int ui_state; // 0 = running, 1 = paused

uniform float u_near;
uniform float u_far;

const float SKYBOX_DEPTH = 0.9999999;

float linearizeDepth(float depth) {
	return (2.0 * u_near) / (u_far + u_near - depth * (u_far - u_near));
}

void main() {
	vec3 opaqueColor = texture(screenTexture, TexCoords).rgb;
	vec3 accum = texture(u_texture_accum, TexCoords).rgb;
	float reveal = texture(u_texture_reveal, TexCoords).r;

	if (reveal == 1.0) {
		FragColor = vec4(opaqueColor, 1.0);
	}
	else {
		vec3 finalColor = mix(accum, opaqueColor, 0.5);
		FragColor = vec4(finalColor, 1.0);
	}
	
	// Apply fog to the final color
	float depth = texture(u_texture_fb_depth, TexCoords).r;
	if (depth < SKYBOX_DEPTH && depth > 0.0) {
		float fogStart = u_near * 6.0;
		float fogEnd = 1.0 * u_far;
		const float fogDensity = 4.0;
		
		float linearDepth = linearizeDepth(depth) * u_far;
		
		if (linearDepth > fogStart) {
			float fogFactor = smoothstep(fogStart, fogEnd, linearDepth);
			fogFactor = pow(fogFactor, 3.0) * fogDensity;
			fogFactor = clamp(fogFactor, 0.0, 1.0);
			
			vec3 fogColor = vec3(0.753,0.847, 1.0);
			FragColor.rgb = mix(FragColor.rgb, fogColor, fogFactor);
		}
	}

	// Apply vignette
	vec2 uv = TexCoords * 2.0 - 1.0;
	float dist = length(uv);
	float vignette = 1.0 - smoothstep(0.5, 2.0, dist) * 0.15;
	FragColor.rgb *= vignette;

	// Pause dimming
	if (ui_state == 1) {
		FragColor.rgb *= 0.5;
	}
}