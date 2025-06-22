#version 300 es
precision mediump float;
out vec4 FragColor;
in vec2 TexCoord;
flat in uint TexId;

uniform sampler2D uiTexture;

void main() {
	vec4 texColor = texture(uiTexture, TexCoord);
	if (texColor.a > 0.0) {
		FragColor = texColor;
		return;
	}

	if (TexId == 6u) {
		// Calculate shadow only if main texture is transparent
		vec2 texSize = vec2(textureSize(uiTexture, 0));
		vec2 shadowCoord = TexCoord + vec2(-1.0, -1.0) / texSize;
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