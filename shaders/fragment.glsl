#version 330 core
out vec4 FragColor;
flat in int blockID;
flat in int faceID;
flat in int rotation;
in vec2 TexCoord;

uniform sampler2D textureAtlas;

const vec3 faceShades[6] = vec3[6](
    vec3(1.0, 1.0, 1.0),  // Front (No shading)
    vec3(0.8, 0.8, 0.8),  // Left (Slight shading)
    vec3(0.9, 0.9, 0.9),  // Back (More shading)
    vec3(0.7, 0.7, 0.7),  // Right (Slight lightening)
    vec3(0.6, 0.6, 0.6),  // Bottom (More shading)
    vec3(1.0, 1.0, 1.0)   // Top (No shading)
);

vec2 getTextureCoords() {
    float texSize = 16.0 / 256.0;
    float x = (blockID - 1) * texSize;
    float y = 1.0;

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

    return vec2(x + finalUV.x * texSize, y + finalUV.y * texSize);
}

void main() {
    vec4 textureColor = texture(textureAtlas, getTextureCoords());
    vec3 faceShade = faceShades[faceID];

    // BlockID 1 is grass and needs a biome color
    if (blockID == 1) {
        FragColor = vec4(textureColor.rgb * vec3(0.569, 0.741, 0.349) * faceShade, textureColor.a);
    } else {
        FragColor = vec4(textureColor.rgb * faceShade, textureColor.a);
    }
}
