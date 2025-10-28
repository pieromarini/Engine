#pragma once

#include "core/core.h"
#include "core/math/math.h"
#include "core/primal_string.h"

#include "os_keys.h"
#include "platform/os/core/os_core.h"

enum OS_EventKind {
	OS_EventKind_Null,
	OS_EventKind_WindowClose,
	OS_EventKind_WindowLoseFocus,
	OS_EventKind_Press,
	OS_EventKind_Release,
	OS_EventKind_Text,
	OS_EventKind_Scroll,
	OS_EventKind_DropFile,
	OS_EventKind_COUNT
};

using OS_Modifiers = u32;
enum {
	OS_Modifier_Ctrl = (1 << 0),
	OS_Modifier_Shift = (1 << 1),
	OS_Modifier_Alt = (1 << 2),
};

struct OS_Event {
	OS_Event* next;
	OS_Event* prev;
	OSWindowHandle window;
	OS_EventKind kind;
	OS_Modifiers modifiers;
	OS_Key key;
	u32 character;
	vec2 position;
	vec2 scroll;
	String8 path;
};

struct OS_EventList {
	OS_Event* first;
	OS_Event* last;
	u64 count;
};

struct OS_ModifiersKeyPair {
	OS_Modifiers modifiers;
	OS_Key key;
};
