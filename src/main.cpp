#include "core/core_inc.h"
#include "platform/os/os_inc.h"

#include "core/core_inc.cpp"
#include "platform/os/os_inc.cpp"

void update() {
	Temp scratch = ScratchBegin();
	OS_EventList events = OS_getEvents(scratch.arena);

	for (OS_Event *event = events.first, *next = nullptr; event != nullptr; event = next) {
		next = event->next;
		if (event->kind == OS_EventKind_WindowClose) {
			OSWindowHandle window = event->window;
			OS_destroyWindow(window);
			OS_consumeEvent(&events, event);
		}
	}
	ScratchEnd(scratch);
}

void entryPoint() {
	auto arena = ArenaAllocDefault();

	OSWindowHandle window = OS_createWindow(0, vec2{ 800, 600 }, Str8L("Engine"));

	OS_windowFirstPaint(window);
	OS_windowSetRepaint(window, update);

	while(window.u64[0] != 0) {
		update();
	}

	OS_destroyWindow(window);

	arenaRelease(arena);
}
