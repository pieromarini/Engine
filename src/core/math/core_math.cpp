#include "core_math.h"

// -- vec2 operators
vec2 operator+(vec2 a, vec2 b) {
	return vec2{ a.x + b.x, a.y + b.y };
}

vec2 operator-(vec2 a, vec2 b) {
	return vec2{ a.x - b.x, a.y - b.y };
}

vec2 operator*(vec2 a, vec2 b) {
	return vec2{ a.x * b.x, a.y * b.y };
}

vec2 operator*(vec2 a, f32 b) {
	return vec2{ a.x * b, a.y * b };
}

vec2 operator/(vec2 a, f32 b) {
	return vec2{ a.x / b, a.y / b };
}

f32 vecDotProduct(vec2 a, vec2 b) {
	return a.x * b.x + a.y * b.y;
}

f32 vecLengthSquared(vec2 a) {
	return vecDotProduct(a, a);
}

f32 vecLength(vec2 a) {
	return SquareRootF32(vecDotProduct(a, a));
}

vec2 vecNormalize(vec2 a) {
	return a / vecLength(a);
}

vec2 mix(vec2 a, vec2 b, f32 t) {
	return vec2{ a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t };
}

// -- vec3 operators
vec3 operator+(vec3 a, vec3 b) {
	return vec3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

vec3 operator-(vec3 a, vec3 b) {
	return vec3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

vec3 operator*(vec3 a, vec3 b) {
	return vec3{ a.x * b.x, a.y * b.y, a.z * b.z };
}

vec3 operator*(vec3 a, f32 b) {
	return vec3{ a.x * b, a.y * b, a.z * b };
}

vec3 operator/(vec3 a, f32 b) {
	return vec3{ a.x / b, a.y / b, a.z / b };
}

f32 vecDotProduct(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3 vecCrossProduct(vec3 a, vec3 b) {
	return vec3{ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}

f32 vecLengthSquared(vec3 a) {
	return vecDotProduct(a, a);
}

f32 vecLength(vec3 a) {
	return sqrtf(vecDotProduct(a, a));
}

vec3 vecNormalize(vec3 a) {
	return a / vecLength(a);
}

vec3 mix(vec3 a, vec3 b, f32 t) {
	return vec3{ a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t, a.z * (1.0f - t) + b.z * t };
}

// -- vec4 operators
vec4 operator+(vec4 a, vec4 b) {
	return vec4{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

vec4 operator-(vec4 a, vec4 b) {
	return vec4{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

vec4 operator*(vec4 a, vec4 b) {
	return vec4{ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

vec4 operator*(vec4 a, f32 b) {
	return vec4{ a.x * b, a.y * b, a.z * b, a.w * b };
}

vec4 operator/(vec4 a, f32 b) {
	return vec4{ a.x / b, a.y / b, a.z / b, a.w / b };
}

vec4 vecTransform(vec4 v, mat4 m) {
	vec4 result{};

	for(int i = 0; i < 4; i += 1) {
		result.elements[i] = (v.elements[0]*m.elements[0][i] +
													v.elements[1]*m.elements[1][i] +
													v.elements[2]*m.elements[2][i] +
													v.elements[3]*m.elements[3][i]);
	}

	return result;
}

f32 vecDotProduct(vec4 a, vec4 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

f32 vecLengthSquared(vec4 a) {
	return vecDotProduct(a, a);
}

f32 vecLength(vec4 a) {
	return sqrtf(vecDotProduct(a, a));
}

vec4 vecNormalize(vec4 a) {
	return a / vecLength(a);
}

vec4 mix(vec4 a, vec4 b, f32 t) {
	return vec4{ a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t, a.z * (1.0f - t) + b.z * t, a.w * (1.0f - t) + b.w * t };
}

// -- Region
f32 clamp1F32(Region1DF32 r, f32 v) {
	v = Clamp(r.min, v, r.max);
	return v;
}

vec2 region2DSize(Region2D r) {
	return { AbsoluteValue(r.max.x - r.min.x), AbsoluteValue(r.max.y - r.min.y) };
}

vec2 region2DCenter(Region2D r) {
	return (r.max + r.min) / 2.0f;
}

Region2D region2DUnion(Region2D a, Region2D b) {
 return {
	 .min = { Min(a.min.x, b.min.x), Min(a.min.y, b.min.y) },
   .max = { Max(a.max.x, b.max.x), Max(a.max.y, b.max.y) }
 };
}

Region2D region2DIntersect(Region2D a, Region2D b) {
	return Region2D{
		.min = vec2{ Max(a.min.x, b.min.x), Max(a.min.y, b.min.y) },
		.max = vec2{ Min(a.max.x, b.max.x), Min(a.max.y, b.max.y) }
	};
}

Region2D region2DPad(Region2D r, f32 x) {
	return { .min = r.min - vec2{ x, x }, .max = r.max + vec2{ x, x } };
}

Region2D region2DShift(Region2D r, vec2 v) {
	return { .min = r.min + v, .max = r.max + v };
}

b32 region2DContains(Region2D r, vec2 v) {
	return (r.min.x <= v.x && v.x <= r.max.x) && (r.min.y <= v.y && v.y <= r.max.y);
}

// -- mat3
mat3 mat3Diagonal(f32 d) {
	mat3 result = { {
		{ d, 0, 0 },
		{ 0, d, 0 },
		{ 0, 0, d }
	} };
	return result;
}

mat4 mat4Diagonal(f32 d) {
	mat4 result = { {
		{ d, 0, 0, 0 },
		{ 0, d, 0, 0 },
		{ 0, 0, d, 0 },
		{ 0, 0, 0, d },
	} };
	return result;
}

mat4 matrixFromArray(f32 values[16]) {
	mat4 result = mat4Diagonal(1.0f);

	memcpy(result.elements, values, 16 * sizeof(f32));
	
	return result;
}

// Transformations
mat4 matrixMakeTranslation(vec3 translation) {
	mat4 result = mat4Diagonal(1.0f);
	result.elements[3][0] = translation.x;
	result.elements[3][1] = translation.y;
	result.elements[3][2] = translation.z;
	return result;
}

mat4 matrixMakeScale(vec3 scale) {
	mat4 result = mat4Diagonal(1.0f);
	result.elements[0][0] = scale.x;
	result.elements[1][1] = scale.y;
	result.elements[2][2] = scale.z;
	return result;
}

mat4 mat4Transpose(const mat4& m) {
	mat4 result{};
	for (int i = 0; i < 4; ++i) {
		for (int j = 0; j < 4; ++j) {
			result.elements[i][j] = m.elements[j][i];
		}
	}
	return result;
}

// -- mat3 operators
mat3 operator*(mat3 a, mat3 b) {
	mat3 c = { 0 };
	for (int j = 0; j < 3; j += 1) {
		for (int i = 0; i < 3; i += 1) {
			c.elements[i][j] = (a.elements[0][j] * b.elements[i][0] + a.elements[1][j] * b.elements[i][1] + a.elements[2][j] * b.elements[i][2]);
		}
	}
	return c;
}

mat3 operator*(mat3 m, f32 scale) {
	for (int j = 0; j < 3; j += 1) {
		for (int i = 0; i < 3; i += 1) {
			m.elements[i][j] *= scale;
		}
	}
	return m;
}

// -- mat4 operators
mat4 operator*(mat4 a, mat4 b) {
	mat4 c = { 0 };
	for (int j = 0; j < 4; j += 1) {
		for (int i = 0; i < 4; i += 1) {
			c.elements[i][j] = (a.elements[0][j] * b.elements[i][0] + a.elements[1][j] * b.elements[i][1] + a.elements[2][j] * b.elements[i][2] + a.elements[3][j] * b.elements[i][3]);
		}
	}
	return c;
}

mat4 operator*(mat4 m, f32 scale) {
	for (int j = 0; j < 4; j += 1) {
		for (int i = 0; i < 4; i += 1) {
			m.elements[i][j] *= scale;
		}
	}
	return m;
}

mat4 matrixInverse(mat4 m) {
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

// Projections
mat4 matrixMakePerspective(f32 fov, f32 aspectRatio, f32 nearZ, f32 farZ) {
	mat4 result = mat4Diagonal(1.0f);

	f32 tanHalfTheta = tanf(fov / 2.0f);
	result.elements[0][0] = 1.0f / (aspectRatio * tanHalfTheta);
	result.elements[1][1] = 1.0f / tanHalfTheta;
	result.elements[2][2] = farZ / (farZ - nearZ);
	result.elements[2][3] = 1.0f;
	result.elements[3][2] = -(farZ * nearZ) / (farZ - nearZ);
	result.elements[3][3] = 0.0f;

	return result;
}

mat4 matrixMakeOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar) {
	mat4 result = mat4Diagonal(1.0f);

	result.elements[0][0] = 2.0f / (right - left);
	result.elements[1][1] = 2.0f / (top - bottom);
	result.elements[2][2] = 1.0f / (zFar - zNear);
	result.elements[3][3] = 1.0f;

	result.elements[3][0] = -(right + left) / (right - left);
	result.elements[3][1] = -(top + bottom) / (top - bottom);
	result.elements[3][2] = -zNear / (zFar - zNear);

	return result;
}

// -- Views

mat4 matrixMakeRotation(f32 pitch, f32 yaw) {
	// LEFT -> +X
	// UP   -> +Y
	quat pitchRotation = quatFromAngleAxis(pitch, vec3{ 1.0f, 0.0f, 0.0f });
	quat yawRotation = quatFromAngleAxis(yaw, vec3{ 0.0f, 1.0f, 0.0f });

	return mat4FromQuat(yawRotation) * mat4FromQuat(pitchRotation);
}

mat4 matrixMakeViewFromPitchYaw(vec3 cameraPosition, f32 pitch, f32 yaw) {
	mat4 translation = matrixMakeTranslation(cameraPosition);
	mat4 rotation = matrixMakeRotation(pitch, yaw);
	return matrixInverse(translation * rotation);
}

mat4 matrixMakeLookAt(vec3 eye, vec3 center, vec3 up) {
 mat4 result{};

 vec3 f = vecNormalize(center - eye);
 vec3 s = vecNormalize(vecCrossProduct(up, f));
 vec3 u = vecCrossProduct(f, s);

 result.elements[0][0] = s.x;
 result.elements[1][0] = s.y;
 result.elements[2][0] = s.z;
 result.elements[3][0] = -vecDotProduct(s, eye);

 result.elements[0][1] = u.x;
 result.elements[1][1] = u.y;
 result.elements[2][1] = u.z;
 result.elements[3][1] = -vecDotProduct(u, eye);

 result.elements[0][2] = f.x;
 result.elements[1][2] = f.y;
 result.elements[2][2] = f.z;
 result.elements[3][2] = -vecDotProduct(f, eye);

 result.elements[0][3] = 0.0f;
 result.elements[1][3] = 0.0f;
 result.elements[2][3] = 0.0f;
 result.elements[3][3] = 1.0f;

 return result;
}

// -- quaternion
quat operator+(quat a, quat b) {
 quat c{};

 c.x = a.x + b.x;
 c.y = a.y + b.y;
 c.z = a.z + b.z;
 c.w = a.w + b.w;

 return c;
}

quat operator-(quat a, quat b) {
 quat c{};

 c.x = a.x - b.x;
 c.y = a.y - b.y;
 c.z = a.z - b.z;
 c.w = a.w - b.w;

 return c;
}

quat operator*(quat a, quat b) {
	quat c{};

	c.x = b.elements[3] * +a.elements[0];
	c.y = b.elements[2] * -a.elements[0];
	c.z = b.elements[1] * +a.elements[0];
	c.w = b.elements[0] * -a.elements[0];
	c.x += b.elements[2] * +a.elements[1];
	c.y += b.elements[3] * +a.elements[1];
	c.z += b.elements[0] * -a.elements[1];
	c.w += b.elements[1] * -a.elements[1];
	c.x += b.elements[1] * -a.elements[2];
	c.y += b.elements[0] * +a.elements[2];
	c.z += b.elements[3] * +a.elements[2];
	c.w += b.elements[2] * -a.elements[2];
	c.x += b.elements[0] * +a.elements[3];
	c.y += b.elements[1] * +a.elements[3];
	c.z += b.elements[2] * +a.elements[3];
	c.w += b.elements[3] * +a.elements[3];

	return c;
}

quat operator*(quat a, f32 b) {
	quat result{};

	result.x = a.x * b;
	result.y = a.y * b;
	result.z = a.z * b;
	result.w = a.w * b;

	return result;
}

quat quatNormalize(quat q) {
	return q * (1.0f / vecLength(q.xyzw));
}

quat quatMix(quat a, quat b, f32 t) {
	quat c{};

	f32 t_inv = 1.0f - t;
	c.x = a.x * (t_inv) + b.x * t;
	c.y = a.y * (t_inv) + b.y * t;
	c.z = a.z * (t_inv) + b.z * t;
	c.w = a.w * (t_inv) + b.w * t;

	return c;
}

f32 quatDot(quat a, quat b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

quat quatFromAngleAxis(f32 angle, vec3 axis) {
	quat result{};

	vec3 axisNormalized = vecNormalize(axis);
	f32 sinHalfAngle = SinF32(angle / 2.0f);

	result.xyz = axisNormalized * sinHalfAngle;
	result.w = CosF32(angle / 2.0f);

	return result;
}

mat4 mat4FromQuat(quat q) {
	quat q_norm = quatNormalize(q);
	f32 xx = q_norm.x * q_norm.x;
	f32 yy = q_norm.y * q_norm.y;
	f32 zz = q_norm.z * q_norm.z;
	f32 xy = q_norm.x * q_norm.y;
	f32 xz = q_norm.x * q_norm.z;
	f32 yz = q_norm.y * q_norm.z;
	f32 wx = q_norm.w * q_norm.x;
	f32 wy = q_norm.w * q_norm.y;
	f32 wz = q_norm.w * q_norm.z;

	mat4 result{};

	result.elements[0][0] = 1.f - 2.f * (yy + zz);
	result.elements[0][1] = 2.f * (xy + wz);
	result.elements[0][2] = 2.f * (xz - wy);
	result.elements[0][3] = 0.f;
	result.elements[1][0] = 2.f * (xy - wz);
	result.elements[1][1] = 1.f - 2.f * (xx + zz);
	result.elements[1][2] = 2.f * (yz + wx);
	result.elements[1][3] = 0.f;
	result.elements[2][0] = 2.f * (xz + wy);
	result.elements[2][1] = 2.f * (yz - wx);
	result.elements[2][2] = 1.f - 2.f * (xx + yy);
	result.elements[2][3] = 0.f;
	result.elements[3][0] = 0.f;
	result.elements[3][1] = 0.f;
	result.elements[3][2] = 0.f;
	result.elements[3][3] = 1.f;

	return result;
}
