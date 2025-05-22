#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
uniform sampler2D uiTexture;
uniform vec4 texParams;

void main() {
	vec2 atlasCoord = TexCoord * texParams.zw + texParams.xy;
	vec4 texColor = texture(uiTexture, atlasCoord);
	
	// Apply different tinting based on face normal
	vec3 faceColor = vec3(1.0);

	if (abs(Normal.y - 1.0) < 0.1) {
		faceColor = vec3(0.5, 0.5, 0.5);
	}
	else if (abs(Normal.y + 1.0) < 0.1) {
		faceColor = vec3(1.0, 1.0, 1.0);
	}
	else if (abs(Normal.x) > 0.9) {
		faceColor = vec3(0.75, 0.75, 0.75);
	}
	else if (abs(Normal.z) > 0.9) {
		faceColor = vec3(0.5, 0.5, 0.5);
	}
	
	FragColor = vec4(texColor.rgb * faceColor, texColor.a);
}