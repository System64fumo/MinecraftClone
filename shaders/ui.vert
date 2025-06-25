#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in uint aTexId;

uniform mat4 projection;
uniform sampler2D uiTexture;

out vec2 TexCoord;
flat out uint TexId;
out vec2 shadowCoord;
out vec2 texSize;

void main() {
	gl_Position = projection * vec4(aPos, 0.0, 1.0);
	TexCoord = aTexCoord;
	TexId = aTexId;
	texSize = vec2(textureSize(uiTexture, 0));
	shadowCoord = TexCoord + vec2(-1.0, -1.0) / texSize;
}