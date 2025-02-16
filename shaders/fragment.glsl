#version 330 core
out vec4 FragColor;
flat in int blockID;
flat in int faceID;
flat in int rotation;
in vec2 TexCoord;

uniform sampler2D textureAtlas;

const vec3 faceShades[6] = vec3[6](
	vec3(1.0, 1.0, 1.0),	// Front (No shading)
	vec3(0.75, 0.75, 0.75),	// Left (Slight shading)
	vec3(1.0, 1.0, 1.0),	// Back (More shading)
	vec3(0.75, 0.75, 0.75),	// Right (Slight lightening)
	vec3(0.5, 0.5, 0.5),	// Bottom (More shading)
	vec3(1.0, 1.0, 1.0)		// Top (No shading)
);

vec2 getTextureCoords() {
	float texSize = 16.0 / 256.0; // Size of each texture in the atlas (16x16 pixels)
	int atlasWidth = 16; // Number of textures per row in the atlas

	// Calculate the X and Y offsets based on blockID
	int textureIndex = blockID - 1;	// Convert blockID to 0-based index
	int xOffset = textureIndex % atlasWidth; // X offset in the atlas
	int yOffset = textureIndex / atlasWidth; // Y offset in the atlas

	// Calculate the base texture coordinates
	float x = xOffset * texSize;
	float y = yOffset * texSize; // Y increases upward in the atlas

	// Handle tiling by using mod on texture coordinates
	vec2 tiledCoord = mod(TexCoord, 1.0);

	// Rotate UV coordinates based on rotation value
	vec2 centeredUV = tiledCoord - vec2(0.5, 0.5);
	float angle = rotation * (3.14159 / 2.0);
	float s = sin(angle);
	float c = cos(angle);
	vec2 rotatedUV = vec2(
		centeredUV.x * c - centeredUV.y * s,
		centeredUV.x * s + centeredUV.y * c
	);
	vec2 finalUV = rotatedUV + vec2(0.5, 0.5);

	// Apply the final texture coordinates
	return vec2(x + finalUV.x * texSize, y + finalUV.y * texSize);
}

void main() {
	vec4 textureColor = texture(textureAtlas, getTextureCoords());
	vec3 faceShade = faceShades[faceID];

	// Textures 1 and 53 are affected by biome colors
	if (blockID == 1 || blockID == 53) {
		FragColor = vec4(textureColor.rgb * vec3(0.569, 0.741, 0.349) * faceShade, textureColor.a);
	} else {
		FragColor = vec4(textureColor.rgb * faceShade, textureColor.a);
	}
}