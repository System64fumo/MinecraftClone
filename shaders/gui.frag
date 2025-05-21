#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 TexCoord;
uniform sampler2D uiTexture;
uniform vec4 texParams; // x,y = offset, z,w = size

void main() {
	vec2 atlasCoord = TexCoord * texParams.zw + texParams.xy;
	FragColor = texture(uiTexture, atlasCoord);
}