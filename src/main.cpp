#include "core/core_inc.h"
#include "platform/os/os_inc.h"
#include "platform/render/render_inc.h"

#include "core/core_inc.cpp"
#include "platform/os/os_inc.cpp"
#include "platform/render/render_inc.cpp"

struct Window {
	Window* next;
	Window* prev;
	OSWindowHandle handle;
};

struct State {
	Arena* arena;
	Window* firstWindow;
	Window* lastWindow;

	b32 quit;
};

per_thread State* state;

void update() {
	Temp scratch = ScratchBegin();
	OS_EventList events = OS_getEvents(scratch.arena);

	for (Window* window = state->firstWindow; window != nullptr; window = window->next) {
		Rect2D rect = OS_clientRectFromWindow(window->handle);
		vec2 size = rect2DSize(rect);
		Render_startWindow(window->handle, size);

		if (OS_windowIsMinimized(window->handle)) {
			continue;
		}

		Render_update();

		Render_endWindow(window->handle);
	}

	for (OS_Event *event = events.first, *next = nullptr; event != nullptr; event = next) {
		next = event->next;
		if (event->kind == OS_EventKind_WindowClose) {
			OSWindowHandle window = event->window;
	
			// TODO(piero): Cleanup shouldn't be done here.
			OS_destroyWindow(window);
			OS_consumeEvent(&events, event);

			// TODO(piero): We should only quit the application if we close the main window.
			state->quit = true;
		}
	}

	ScratchEnd(scratch);
}

void entryPoint() {
	auto arena = ArenaAllocDefault();
	state = PushStruct(arena, State);
	state->arena = arena;

	OSWindowHandle osWindow = OS_createWindow(0, vec2{ 800, 600 }, Str8L("Engine"));

	Render_equipWindow(osWindow);

	OS_windowFirstPaint(osWindow);
	OS_windowSetRepaint(osWindow, update);

	Window* window = PushStruct(arena, Window);
	window->handle = osWindow;

	DLLPushBack(state->firstWindow, state->lastWindow, window);

	while(!state->quit) {
		update();
	}

	arenaRelease(arena);
}
