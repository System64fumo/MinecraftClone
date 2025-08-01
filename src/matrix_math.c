#include "main.h"
#include "entity.h"
#include <math.h>
#include <string.h>

void setup_matrices() {
	matrix4_identity(view);

	float pitch = global_entities[0].pitch * DEG_TO_RAD;
	float yaw = global_entities[0].yaw * DEG_TO_RAD;

	#if USE_ARM_OPTIMIZED_CODE

	float cos_pitch = cosf(pitch);
	float sin_pitch = sinf(pitch);
	float cos_yaw = cosf(yaw);
	float sin_yaw = sinf(yaw);

	float32x4_t f = {cos_yaw * cos_pitch, sin_pitch, sin_yaw * cos_pitch, 0.0f};
	float32x4_t len_sq = vmulq_f32(f, f);
	float len = sqrtf(vaddvq_f32(len_sq));
	f = vdivq_f32(f, vdupq_n_f32(len));

	float32x4_t s = {-vgetq_lane_f32(f, 2), 0.0f, vgetq_lane_f32(f, 0), 0.0f};
	len_sq = vmulq_f32(s, s);
	len = sqrtf(vaddvq_f32(len_sq));
	s = vdivq_f32(s, vdupq_n_f32(len));

	float32x4_t u = {
		vgetq_lane_f32(s, 1) * vgetq_lane_f32(f, 2) - vgetq_lane_f32(s, 2) * vgetq_lane_f32(f, 1),
		vgetq_lane_f32(s, 2) * vgetq_lane_f32(f, 0) - vgetq_lane_f32(s, 0) * vgetq_lane_f32(f, 2),
		vgetq_lane_f32(s, 0) * vgetq_lane_f32(f, 1) - vgetq_lane_f32(s, 1) * vgetq_lane_f32(f, 0),
		0.0f
	};

	view[0] = vgetq_lane_f32(s, 0); view[4] = vgetq_lane_f32(s, 1); view[8] = vgetq_lane_f32(s, 2);
	view[1] = vgetq_lane_f32(u, 0); view[5] = vgetq_lane_f32(u, 1); view[9] = vgetq_lane_f32(u, 2);
	view[2] = -vgetq_lane_f32(f, 0); view[6] = -vgetq_lane_f32(f, 1); view[10] = -vgetq_lane_f32(f, 2);

	float32x4_t pos = {global_entities[0].pos.x, global_entities[0].pos.y + global_entities[0].eye_level, global_entities[0].pos.z, 0.0f};
	view[12] = -vaddvq_f32(vmulq_f32(s, pos));
	view[13] = -vaddvq_f32(vmulq_f32(u, pos));
	view[14] = vaddvq_f32(vmulq_f32(f, pos));

	#else // Non ARM platforms

	float cos_pitch = cosf(pitch);
	float sin_pitch = sinf(pitch);
	float cos_yaw = cosf(yaw);
	float sin_yaw = sinf(yaw);

	float f[3] = {
		cos_yaw * cos_pitch,
		sin_pitch,
		sin_yaw * cos_pitch
	};
	float len = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
	f[0] /= len; f[1] /= len; f[2] /= len;

	float s[3] = {
		-f[2],
		0.0f,
		f[0]
	};
	len = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
	s[0] /= len; s[1] /= len; s[2] /= len;

	float u[3] = {
		s[1] * f[2] - s[2] * f[1],
		s[2] * f[0] - s[0] * f[2],
		s[0] * f[1] - s[1] * f[0]
	};

	view[0] = s[0]; view[4] = s[1]; view[8] = s[2];
	view[1] = u[0]; view[5] = u[1]; view[9] = u[2];
	view[2] = -f[0]; view[6] = -f[1]; view[10] = -f[2];

	float eye_y = global_entities[0].pos.y + global_entities[0].eye_level;
	view[12] = -(s[0] * global_entities[0].pos.x + s[1] * eye_y + s[2] * global_entities[0].pos.z);
	view[13] = -(u[0] * global_entities[0].pos.x + u[1] * eye_y + u[2] * global_entities[0].pos.z);
	view[14] = (f[0] * global_entities[0].pos.x + f[1] * eye_y + f[2] * global_entities[0].pos.z);
	#endif
}

void matrix4_identity(float* mat) {
	mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
	mat[1] = mat[2] = mat[3] = mat[4] = mat[6] = mat[7] = mat[8] = mat[9] = mat[11] = mat[12] = mat[13] = mat[14] = 0.0f;
}

void matrix4_translate(float* mat, float x, float y, float z) {
	mat[12] = x;
	mat[13] = y;
	mat[14] = z;
}

void matrix4_multiply(mat4 result, mat4 mat1, mat4 mat2) {
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			result[i * 4 + j] = 0.0f;
			for (int k = 0; k < 4; k++) {
				result[i * 4 + j] += mat1[i * 4 + k] * mat2[k * 4 + j];
			}
		}
	}
}

void matrix4_rotate(float* mat, float angle, float x, float y, float z) {
	float c = cosf(angle);
	float s = sinf(angle);
	float nc = 1 - c;

	float len = sqrtf(x*x + y*y + z*z);
	x /= len; y /= len; z /= len;

	mat[0] = x*x*nc + c;	mat[4] = x*y*nc - z*s;	mat[8] = x*z*nc + y*s;
	mat[1] = y*x*nc + z*s;	mat[5] = y*y*nc + c;	mat[9] = y*z*nc - x*s;
	mat[2] = x*z*nc - y*s;	mat[6] = y*z*nc + x*s;	mat[10] = z*z*nc + c;
}

void matrix4_ortho(mat4 out, float left, float right, float bottom, float top, float near, float far) {
	matrix4_identity(out);
	out[0] = 2.0f / (right - left);
	out[5] = 2.0f / (top - bottom);
	out[10] = -2.0f / (far - near);
	out[12] = -(right + left) / (right - left);
	out[13] = -(top + bottom) / (top - bottom);
	out[14] = -(far + near) / (far - near);
}

void matrix4_perspective(float* mat, float fovy, float aspect, float near, float far) {
	float f = 1.0f / tanf(fovy * 0.5f);

	mat[0] = f / aspect;
	mat[5] = f;
	mat[10] = (far + near) / (near - far);
	mat[11] = -1.0f;
	mat[14] = (2.0f * far * near) / (near - far);
	mat[15] = 0.0f;
}

void matrix4_scale(float* matrix, float sx, float sy, float sz) {
	mat4 temp;
	memcpy(temp, matrix, 16 * sizeof(float));
	
	// Scale the matrix columns
	for (int i = 0; i < 4; i++) {
		matrix[i]	 = temp[i] * sx;	 // Scale x column
		matrix[i + 4] = temp[i + 4] * sy; // Scale y column
		matrix[i + 8] = temp[i + 8] * sz; // Scale z column
	}
}

void matrix4_rotate_x(float* matrix, float angle) {
	matrix4_identity(matrix);
	float c = cosf(angle);
	float s = sinf(angle);

	matrix[5] = c;
	matrix[6] = -s;
	matrix[9] = s;
	matrix[10] = c;
}

void matrix4_rotate_y(float* matrix, float angle) {
	matrix4_identity(matrix);
	float c = cosf(angle);
	float s = sinf(angle);

	matrix[0] = c;
	matrix[2] = -s;
	matrix[8] = s;
	matrix[10] = c;
}

void matrix4_rotate_z(float* matrix, float angle) {
	matrix4_identity(matrix);
	float c = cosf(angle);
	float s = sinf(angle);

	matrix[0] = c;
	matrix[1] = -s;
	matrix[4] = s;
	matrix[5] = c;
}

void matrix4_inverse(const float src[16], float dst[16]) {
	float inv[16];
	float det;
	int i;

	inv[0] = src[5] * src[10] * src[15] -
			 src[5] * src[11] * src[14] -
			 src[9] * src[6] * src[15] +
			 src[9] * src[7] * src[14] +
			 src[13] * src[6] * src[11] -
			 src[13] * src[7] * src[10];

	inv[4] = -src[4] * src[10] * src[15] +
			  src[4] * src[11] * src[14] +
			  src[8] * src[6] * src[15] -
			  src[8] * src[7] * src[14] -
			  src[12] * src[6] * src[11] +
			  src[12] * src[7] * src[10];

	inv[8] = src[4] * src[9] * src[15] -
			 src[4] * src[11] * src[13] -
			 src[8] * src[5] * src[15] +
			 src[8] * src[7] * src[13] +
			 src[12] * src[5] * src[11] -
			 src[12] * src[7] * src[9];

	inv[12] = -src[4] * src[9] * src[14] +
			   src[4] * src[10] * src[13] +
			   src[8] * src[5] * src[14] -
			   src[8] * src[6] * src[13] -
			   src[12] * src[5] * src[10] +
			   src[12] * src[6] * src[9];

	inv[1] = -src[1] * src[10] * src[15] +
			  src[1] * src[11] * src[14] +
			  src[9] * src[2] * src[15] -
			  src[9] * src[3] * src[14] -
			  src[13] * src[2] * src[11] +
			  src[13] * src[3] * src[10];

	inv[5] = src[0] * src[10] * src[15] -
			 src[0] * src[11] * src[14] -
			 src[8] * src[2] * src[15] +
			 src[8] * src[3] * src[14] +
			 src[12] * src[2] * src[11] -
			 src[12] * src[3] * src[10];

	inv[9] = -src[0] * src[9] * src[15] +
			  src[0] * src[11] * src[13] +
			  src[8] * src[1] * src[15] -
			  src[8] * src[3] * src[13] -
			  src[12] * src[1] * src[11] +
			  src[12] * src[3] * src[9];

	inv[13] = src[0] * src[9] * src[14] -
			  src[0] * src[10] * src[13] -
			  src[8] * src[1] * src[14] +
			  src[8] * src[2] * src[13] +
			  src[12] * src[1] * src[10] -
			  src[12] * src[2] * src[9];

	inv[2] = src[1] * src[6] * src[15] -
			 src[1] * src[7] * src[14] -
			 src[5] * src[2] * src[15] +
			 src[5] * src[3] * src[14] +
			 src[13] * src[2] * src[7] -
			 src[13] * src[3] * src[6];

	inv[6] = -src[0] * src[6] * src[15] +
			  src[0] * src[7] * src[14] +
			  src[4] * src[2] * src[15] -
			  src[4] * src[3] * src[14] -
			  src[12] * src[2] * src[7] +
			  src[12] * src[3] * src[6];

	inv[10] = src[0] * src[5] * src[15] -
			  src[0] * src[7] * src[13] -
			  src[4] * src[1] * src[15] +
			  src[4] * src[3] * src[13] +
			  src[12] * src[1] * src[7] -
			  src[12] * src[3] * src[5];

	inv[14] = -src[0] * src[5] * src[14] +
			   src[0] * src[6] * src[13] +
			   src[4] * src[1] * src[14] -
			   src[4] * src[2] * src[13] -
			   src[12] * src[1] * src[6] +
			   src[12] * src[2] * src[5];

	inv[3] = -src[1] * src[6] * src[11] +
			  src[1] * src[7] * src[10] +
			  src[5] * src[2] * src[11] -
			  src[5] * src[3] * src[10] -
			  src[9] * src[2] * src[7] +
			  src[9] * src[3] * src[6];

	inv[7] = src[0] * src[6] * src[11] -
			 src[0] * src[7] * src[10] -
			 src[4] * src[2] * src[11] +
			 src[4] * src[3] * src[10] +
			 src[8] * src[2] * src[7] -
			 src[8] * src[3] * src[6];

	inv[11] = -src[0] * src[5] * src[11] +
			   src[0] * src[7] * src[9] +
			   src[4] * src[1] * src[11] -
			   src[4] * src[3] * src[9] -
			   src[8] * src[1] * src[7] +
			   src[8] * src[3] * src[5];

	inv[15] = src[0] * src[5] * src[10] -
			  src[0] * src[6] * src[9] -
			  src[4] * src[1] * src[10] +
			  src[4] * src[2] * src[9] +
			  src[8] * src[1] * src[6] -
			  src[8] * src[2] * src[5];

	det = src[0] * inv[0] + src[1] * inv[4] + src[2] * inv[8] + src[3] * inv[12];

	if (det == 0) {
		// Matrix is not invertible, return identity
		matrix4_identity(dst);
		return;
	}

	det = 1.0f / det;

	for (i = 0; i < 16; i++) {
		dst[i] = inv[i] * det;
	}
}