#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;
uniform sampler2D u_texture_fb_depth;
uniform int ui_state;

uniform float u_far;
uniform float u_sky_brightness;
uniform mat4 u_inv_projection;
uniform mat4 u_inv_view;

const float SKYBOX_DEPTH  = 0.9999999;
const float CLOUDS_HEIGHT = 192.0;

vec3 getWorldPosition(vec2 tc, float depth) {
	vec4 ndc     = vec4(tc * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec4 viewPos = u_inv_projection * ndc;
	viewPos     /= viewPos.w;
	return (u_inv_view * viewPos).xyz;
}

void main() {
	vec3  color = texture(screenTexture, TexCoords).rgb;
	float depth = texture(u_texture_fb_depth, TexCoords).r;

	if (depth < SKYBOX_DEPTH && depth > 0.0) {
		vec3  worldPos  = getWorldPosition(TexCoords, depth);
		vec3  cameraPos = vec3(u_inv_view[3][0], u_inv_view[3][1], u_inv_view[3][2]);
		float dist      = length(worldPos - cameraPos);
		bool  isCloud   = abs(worldPos.y - CLOUDS_HEIGHT) < 50.0;

		// Fog must be fully opaque before the render frontier so loading gaps
		// are never visible. World geometry fog ends at 55% of far clip,
		// matching the 0.8 * render_distance limit in MAX_RENDER_DISTANCE_SQ.
		float fogStart = isCloud ? u_far * 0.4 : u_far * 0.05;
		float fogEnd   = isCloud ? u_far * 1.2 : u_far * 0.55;

		if (dist > fogStart) {
			// Pure smoothstep — no exponent or density multiplier to avoid banding.
			float fogFactor = smoothstep(fogStart, fogEnd, dist);

			// Day fog: sky horizon blue. Night fog: near-black with slight blue tint.
			vec3 dayFogColor   = vec3(0.753, 0.847, 1.0);
			vec3 nightFogColor = vec3(0.015, 0.018, 0.035);
			vec3 fogColor      = mix(nightFogColor, dayFogColor, u_sky_brightness);

			color = mix(color, fogColor, fogFactor);
		}
	}

	// Vignette
	vec2  uv      = TexCoords * 2.0 - 1.0;
	float vignette = 1.0 - smoothstep(0.5, 2.0, length(uv)) * 0.15;
	color *= vignette;

	if (ui_state == 1) color *= 0.5;

	FragColor = vec4(color, 1.0);
}
