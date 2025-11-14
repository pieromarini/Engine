#pragma once

#include "core/core.h"

struct ProfileAnchor {
	u64 tscElapsedExclusive;
	u64 tscElapsedInclusive;
	u64 hitCount;
	const char* label;
};

struct Profiler {
	ProfileAnchor anchors[4096];
	u64 startTSC;
	u64 endTSC;
};

struct ProfileBlock {
	ProfileBlock(const char* _label, u32 _index);
	~ProfileBlock();

	const char* label;
	u64 oldTSCElapsedInclusive;
	u64 startTSC;
	u32 parentIndex;
	u32 anchorIndex;
};

#if ENABLE_PROFILING

#define NameConcat2(A, B) A##B
#define NameConcat(A, B) NameConcat2(A, B)
#define PerfBlock(Name) ProfileBlock NameConcat(Block, __LINE__)(Name, __COUNTER__ + 1);
#define PerfScope PerfBlock(__func__)

#else

#define PerfBlock(Name)
#define PerfScope

#endif

static void PrintTimeElapsed(u64 TotalTSCElapsed, ProfileAnchor* Anchor, u64 cpuFreq);

static void BeginProfile();
static void EndProfile();
