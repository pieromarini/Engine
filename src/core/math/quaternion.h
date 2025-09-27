#pragma once

#include "core/core.h"
#include "vector.h"

namespace pm {

union quat {
	vec4 xyzw;
	struct {
		f32 x;
		f32 y;
		f32 z;
		f32 w;
	};
	f32 elements[4];
};

}// namespace pm
