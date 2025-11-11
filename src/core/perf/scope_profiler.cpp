#include "scope_profiler.h"
#include "platform/os/core/os_core.h"
#include <cstdio>

static Profiler globalProfiler;
static u32 globalProfilerParent;


ProfileBlock::ProfileBlock(char const* _label, u32 _index) {
	parentIndex = globalProfilerParent;

	anchorIndex = _index;
	Label = _label;

	ProfileAnchor* Anchor = globalProfiler.anchors + anchorIndex;
	oldTSCElapsedInclusive = Anchor->tscElapsedInclusive;

	globalProfilerParent = anchorIndex;
	startTSC = OS_readCPUTimer();
}

ProfileBlock::~ProfileBlock() {
	u64 elapsed = OS_readCPUTimer() - startTSC;
	globalProfilerParent = parentIndex;

	ProfileAnchor* parent = globalProfiler.anchors + parentIndex;
	ProfileAnchor* anchor = globalProfiler.anchors + anchorIndex;

	parent->tscElapsedExclusive -= elapsed;
	anchor->tscElapsedExclusive += elapsed;
	anchor->tscElapsedInclusive = oldTSCElapsedInclusive + elapsed;
	++anchor->hitCount;

	anchor->label = Label;
}

static void PrintTimeElapsed(u64 TotalTSCElapsed, ProfileAnchor* Anchor) {
	f64 Percent = 100.0 * ((f64)Anchor->tscElapsedExclusive / (f64)TotalTSCElapsed);
	printf("  %s[%llu]: %llu (%.2f%%", Anchor->label, Anchor->hitCount, Anchor->tscElapsedExclusive, Percent);
	if (Anchor->tscElapsedInclusive != Anchor->tscElapsedExclusive) {
		f64 PercentWithChildren = 100.0 * ((f64)Anchor->tscElapsedInclusive / (f64)TotalTSCElapsed);
		printf(", %.2f%% w/children", PercentWithChildren);
	}
	printf(")\n");
}

static void BeginProfile() {
	globalProfiler.startTSC = OS_readCPUTimer();
}

static void EndProfile() {
	globalProfiler.endTSC = OS_readCPUTimer();
	u64 CPUFreq = OS_estimateCPUTimerFreq();

	u64 TotalCPUElapsed = globalProfiler.endTSC - globalProfiler.startTSC;

	if (CPUFreq) {
		printf("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)TotalCPUElapsed / (f64)CPUFreq, CPUFreq);
	}

	for (u32 AnchorIndex = 0; AnchorIndex < ArrayCount(globalProfiler.anchors); ++AnchorIndex) {
		ProfileAnchor* Anchor = globalProfiler.anchors + AnchorIndex;
		if (Anchor->tscElapsedInclusive) {
			PrintTimeElapsed(TotalCPUElapsed, Anchor);
		}
	}
}
