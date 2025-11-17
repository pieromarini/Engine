#include "entry_point.h"

void mainEntryPoint(int argc, char** argv) {
#if ENABLE_PROFILING
  BeginProfile();
#endif

  // Init all subsystems
  OS_init();

  Render_init();

  // Entry point is defined by the OS layer
  entryPoint();

#if ENABLE_PROFILING
  EndProfile();
#endif
}

static_assert(__COUNTER__ < ArrayCount(Profiler::anchors), "Number of profile points exceeds size of profiler::Anchors array");
