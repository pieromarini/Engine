#pragma once

#include <cstdint>

#if defined(_WIN32)
#define PLATFORM_WINDOWS
#elif defined(__linux__) || defined(__APPLE__)
#define PLATFORM_POSIX
#endif


#if LANG_CPP
# define no_name_mangle extern "C"
#else
# define no_name_mangle
#endif

# define root_global no_name_mangle
# define root_function no_name_mangle function

#ifdef PLATFORM_WINDOWS
# define exported no_name_mangle __declspec(dllexport)
#else
# define exported no_name_mangle
#endif

#ifdef PLATFORM_WINDOWS
# define imported no_name_mangle __declspec(dllimport)
#else
# define imported no_name_mangle
#endif

#if COMPILER_MSVC || (COMPILER_CLANG && defined(PLATFORM_WINDOWS))
# pragma section(".rdata$", read)
# define read_only __declspec(allocate(".rdata$"))
#elif (COMPILER_CLANG && OS_LINUX)
# define read_only __attribute__((section(".rodata")))
#else
// TODO: gcc
#endif

#if defined(__clang__)
# define Expect(expr, val) __builtin_expect((expr), (val))
#else
# define Expect(expr, val) (expr)
#endif

#define Likely(expr)   Expect(expr, 1)
#define Unlikely(expr) Expect(expr, 0)

// Custom base type names
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using b8 = i8;
using b16 = i16;
using b32 = i32;
using b64 = i64;
using f32 = float;
using f64 = double;

read_only static u8 u8Max = 0xFF;
read_only static u8 u8Min = 0;

read_only static u16 u16Max = 0xFFFF;
read_only static u16 u16Min = 0;

read_only static u32 u32Max = 0xFFFFFFFF;
read_only static u32 u32Min = 0;

read_only static u64 u64Max = 0xFFFFFFFFFFFFFFFF;
read_only static u64 u64Min = 0;

read_only static i8 i8Max = 0x7F;
read_only static i8 i8Min = -1 - 0x7F;

read_only static i16 i16Max = 0x7FFF;
read_only static i16 i16Min = -1 - 0x7FFF;

read_only static i32 i32Max = 0x7FFFFFFF;
read_only static i32 i32Min = -1 - 0x7FFFFFFF;

read_only static i64 i64Max = 0x7FFFFFFFFFFFFFFF;
read_only static i64 i64Min = -1 - 0x7FFFFFFFFFFFFFFF;

read_only static u32 signF32 = 0x80000000;
read_only static u32 exponentF32 = 0x7F800000;
read_only static u32 mantissaF32 = 0x7FFFFF;

read_only static f32 f32Max = 3.402823e+38f;
read_only static f32 f32Min = -3.402823e+38f;
read_only static f32 f32SmallestPositive = 1.1754943508e-38;
read_only static f32 f32Epsilon = 5.96046448e-8;

read_only static u64 signF64 = 0x8000000000000000ull;
read_only static u64 exponentF64 = 0x7FF0000000000000ull;
read_only static u64 mantissaF64 = 0xFFFFFFFFFFFFFull;

// integer/pointers
#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))
#define IntFromPtr(p) (u64)(((u8*)p) - 0)
#define PtrFromInt(i) (void*)(((u8*)0) + i)
#define Member(type, member_name) ((type *)0)->member_name
#define OffsetOf(type, member_name) IntFromPtr(&Member(type, member_name))
#define BaseFromMember(type, member_name, ptr) (type *)((u8 *)(ptr) - OffsetOf(type, member_name))

// Units
#define Bytes(n) (n)
#define Kilobytes(n) (n << 10)
#define Megabytes(n) (n << 20)
#define Gigabytes(n) (((u64)n) << 30)
#define Terabytes(n) (((u64)n) << 40)

#define Swap(a, b) do{ auto _swapper_ = a; a = b; b = _swapper_; }while(0)

// Clamps, min, max
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))
#define ClampTop(x, a) Min(x,a)
#define ClampBot(a, x) Max(a,x)
#define Clamp(a, x, b) (((a)>(x))?(a):((b)<(x))?(b):(x))

// Fake context
#define DeferLoop(start, end) for(int _i_ = ((start), 0); _i_ == 0; (_i_ += 1, (end)))

// Alignments
#define AlignPow2(x, b) (((x) + (b) - 1) & (~((b) - 1)))
#define AlignDownPow2(x, b) ((x) & (~((b) - 1)))

// assertions
#if defined(PLATFORM_WINDOWS)
#define BreakDebugger() __debugbreak()
#else
#define BreakDebugger() (*(volatile int *)0 = 0)
#endif

// #define AssertAlways(b) do { if(!(b)) { BreakDebugger(); } } while(0)

#undef Assert
#if DEBUG
# define Assert(b) do { if(!(b)) { BreakDebugger(); } } while(0)
#else
# define Assert(b) ((void)(b))
#endif

// #define NotImplemented AssertAlways(!"Not Implemented")
// #define InvalidPath AssertAlways(!"Invalid Path")

static f32 AbsoluteValue(f32 f) {
	union { u32 u; f32 f; } x{};
	x.f = f;
	x.u = x.u & ~signF32;
	return x.f;
}

static f64 AbsoluteValue(f64 f) {
	union { u64 u; f64 f; } x{};
	x.f = f;
	x.u = x.u & ~signF64;
	return x.f;
}

static f32 SignFromF32(f32 f) {
	return f < 0.f ? -1.f : +1.f;
}

static f64 SignFromF64(f64 f) {
	return f < 0.0 ? -1.0 : +1.0;
}

// memory copy/move/set wrappers
#define MemoryCopy(dst, src, size) memcpy((dst), (src), (size))
#define MemoryMove(dst, src, size) memmove((dst), (src), (size))
#define MemorySet(dst, byte, size) memset((dst), (byte), (size))

#define MemoryCopyStruct(dst, src)            \
	do {                                        \
		Assert(sizeof(*(dst)) == sizeof(*(src))); \
		MemoryCopy((dst), (src), sizeof(*(dst))); \
	} while (0)
#define MemoryCopyArray(dst, src)          \
	do {                                     \
		Assert(sizeof(dst) == sizeof(src));    \
		MemoryCopy((dst), (src), sizeof(src)); \
	} while (0)

#define MemoryZero(ptr, size) MemorySet((ptr), 0, (size))
#define MemoryZeroStruct(ptr) MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(arr) MemoryZero((arr), sizeof(arr))

#if COMPILER_MSVC
# if defined(__SANITIZE_ADDRESS__)
#  define ASAN_ENABLED 1
#  define no_asan __declspec(no_sanitize_address)
# else
#  define no_asan
# endif
#elif COMPILER_CLANG
# if defined(__has_feature)
#  if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#   define ASAN_ENABLED 1
#  endif
# endif
# define no_asan __attribute__((no_sanitize("address")))
#else
# define no_asan
#endif

#if ASAN_ENABLED
#pragma comment(lib, "clang_rt.asan-x86_64.lib")
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
#define AsanPoisonMemoryRegion(addr, size)   __asan_poison_memory_region((addr), (size))
#define AsanUnpoisonMemoryRegion(addr, size) __asan_unpoison_memory_region((addr), (size))
#else
# define AsanPoisonMemoryRegion(addr, size)   ((void)(addr), (void)(size))
# define AsanUnpoisonMemoryRegion(addr, size) ((void)(addr), (void)(size))
#endif

#if COMPILER_MSVC
#define per_thread __declspec(thread)
#elif COMPILER_CLANG || COMPILER_GCC
#define per_thread __thread
#else
#define per_thread
#endif

// Linked List helpers
// Based on: https://www.youtube.com/watch?v=gAijHHlyD5s
#define CheckNull(p) ((p)==0)
#define SetNull(p) ((p)=0)

#define QueuePush_NZ(f,l,n,next,zchk,zset) (zchk(f)?\
(((f)=(l)=(n)), zset((n)->next)):\
((l)->next=(n),(l)=(n),zset((n)->next)))
#define QueuePushFront_NZ(f,l,n,next,zchk,zset) (zchk(f) ? (((f) = (l) = (n)), zset((n)->next)) :\
((n)->next = (f)), ((f) = (n)))
#define QueuePop_NZ(f,l,next,zset) ((f)==(l)?\
(zset(f),zset(l)):\
((f)=(f)->next))
#define StackPush_N(f,n,next) ((n)->next=(f),(f)=(n))
#define StackPop_NZ(f,next,zchk) (zchk(f)?0:((f)=(f)->next))

#define DLLInsert_NPZ(f,l,p,n,next,prev,zchk,zset) \
(zchk(f) ? (((f) = (l) = (n)), zset((n)->next), zset((n)->prev)) :\
zchk(p) ? (zset((n)->prev), (n)->next = (f), (zchk(f) ? (0) : ((f)->prev = (n))), (f) = (n)) :\
((zchk((p)->next) ? (0) : (((p)->next->prev) = (n))), (n)->next = (p)->next, (n)->prev = (p), (p)->next = (n),\
((p) == (l) ? (l) = (n) : (0))))
#define DLLPushBack_NPZ(f,l,n,next,prev,zchk,zset) DLLInsert_NPZ(f,l,l,n,next,prev,zchk,zset)
#define DLLRemove_NPZ(f,l,n,next,prev,zchk,zset) (((f)==(n))?\
((f)=(f)->next, (zchk(f) ? (zset(l)) : zset((f)->prev))):\
((l)==(n))?\
((l)=(l)->prev, (zchk(l) ? (zset(f)) : zset((l)->next))):\
((zchk((n)->next) ? (0) : ((n)->next->prev=(n)->prev)),\
(zchk((n)->prev) ? (0) : ((n)->prev->next=(n)->next))))

#define QueuePush(f,l,n)         QueuePush_NZ(f,l,n,next,CheckNull,SetNull)
#define QueuePushFront(f,l,n)    QueuePushFront_NZ(f,l,n,next,CheckNull,SetNull)
#define QueuePop(f,l)            QueuePop_NZ(f,l,next,SetNull)
#define StackPush(f,n)           StackPush_N(f,n,next)
#define StackPop(f)              StackPop_NZ(f,next,CheckNull)
#define DLLPushBack(f,l,n)       DLLPushBack_NPZ(f,l,n,next,prev,CheckNull,SetNull)
#define DLLPushFront(f,l,n)      DLLPushBack_NPZ(l,f,n,prev,next,CheckNull,SetNull)
#define DLLInsert(f,l,p,n)       DLLInsert_NPZ(f,l,p,n,next,prev,CheckNull,SetNull)
#define DLLRemove(f,l,n)         DLLRemove_NPZ(f,l,n,next,prev,CheckNull,SetNull)

// Member offset
struct MemberOffset {
 u64 v;
};
#define MemberOff(S, member) (MemberOffset){OffsetOf(S, member)}
#define MemberOffLit(S, member) {OffsetOf(S, member)}
#define MemberFromOff(ptr, type, memoff) (*(type *)((u8 *)ptr + memoff.v))

// String types
// We want to eventually support UTF-8/16/32
// For now, we will mostly work with UTF-8 strings.

struct String8 {
	u8* str;
	u64 size;
};

struct String16 {
	u16* str;
	u64 size;
};

struct String32 {
	u32* str;
	u64 size;
};

enum Axis2D {
	Axis2D_X,
	Axis2D_Y,
	Axis2D_COUNT
};

#define Axis2D_Flip(a) ((Axis2D)(!(a)))
