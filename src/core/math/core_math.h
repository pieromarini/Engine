#pragma once

#include "core/core.h"

#include <cmath>

#define InfinityF32                 ((F32)INFINITY)

#define PiF32                       (3.1415926535897f)
#define OneOverSquareRootOfTwoPiF32 (0.3989422804f)
#define EulersNumberF32             (2.7182818284590452353602874713527f)

#define PiF64                       (3.1415926535897)
#define OneOverSquareRootOfTwoPiF64 (0.3989422804)
#define EulersNumberF64             (2.7182818284590452353602874713527)

#define FloorF32(f)        floorf(f)
#define CeilF32(f)         ceilf(f)
#define RoundF32(f)        roundf(f)
#define ModF32(x, y)       fmodf(x, y)
#define DegFromRadF32(v)   ((180.f/PiF32) * (v))
#define RadFromDegF32(v)   ((PiF32/180.f) * (v))
#define SquareRootF32(x)   sqrtf(x)
#define SinF32(v)          sinf(v)
#define CosF32(v)          cosf(v)
#define TanF32(v)          tanf(v)
#define ArcSinF32(v)       asinf(v)
#define ArcCosF32(v)       acosf(v)
#define ArcTanF32(v)       atanf(v)
#define ArcTan2F32(y, x)   atan2f((y), (x))
#define Sin2F32(v)         PowF32(Sin(v), 2)
#define Cos2F32(v)         PowF32(Cos(v), 2)
#define PowF32(b, exp)     powf(b, exp)
#define Log10F32(v)        log10f(v)
#define LogEF32(v)         logf(v)

#define FloorF64(f)        floor(f)
#define CeilF64(f)         ceil(f)
#define RoundF64(f)        round(f)
#define ModF64(x, y)       fmod(x, y)
#define DegFromRadF64(v)   ((180.0/PiF64) * (v))
#define RadFromDegF64(v)   ((PiF64/180.0) * (v))
#define SquareRootF64(x)   sqrt(x)
#define SinF64(v)          sin(v)
#define CosF64(v)          cos(v)
#define TanF64(v)          tan(v)
#define ArcSinF64(v)       asin(v)
#define ArcCosF64(v)       acos(v)
#define ArcTanF64(v)       atan(v)
#define ArcTan2F64(y, x)   atan2((y), (x))
#define Sin2F64(v)         PowF64(Sin(v), 2)
#define Cos2F64(v)         PowF64(Cos(v), 2)
#define PowF64(b, exp)     pow(b, exp)
#define Log10F64(v)        log10(v)
#define LogEF64(v)         log(v)

#define Floor(f)           FloorF32(f)
#define Ceil(f)            CeilF32(f)
#define Mod(x, y)          ModF32(x, y)
#define DegFromRad(v)      DegFromRadF32(v)
#define RadFromDeg(v)      RadFromDegF32(v)
#define SquareRoot(x)      SquareRootF32(x)
#define Sin(v)             SinF32(v)
#define Cos(v)             CosF32(v)
#define Tan(v)             TanF32(v)
#define Sin2(v)            Sin2F32(v)
#define Cos2(v)            Cos2F32(v)
#define Pow(b, exp)        PowF32(b, exp)
#define Log10(v)           Log10F32(v)
#define LogE(v)            LogEF32(v)

union vec2 {
	struct {
		f32 x;
		f32 y;
	};
	f32 elements[2];
};

union vec2i32 {
	struct {
		i32 x;
		i32 y;
	};
	i32 elements[2];
};

union vec3 {
	struct {
		f32 x;
		f32 y;
		f32 z;
	};
	struct {
		f32 r;
		f32 g;
		f32 b;
	};
	f32 elements[3];
};

union vec4 {
	struct {
		f32 x;
		f32 y;
		f32 z;
		f32 w;
	};

	struct {
		vec2 xy;
		vec2 zw;
	};

	struct {
		vec3 xyz;
		f32 _w1;
	};

	struct {
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};

	struct {
		vec3 rgb;
		f32 _w2;
	};

	f32 elements[4];
};

// -- matrix
struct mat3 {
	f32 elements[3][3];
};
struct mat4 {
	f32 elements[4][4];
};

// -- vec2 operators
vec2 operator+(vec2 a, vec2 b);
vec2 operator-(vec2 a, vec2 b);
vec2 operator*(vec2 a, vec2 b);
vec2 operator*(vec2 a, f32 b);
vec2 operator/(vec2 a, f32 b);

f32 vecDotProduct(vec2 a, vec2 b);
f32 vecLengthSquared(vec2 a);
f32 vecLength(vec2 a);
vec2 vecNormalize(vec2 a);

vec2 mix(vec2 a, vec2 b, f32 t);

// -- vec3 operators
vec3 operator+(vec3 a, vec3 b);
vec3 operator-(vec3 a, vec3 b);
vec3 operator*(vec3 a, vec3 b);
vec3 operator*(vec3 a, f32 b);
vec3 operator/(vec3 a, f32 b);

f32 vecDotProduct(vec3 a, vec3 b);
vec3 vecCrossProduct(vec3 a, vec3 b);
f32 vecLengthSquared(vec3 a);
f32 vecLength(vec3 a);
vec3 vecNormalize(vec3 a);

vec3 mix(vec3 a, vec3 b, f32 t);

// -- vec4 operators
vec4 operator+(vec4 a, vec4 b);
vec4 operator-(vec4 a, vec4 b);
vec4 operator*(vec4 a, vec4 b);
vec4 operator*(vec4 a, f32 b);
vec4 operator/(vec4 a, f32 b);

vec4 vecTransform(vec4 v, mat4 m);

f32 vecDotProduct(vec4 a, vec4 b);
f32 vecLengthSquared(vec4 a);
f32 vecLength(vec4 a);
vec4 vecNormalize(vec4 a);

vec4 mix(vec4 a, vec4 b, f32 t);


// -- Region
struct Region1DU64 {
	Region1DU64(u64 _min, u64 _max) : min(_min), max(_max) {
		if (max < min) {
			Swap(min, max);
		}
	}

	u64 min;
	u64 max;
};

struct Region1DF32 {
	Region1DF32(f32 _min, f32 _max) : min(_min), max(_max) {
		if (max < min) {
			Swap(min, max);
		}
	}

	f32 min;
	f32 max;
};

struct Region2D {
	vec2 min;
	vec2 max;
};

struct Region3D {
	vec3 min;
	vec3 max;
};

// -- Region1D
f32 clamp1F32(Region1DF32 r, f32 v);

// -- Region2D
vec2 region2DSize(Region2D r);
vec2 region2DCenter(Region2D r);
Region2D region2DUnion(Region2D a, Region2D b);
Region2D region2DIntersect(Region2D a, Region2D b);
Region2D region2DPad(Region2D r, f32 x);
Region2D region2DShift(Region2D r, vec2 v);

b32 region2DContains(Region2D r, vec2 v);

// -- matrix
mat3 mat3Diagonal(f32 d);
mat4 mat4Diagonal(f32 d);
mat4 matrixFromArray(f32 values[16]);

// -- Transformations
mat4 matrixMakeTranslation(vec3 translation);
mat4 matrixMakeScale(vec3 scale);
mat4 mat4Transpose(const mat4& m);

// -- mat3 operators
mat3 operator*(mat3 a, mat3 b);
mat3 operator*(mat3 m, f32 scale);

// -- mat4 operators
mat4 operator*(mat4 a, mat4 b);
mat4 operator*(mat4 m, f32 scale);

mat4 matrixInverse(mat4 m);

// -- Projections
mat4 matrixMakePerspective(f32 fov, f32 aspectRatio, f32 nearZ, f32 farZ);
mat4 matrixMakeOrthographic(f32 left, f32 right, f32 bottom, f32 top, f32 zNear, f32 zFar);

// -- Views
mat4 matrixMakeRotation(f32 pitch, f32 yaw);
mat4 matrixMakeViewFromPitchYaw(vec3 cameraPosition, f32 pitch, f32 yaw);
mat4 matrixMakeLookAt(vec3 eye, vec3 center, vec3 up);

// -- quaternion
union quat {
	vec4 xyzw;
	struct {
		vec3 xyz;
		f32 w;
	};
	struct {
		vec2 xy;
		vec2 zw;
	};
	struct {
		f32 x;
		vec3 yzw;
	};
	struct {
		f32 _x;
		f32 y;
		f32 z;
		f32 _w;
	};
	f32 elements[4];
};

quat operator+(quat a, quat b);
quat operator-(quat a, quat b);
quat operator*(quat a, quat b);
quat operator*(quat a, f32 b);

quat quatNormalize(quat q);

quat quatMix(quat a, quat b, f32 t);
f32 quatDot(quat a, quat b);

quat quatFromAngleAxis(f32 turns, vec3 axis);
mat4 mat4FromQuat(quat q);

