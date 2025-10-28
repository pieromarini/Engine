#pragma once

#include "core/core.h"
#include "vector.h"

struct Rect1D {
	// Constructor to always "sort" the min/max values
	Rect1D(u64 _min, u64 _max) : min(_min), max(_max) {
		if (max < min) {
			Swap(min, max);
		}
	}

	u64 min;
	u64 max;
};

struct Rect1DF32 {
	// Constructor to always "sort" the min/max values
	Rect1DF32(f32 _min, f32 _max) : min(_min), max(_max) {
		if (max < min) {
			Swap(min, max);
		}
	}

	f32 min;
	f32 max;
};

struct Rect2D {
	vec2 min;
	vec2 max;
};

struct Rect3D {
	vec3 min;
	vec3 max;
};

inline vec2 rect2DSize(Rect2D r) {
	// return glm::abs(r.max - r.min);
}

inline vec2 rect2DCenter(Rect2D r) {
	return (r.max + r.min) / 2.0f;
}

inline Rect2D rect2DIntersect(Rect2D a, Rect2D b) {
	return Rect2D{
		.min = vec2{ Max(a.min.x, b.min.x), Max(a.min.y, b.min.y) },
		.max = vec2{ Min(a.max.x, b.max.x), Min(a.max.y, b.max.y) }
	};
}

inline Rect2D rect2DPad(Rect2D r, f32 x) {
	return { .min = r.min - vec2{ x, x }, .max = r.max + vec2{ x, x } };
}

inline Rect2D rect2DShift(Rect2D r, vec2 v) {
	return { .min = r.min + v, .max = r.max + v };
}

inline f32 clamp1F32(Rect1DF32 r, f32 v) {
	v = Clamp(r.min, v, r.max);
	return v;
}

inline b32 rect2DContains(Rect2D r, vec2 v) {
	return (r.min.x <= v.x && v.x <= r.max.x) && (r.min.y <= v.y && v.y <= r.max.y);
}
