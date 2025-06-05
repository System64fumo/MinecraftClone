#version 300 es
precision mediump float;
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
flat in uint TexId;

uniform sampler2D uiTexture;

void main() {
	uint atlas_width = 16u;
	uint tex_id_zero_based = TexId - 1u;
	uint tex_x = tex_id_zero_based % atlas_width;
	uint tex_y = tex_id_zero_based / atlas_width;

	float tx = float(tex_x * 16u) / 256.0;
	float ty = float(tex_y * 16u) / 256.0;
	float tw = 16.0 / 256.0;
	float th = 16.0 / 256.0;

	vec2 atlasCoord = TexCoord * vec2(tw, th) + vec2(tx, ty);
	vec4 texColor = texture(uiTexture, atlasCoord);

	float brightness = 1.0;

	vec3 absNormal = abs(Normal);

	if (absNormal.y > absNormal.x && absNormal.y > absNormal.z) {
		if (Normal.y > 0.0)
			brightness = 1.0; // Top face
		else
			brightness = 0.5; // Bottom face
	}
	else if (absNormal.x > absNormal.z)
		brightness = 0.8; // Left/Right face
	else
		brightness = 0.6; // Front/Back face

	vec3 litColor = texColor.rgb  * brightness;

	if (TexId == 1u || TexId == 53u) {
		litColor *= vec3(0.569, 0.741, 0.349);
	}

	FragColor = vec4(litColor, texColor.a);
}