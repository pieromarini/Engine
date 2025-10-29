#include "entry_point.h"

void mainEntryPoint(int argc, char** argv) {

	// Init all subsystems
	OS_init();

	Render_init();

	// Entry point is defined by the OS layer
	entryPoint();
}
