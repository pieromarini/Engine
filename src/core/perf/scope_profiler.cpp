#include "scope_profiler.h"
#include "platform/os/core/os_core.h"
#include <cstdio>

static Profiler globalProfiler;
static u32 globalProfilerParent;


ProfileBlock::ProfileBlock(char const* _label, u32 _index) {
  parentIndex = globalProfilerParent;

  anchorIndex = _index;
  label = _label;

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

  anchor->label = label;
}

static void PrintTimeElapsed(u64 TotalTSCElapsed, ProfileAnchor* Anchor, u64 cpuFreq) {
  f64 Percent = 100.0 * ((f64)Anchor->tscElapsedExclusive / (f64)TotalTSCElapsed);
  printf("  %s[%llu]: %.4fms (%.2f%%", Anchor->label, Anchor->hitCount, 1000.0f * (f64)Anchor->tscElapsedExclusive / (f64)cpuFreq, Percent);
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
  u64 cpuFreq = OS_estimateCPUTimerFreq();

  u64 totalCPUElapsed = globalProfiler.endTSC - globalProfiler.startTSC;

  if (cpuFreq) {
    printf("\nTotal time: %0.4fms (CPU freq %.2f GHz)\n", 1000.0 * (f64)totalCPUElapsed / (f64)cpuFreq, (f32)cpuFreq / 1e9);
  }

  for (u32 index = 0; index < ArrayCount(globalProfiler.anchors); ++index) {
    ProfileAnchor* anchor = globalProfiler.anchors + index;
    if (anchor->tscElapsedInclusive) {
      PrintTimeElapsed(totalCPUElapsed, anchor, cpuFreq);
    }
  }
}
