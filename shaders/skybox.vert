#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main() {
	TexCoords = aPos;
	mat4 viewNoTranslation = mat4(mat3(view));
	vec4 pos = projection * viewNoTranslation * vec4(aPos, 1.0);
	gl_Position = pos.xyww;
	gl_Position.z = gl_Position.w * 0.999999; // Slightly less than far plane
}