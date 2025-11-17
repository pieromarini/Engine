#pragma once

#include <windows.h>

#include "core/memory/arena.h"
#include "os_gfx.h"

struct Win32TitleBarClientArea {
  Win32TitleBarClientArea* next;
  Region2D rect;
};

struct Win32Window {
  Win32Window* next;
  Win32Window* prev;
  HWND hwnd;
  HDC hdc;
  OS_RepaintFunction* repaint;
  u32 width;
  u32 height;

  b32 customBorder;
  f32 customBorderTitleThickness;
  f32 customBorderEdgeThickness;
  Arena* paintArena;
  Win32TitleBarClientArea *firstTitleBarClientArea;
  Win32TitleBarClientArea *lastTitleBarClientArea;
};

struct Win32GfxState {
  HINSTANCE hInstance;
  Arena* arena;

  OS_CursorType cursorType;

  // mouse state
  vec2 lastMousePosition;
  b32 mouseInside;
  b32 mouseRelativeMode;

  Arena* windowArena;
  Win32Window* firstWindow;
  Win32Window* lastWindow;
  Win32Window* freeWindow;
};

OSWindowHandle OS_handleFromWindow(Win32Window* window);
Win32Window* OS_windowFromHandle(OSWindowHandle handle);

// Enables RAW Input from the Win32 API. Used for relative mouse movement
void OS_setRawMouseInputEnabled(OSWindowHandle handle, b32 enable);
