#version 300 es
precision mediump float;
out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D screenTexture;

void main() {
	vec3 color = texture(screenTexture, TexCoords).rgb;

	vec2 uv = TexCoords * 2.0 - 1.0;
	float dist = length(uv);

	// Apply vignette effect
	float vignette = smoothstep(0.5, 2.0, dist);
	vignette = 1.0 - vignette * 0.25;
	color *= vignette;

	FragColor = vec4(color, 1.0);
}