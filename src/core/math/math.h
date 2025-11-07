#pragma once

#include "core/core.h"

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

#include "vector.h"
#include "region.h"
#include "matrix.h"
#include "quaternion.h"
