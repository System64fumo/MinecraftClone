#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in uint inFaceID;
layout (location = 2) in uint inTexID;
layout (location = 3) in vec2 inSize;
/*layout (location = 4) in uvec4 inLightLevel0;
layout (location = 5) in uvec4 inLightLevel1;
layout (location = 6) in uvec4 inLightLevel2;
layout (location = 7) in uvec4 inLightLevel3;
layout (location = 8) in uvec4 inLightLevel4;
layout (location = 9) in uvec4 inLightLevel5;
layout (location = 10) in uvec4 inLightLevel6;
layout (location = 11) in uvec4 inLightLevel7;*/

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

flat out uint faceID;
flat out uint texID;
out vec2 size;
flat out uint lightData[32];

void main() {
	gl_Position = projection * view * model * vec4(aPos, 1.0);
	faceID = inFaceID;
	texID = inTexID;
	size = inSize;

	for (int i = 0; i < 32; i++) {
		lightData[i] = 0xFFFFFFFFu;
	}

/*	lightData[0] = inLightLevel0.x;
	lightData[1] = inLightLevel0.y;
	lightData[2] = inLightLevel0.z;
	lightData[3] = inLightLevel0.w;
	lightData[4] = inLightLevel1.x;
	lightData[5] = inLightLevel1.y;
	lightData[6] = inLightLevel1.z;
	lightData[7] = inLightLevel1.w;
	lightData[8] = inLightLevel2.x;
	lightData[9] = inLightLevel2.y;
	lightData[10] = inLightLevel2.z;
	lightData[11] = inLightLevel2.w;
	lightData[12] = inLightLevel3.x;
	lightData[13] = inLightLevel3.y;
	lightData[14] = inLightLevel3.z;
	lightData[15] = inLightLevel3.w;
	lightData[16] = inLightLevel4.x;
	lightData[17] = inLightLevel4.y;
	lightData[18] = inLightLevel4.z;
	lightData[19] = inLightLevel4.w;
	lightData[20] = inLightLevel5.x;
	lightData[21] = inLightLevel5.y;
	lightData[22] = inLightLevel5.z;
	lightData[23] = inLightLevel5.w;
	lightData[24] = inLightLevel6.x;
	lightData[25] = inLightLevel6.y;
	lightData[26] = inLightLevel6.z;
	lightData[27] = inLightLevel6.w;
	lightData[28] = inLightLevel7.x;
	lightData[29] = inLightLevel7.y;
	lightData[30] = inLightLevel7.z;
	lightData[31] = inLightLevel7.w;*/
}