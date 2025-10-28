#pragma once


#include "core/core.h"
#include "vector.h"

#include <cmath>

struct mat3 {
	f32 elements[3][3];
};
struct mat4 {
	f32 elements[4][4];
};

inline mat3 mat3Diagonal(f32 d) {
	mat3 result = { { { d, 0, 0 },
		{ 0, d, 0 },
		{ 0, 0, d } } };
	return result;
}

inline mat4 mat4Diagonal(f32 d) {
	mat4 result = { {
		{ d, 0, 0, 0 },
		{ 0, d, 0, 0 },
		{ 0, 0, d, 0 },
		{ 0, 0, 0, d },
	} };
	return result;
}

// Transformations
inline mat4 matrixMakeTranslation(vec3 translation) {
	mat4 result = mat4Diagonal(1.0f);
	result.elements[3][0] = translation.x;
	result.elements[3][1] = translation.y;
	result.elements[3][2] = translation.z;
	return result;
}

inline mat4 matrixMakeScale(vec3 scale) {
	mat4 result = mat4Diagonal(1.0f);
	result.elements[0][0] = scale.x;
	result.elements[1][1] = scale.y;
	result.elements[2][2] = scale.z;
	return result;
}

// TODO(piero): rotation matrix

// Projections
inline mat4 matrixMakePerspective(f32 fov, f32 aspectRatio, f32 nearZ, f32 farZ) {
	mat4 result = mat4Diagonal(1.0f);
	f32 tanHalfTheta = tanf(fov / 2);
	result.elements[0][0] = 1.f / tanHalfTheta;
	result.elements[1][1] = aspectRatio / tanHalfTheta;
	result.elements[2][3] = 1.f;
	result.elements[2][2] = -(nearZ + farZ) / (nearZ - farZ);
	result.elements[3][2] = (2.f * nearZ * farZ) / (nearZ - farZ);
	result.elements[3][3] = 0.f;
	return result;
}

inline mat4 matrixMakeOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) {
	mat4 result = mat4Diagonal(1.f);

	result.elements[0][0] = 2.f / (right - left);
	result.elements[1][1] = 2.f / (top - bottom);
	result.elements[2][2] = 2.f / (farZ - nearZ);
	result.elements[3][3] = 1.f;

	result.elements[3][0] = (left + right) / (left - right);
	result.elements[3][1] = (bottom + top) / (bottom - top);
	result.elements[3][2] = (nearZ + farZ) / (nearZ - farZ);

	return result;
}

// Matrix multiplication
inline mat3 operator*(mat3 a, mat3 b) {
	mat3 c = { 0 };
	for (int j = 0; j < 3; j += 1) {
		for (int i = 0; i < 3; i += 1) {
			c.elements[i][j] = (a.elements[0][j] * b.elements[i][0] + a.elements[1][j] * b.elements[i][1] + a.elements[2][j] * b.elements[i][2]);
		}
	}
	return c;
}

inline mat3 operator*(mat3 m, f32 scale) {
	for (int j = 0; j < 3; j += 1) {
		for (int i = 0; i < 3; i += 1) {
			m.elements[i][j] *= scale;
		}
	}
	return m;
}

inline mat4 operator*(mat4 a, mat4 b) {
	mat4 c = { 0 };
	for (int j = 0; j < 4; j += 1) {
		for (int i = 0; i < 4; i += 1) {
			c.elements[i][j] = (a.elements[0][j] * b.elements[i][0] + a.elements[1][j] * b.elements[i][1] + a.elements[2][j] * b.elements[i][2] + a.elements[3][j] * b.elements[i][3]);
		}
	}
	return c;
}

inline mat4 operator*(mat4 m, f32 scale) {
	for (int j = 0; j < 4; j += 1) {
		for (int i = 0; i < 4; i += 1) {
			m.elements[i][j] *= scale;
		}
	}
	return m;
}

inline mat4 matInverse(mat4 m) {
	f32 coef00 = m.elements[2][2] * m.elements[3][3] - m.elements[3][2] * m.elements[2][3];
	f32 coef02 = m.elements[1][2] * m.elements[3][3] - m.elements[3][2] * m.elements[1][3];
	f32 coef03 = m.elements[1][2] * m.elements[2][3] - m.elements[2][2] * m.elements[1][3];
	f32 coef04 = m.elements[2][1] * m.elements[3][3] - m.elements[3][1] * m.elements[2][3];
	f32 coef06 = m.elements[1][1] * m.elements[3][3] - m.elements[3][1] * m.elements[1][3];
	f32 coef07 = m.elements[1][1] * m.elements[2][3] - m.elements[2][1] * m.elements[1][3];
	f32 coef08 = m.elements[2][1] * m.elements[3][2] - m.elements[3][1] * m.elements[2][2];
	f32 coef10 = m.elements[1][1] * m.elements[3][2] - m.elements[3][1] * m.elements[1][2];
	f32 coef11 = m.elements[1][1] * m.elements[2][2] - m.elements[2][1] * m.elements[1][2];
	f32 coef12 = m.elements[2][0] * m.elements[3][3] - m.elements[3][0] * m.elements[2][3];
	f32 coef14 = m.elements[1][0] * m.elements[3][3] - m.elements[3][0] * m.elements[1][3];
	f32 coef15 = m.elements[1][0] * m.elements[2][3] - m.elements[2][0] * m.elements[1][3];
	f32 coef16 = m.elements[2][0] * m.elements[3][2] - m.elements[3][0] * m.elements[2][2];
	f32 coef18 = m.elements[1][0] * m.elements[3][2] - m.elements[3][0] * m.elements[1][2];
	f32 coef19 = m.elements[1][0] * m.elements[2][2] - m.elements[2][0] * m.elements[1][2];
	f32 coef20 = m.elements[2][0] * m.elements[3][1] - m.elements[3][0] * m.elements[2][1];
	f32 coef22 = m.elements[1][0] * m.elements[3][1] - m.elements[3][0] * m.elements[1][1];
	f32 coef23 = m.elements[1][0] * m.elements[2][1] - m.elements[2][0] * m.elements[1][1];

	vec4 fac0 = { coef00, coef00, coef02, coef03 };
	vec4 fac1 = { coef04, coef04, coef06, coef07 };
	vec4 fac2 = { coef08, coef08, coef10, coef11 };
	vec4 fac3 = { coef12, coef12, coef14, coef15 };
	vec4 fac4 = { coef16, coef16, coef18, coef19 };
	vec4 fac5 = { coef20, coef20, coef22, coef23 };

	vec4 vec0 = { m.elements[1][0], m.elements[0][0], m.elements[0][0], m.elements[0][0] };
	vec4 vec1 = { m.elements[1][1], m.elements[0][1], m.elements[0][1], m.elements[0][1] };
	vec4 vec2 = { m.elements[1][2], m.elements[0][2], m.elements[0][2], m.elements[0][2] };
	vec4 vec3 = { m.elements[1][3], m.elements[0][3], m.elements[0][3], m.elements[0][3] };

	vec4 inv0 = ((vec1 * fac0) - (vec2 * fac1)) + (vec3 * fac2);
	vec4 inv1 = ((vec0 * fac0) - (vec2 * fac3)) + (vec3 * fac4);
	vec4 inv2 = ((vec0 * fac1) - (vec1 * fac3)) + (vec3 * fac5);
	vec4 inv3 = ((vec0 * fac2) - (vec1 * fac4)) + (vec2 * fac5);

	vec4 sign_a = { +1, -1, +1, -1 };
	vec4 sign_b = { -1, +1, -1, +1 };

	mat4 inverse{};
	for (u32 i = 0; i < 4; i += 1) {
		inverse.elements[0][i] = inv0.elements[i] * sign_a.elements[i];
		inverse.elements[1][i] = inv1.elements[i] * sign_b.elements[i];
		inverse.elements[2][i] = inv2.elements[i] * sign_a.elements[i];
		inverse.elements[3][i] = inv3.elements[i] * sign_b.elements[i];
	}

	vec4 row0 = { inverse.elements[0][0], inverse.elements[1][0], inverse.elements[2][0], inverse.elements[3][0] };
	vec4 m0 = { m.elements[0][0], m.elements[0][1], m.elements[0][2], m.elements[0][3] };
	vec4 dot0 = m0 * row0;
	f32 dot1 = (dot0.x + dot0.y) + (dot0.z + dot0.w);

	f32 oneOverDet = 1 / dot1;

	return inverse * oneOverDet;
}
