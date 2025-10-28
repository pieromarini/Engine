#include "entry_point.h"

void mainEntryPoint(int argc, char** argv) {

	OS_gfxInit();

	// Entry point is defined by the OS layer
	entryPoint();
}
