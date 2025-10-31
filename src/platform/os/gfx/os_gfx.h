#pragma once

#include "core/core.h"
#include "core/math/region.h"
#include "core/math/vector.h"
#include "platform/os/core/os_core.h"
#include "platform/os/os_events.h"

using OS_WindowFlags = u32;
enum {
	OS_WindowFlag_CustomBorder = (1 << 0),
};

using OS_RepaintFunction = void();

void OS_gfxInit();

OSWindowHandle OS_createWindow(OS_WindowFlags flags, vec2 size, String8 title);
void OS_destroyWindow(OSWindowHandle window);

void OS_windowMinimize(OSWindowHandle handle);
void OS_windowMaximize(OSWindowHandle handle);
void OS_windowRestore(OSWindowHandle handle);
bool OS_windowIsFocused(OSWindowHandle handle);
bool OS_windowIsFullscreen(OSWindowHandle handle);

void OS_windowFirstPaint(OSWindowHandle handle);
void OS_windowSetRepaint(OSWindowHandle handle, OS_RepaintFunction* repaint);

Rect2D OS_rectFromWindow(OSWindowHandle handle);
Rect2D OS_clientRectFromWindow(OSWindowHandle handle);
vec2 OS_mouseFromWindow(OSWindowHandle handle);

void OS_windowClearCustomBorderData(OSWindowHandle handle);
void OS_windowPushCustomTitleBar(OSWindowHandle handle, f32 thickness);
void OS_windowPushCustomEdges(OSWindowHandle handle, f32 thickness);
void OS_windowPushCustomTitlebarClientArea(OSWindowHandle handle, Rect2D rect);

OS_Modifiers OS_getModifiers();
OS_EventList OS_getEvents(Arena* arena);
void OS_consumeEvent(OS_EventList* events, OS_Event* event);
