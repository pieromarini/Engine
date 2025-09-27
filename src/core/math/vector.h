#pragma once

#include "core/core.h"

#include <cmath>

namespace pm {

union vec2 {
	struct {
		f32 x;
		f32 y;
	};
	f32 elements[2];
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
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};
	f32 elements[4];
};

// -- vec2
inline vec2 operator+(vec2 a, vec2 b) {
	return vec2{ a.x + b.x, a.y + b.y };
}

inline vec2 operator-(vec2 a, vec2 b) {
	return vec2{ a.x - b.x, a.y - b.y };
}

inline vec2 operator*(vec2 a, vec2 b) {
	return vec2{ a.x * b.x, a.y * b.y };
}

inline vec2 operator*(vec2 a, f32 b) {
	return vec2{ a.x * b, a.y * b };
}

inline vec2 operator/(vec2 a, f32 b) {
	return vec2{ a.x / b, a.y / b };
}

inline f32 vecDotProduct(vec2 a, vec2 b) {
	return a.x * b.x + a.y * b.y;
}

inline f32 vecLengthSquared(vec2 a) {
	return vecDotProduct(a, a);
}

inline f32 vecLength(vec2 a) {
	return sqrtf(vecDotProduct(a, a));
}

inline vec2 vecNormalize(vec2 a) {
	return a / vecLength(a);
}

inline vec2 mix(vec2 a, vec2 b, f32 t) {
	return vec2{ a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t };
}

// -- vec3
inline vec3 operator+(vec3 a, vec3 b) {
	return vec3{ a.x + b.x, a.y + b.y, a.z + b.z };
}

inline vec3 operator-(vec3 a, vec3 b) {
	return vec3{ a.x - b.x, a.y - b.y, a.z - b.z };
}

inline vec3 operator*(vec3 a, vec3 b) {
	return vec3{ a.x * b.x, a.y * b.y, a.z * b.z };
}

inline vec3 operator*(vec3 a, f32 b) {
	return vec3{ a.x * b, a.y * b, a.z * b };
}

inline vec3 operator/(vec3 a, f32 b) {
	return vec3{ a.x / b, a.y / b, a.z / b };
}

inline f32 vecDotProduct(vec3 a, vec3 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline vec3 vecCrossProduct(vec3 a, vec3 b) {
	return vec3{ a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}

inline f32 vecLengthSquared(vec3 a) {
	return vecDotProduct(a, a);
}

inline f32 vecLength(vec3 a) {
	return sqrtf(vecDotProduct(a, a));
}

inline vec3 vecNormalize(vec3 a) {
	return a / vecLength(a);
}

inline vec3 mix(vec3 a, vec3 b, f32 t) {
	return vec3{ a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t, a.z * (1.0f - t) + b.z * t };
}

// -- vec4
inline vec4 operator+(vec4 a, vec4 b) {
	return vec4{ a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w };
}

inline vec4 operator-(vec4 a, vec4 b) {
	return vec4{ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

inline vec4 operator*(vec4 a, vec4 b) {
	return vec4{ a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w };
}

inline vec4 operator*(vec4 a, f32 b) {
	return vec4{ a.x * b, a.y * b, a.z * b, a.w * b };
}

inline vec4 operator/(vec4 a, f32 b) {
	return vec4{ a.x / b, a.y / b, a.z / b, a.w / b };
}

inline f32 vecDotProduct(vec4 a, vec4 b) {
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline f32 vecLengthSquared(vec4 a) {
	return vecDotProduct(a, a);
}

inline f32 vecLength(vec4 a) {
	return sqrtf(vecDotProduct(a, a));
}

inline vec4 vecNormalize(vec4 a) {
	return a / vecLength(a);
}

inline vec4 mix(vec4 a, vec4 b, f32 t) {
	return vec4{ a.x * (1.0f - t) + b.x * t, a.y * (1.0f - t) + b.y * t, a.z * (1.0f - t) + b.z * t, a.w * (1.0f - t) + b.w * t };
}

}// namespace pm
