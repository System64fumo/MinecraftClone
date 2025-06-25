#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

out vec4 FragColor;
in vec2 TexCoord;
flat in uint TexId;
in vec2 shadowCoord;
in vec2 texSize;

uniform sampler2D uiTexture;

void main() {
	vec4 texColor = texture(uiTexture, TexCoord);
	if (texColor.a > 0.0) {
		FragColor = texColor;
		return;
	}

	// Font texture atlas
	if (TexId == 1u) {
		vec4 shadowTex = texture(uiTexture, shadowCoord);
		
		if (shadowTex.a > 0.0) {
			FragColor = vec4(0.0, 0.0, 0.0, 0.5 * shadowTex.a);
		} else {
			FragColor = vec4(0.0);
		}
	} else {
		FragColor = vec4(0.0);
	}
}