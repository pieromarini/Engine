#include <dwmapi.h>
#include <oleidl.h>
#include <windows.h>
#include <windowsx.h>
#include <Uxtheme.h>


#include "core/core.h"
#include "core/math/region.h"
#include "core/memory/arena.h"
#include "core/core_strings.h"
#include "core/thread_context.h"
#include "os_gfx.h"
#include "platform/os/os_events.h"

#include "os_gfx_win32.h"

#pragma comment(lib, "user32")
#pragma comment(lib, "dwmapi")
#pragma comment(lib, "ole32")
#pragma comment(lib, "UxTheme")

#define OS_W32_GraphicalWindowClassName L"ApplicationWindowClass"

per_thread Win32GfxState* win32GfxState = nullptr;
per_thread Arena* win32EventsArena = nullptr;
per_thread OS_EventList* win32EventsList = nullptr;

OSWindowHandle OS_handleFromWindow(Win32Window* window) {
	OSWindowHandle handle{};
	handle.u64[0] = (u64)window;
	return handle;
}

Win32Window* OS_windowFromHandle(OSWindowHandle handle) {
	Win32Window* window = (Win32Window*)handle.u64[0];
	return window;
}

static LRESULT WINAPI OS_defaultWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	LRESULT result = 0;

	OS_Event* event{ nullptr };
	Win32Window* window = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	OSWindowHandle windowHandle = OS_handleFromWindow(window);
	Temp scratch = ScratchBegin(&win32EventsArena, 1);
	OS_EventList fallbackList{};

	if (!win32EventsArena) {
		win32EventsArena = scratch.arena;
		win32EventsList = &fallbackList;
	}

	bool isRelease = false;
	Axis2D scrollAxis = Axis2D_Y;

	switch (message) {
	case WM_CLOSE: {
		event = PushStruct(win32EventsArena, OS_Event);
		event->kind = OS_EventKind_WindowClose;
		event->window = windowHandle;
	} break;

	case WM_KILLFOCUS: {
		event = PushStruct(win32EventsArena, OS_Event);
		event->kind = OS_EventKind_WindowLoseFocus;
		event->window = windowHandle;
		ReleaseCapture();
	} break;
	// mouse buttons
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP: {
		isRelease = true;
	};
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN: {
		OS_EventKind kind = isRelease ? OS_EventKind_Release : OS_EventKind_Press;
		event = PushStruct(win32EventsArena, OS_Event);
		switch (message) {
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
			event->key = OS_Key_MouseLeft;
			break;
		case WM_MBUTTONUP:
		case WM_MBUTTONDOWN:
			event->key = OS_Key_MouseMiddle;
			break;
		case WM_RBUTTONUP:
		case WM_RBUTTONDOWN:
			event->key = OS_Key_MouseRight;
			break;
		}
		event->kind = kind;
		event->window = windowHandle;
		event->position.x = (f32)(i16)LOWORD(lParam);
		event->position.y = (f32)(i16)HIWORD(lParam);
		if (isRelease) {
			ReleaseCapture();
		} else {
			SetCapture(hwnd);
		}
	} break;
	case WM_MOUSEMOVE: {
		OS_Event* event = PushStruct(win32EventsArena, OS_Event);
		event->position.x = (f32)(i16)LOWORD(lParam);
		event->position.y = (f32)(i16)HIWORD(lParam);
	} break;

	// mouse wheel
	case WM_MOUSEHWHEEL:
		scrollAxis = Axis2D_X;
		goto scroll;
	case WM_MOUSEWHEEL:
	scroll:;
		{
			POINT p;
			p.x = (i32)(i16)LOWORD(lParam);
			p.y = (i32)(i16)HIWORD(lParam);
			ScreenToClient(window->hwnd, &p);
			i16 wheelDelta = HIWORD(wParam);
			event = PushArray(win32EventsArena, OS_Event, 1);
			event->kind = OS_EventKind_Scroll;
			event->window = windowHandle;
			if (scrollAxis == Axis2D_Y && wParam & MK_SHIFT) {
				scrollAxis = Axis2D_Flip(scrollAxis);
			}
			if (wParam & MK_SHIFT) {
				event->modifiers |= OS_Modifier_Shift;
			}
			if (wParam & MK_CONTROL) {
				event->modifiers |= OS_Modifier_Ctrl;
			}
			if (wParam & MK_ALT) {
				event->modifiers |= OS_Modifier_Alt;
			}
			event->scroll.elements[scrollAxis] = -(f32)wheelDelta;
			event->position.x = (f32)p.x;
			event->position.y = (f32)p.y;
		}
		break;
	case WM_SETCURSOR: {
		if (rect2DContains(OS_clientRectFromWindow(windowHandle), OS_mouseFromWindow(windowHandle)) && win32GfxState->cursorType != OS_CursorType_Null) {
			static b32 tableInitialized = false;
			static HCURSOR cursorTable[OS_CursorType_COUNT];
			if (!tableInitialized) {
				tableInitialized = 1;
				cursorTable[OS_CursorType_Pointer] = LoadCursorA(nullptr, IDC_ARROW);
				cursorTable[OS_CursorType_Hand] = LoadCursorA(nullptr, IDC_HAND);
				cursorTable[OS_CursorType_WestEast] = LoadCursorA(nullptr, IDC_SIZEWE);
				cursorTable[OS_CursorType_NorthSouth] = LoadCursorA(nullptr, IDC_SIZENS);
				cursorTable[OS_CursorType_NorthEastSouthWest] = LoadCursorA(nullptr, IDC_SIZENESW);
				cursorTable[OS_CursorType_NorthWestSouthEast] = LoadCursorA(nullptr, IDC_SIZENWSE);
				cursorTable[OS_CursorType_AllCardinalDirections] = LoadCursorA(nullptr, IDC_SIZEALL);
				cursorTable[OS_CursorType_IBar] = LoadCursorA(nullptr, IDC_IBEAM);
				cursorTable[OS_CursorType_Blocked] = LoadCursorA(nullptr, IDC_NO);
				cursorTable[OS_CursorType_Loading] = LoadCursorA(nullptr, IDC_WAIT);
				cursorTable[OS_CursorType_Pan] = LoadCursorA(nullptr, IDC_SIZEALL);
			}
			if (win32GfxState->cursorType == OS_CursorType_Hidden) {
				ShowCursor(0);
			} else {
				ShowCursor(1);
				SetCursor(cursorTable[win32GfxState->cursorType]);
			}
		} else {
			result = DefWindowProcW(hwnd, message, wParam, lParam);
		}
	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP: {
		DefWindowProcW(hwnd, message, wParam, lParam);
	};
	case WM_KEYDOWN:
	case WM_KEYUP: {
		b32 wasDown = !!(lParam & (1 << 30));
		b32 isDown = !(lParam & (1 << 31));
		OS_EventKind kind = isDown ? OS_EventKind_Press : OS_EventKind_Release;

		static OS_Key keyTable[256] = {};
		static b32 keyTableInitialized = false;

		if (!keyTableInitialized) {
			keyTableInitialized = 1;

			for (u32 i = 'A', j = OS_Key_A; i <= 'Z'; i += 1, j += 1) {
				keyTable[i] = (OS_Key)j;
			}
			for (u32 i = '0', j = OS_Key_0; i <= '9'; i += 1, j += 1) {
				keyTable[i] = (OS_Key)j;
			}
			for (u32 i = VK_F1, j = OS_Key_F1; i <= VK_F24; i += 1, j += 1) {
				keyTable[i] = (OS_Key)j;
			}

			keyTable[VK_ESCAPE] = OS_Key_Esc;
			keyTable[VK_OEM_3] = OS_Key_GraveAccent;
			keyTable[VK_OEM_MINUS] = OS_Key_Minus;
			keyTable[VK_OEM_PLUS] = OS_Key_Equal;
			keyTable[VK_BACK] = OS_Key_Backspace;
			keyTable[VK_TAB] = OS_Key_Tab;
			keyTable[VK_SPACE] = OS_Key_Space;
			keyTable[VK_RETURN] = OS_Key_Enter;
			keyTable[VK_CONTROL] = OS_Key_Ctrl;
			keyTable[VK_SHIFT] = OS_Key_Shift;
			keyTable[VK_MENU] = OS_Key_Alt;
			keyTable[VK_UP] = OS_Key_Up;
			keyTable[VK_LEFT] = OS_Key_Left;
			keyTable[VK_DOWN] = OS_Key_Down;
			keyTable[VK_RIGHT] = OS_Key_Right;
			keyTable[VK_DELETE] = OS_Key_Delete;
			keyTable[VK_PRIOR] = OS_Key_PageUp;
			keyTable[VK_NEXT] = OS_Key_PageDown;
			keyTable[VK_HOME] = OS_Key_Home;
			keyTable[VK_END] = OS_Key_End;
			keyTable[VK_OEM_2] = OS_Key_ForwardSlash;
			keyTable[VK_OEM_PERIOD] = OS_Key_Period;
			keyTable[VK_OEM_COMMA] = OS_Key_Comma;
			keyTable[VK_OEM_7] = OS_Key_Quote;
			keyTable[VK_OEM_4] = OS_Key_LeftBracket;
			keyTable[VK_OEM_6] = OS_Key_RightBracket;
			keyTable[VK_INSERT] = OS_Key_Insert;
			keyTable[VK_OEM_1] = OS_Key_Semicolon;
		}

		OS_Key key = OS_Key_Null;
		if (wParam < ArrayCount(keyTable)) {
			key = keyTable[wParam];
		}

		event = PushArray(win32EventsArena, OS_Event, 1);
		event->kind = kind;
		event->window = windowHandle;
		event->key = key;
	} break;

	case WM_SYSCOMMAND: {
		switch (wParam) {
		case SC_CLOSE: {
			event = PushArray(win32EventsArena, OS_Event, 1);
			event->kind = OS_EventKind_WindowClose;
			event->window = windowHandle;
		} break;
		case SC_KEYMENU: {
		} break;
		default: {
			result = DefWindowProcW(hwnd, message, wParam, lParam);
		} break;
		}
	} break;

	case WM_CHAR:
	case WM_SYSCHAR: {
		u32 charInput = wParam;
		if (charInput == '\r') {
			charInput = '\n';
		}
		if ((charInput >= 32 && charInput != 127) || charInput == '\t' || charInput == '\n') {
			event = PushArray(win32EventsArena, OS_Event, 1);
			event->kind = OS_EventKind_Text;
			event->window = windowHandle;
			event->character = charInput;
		}
	} break;

	case WM_DPICHANGED: {
		f32 newDPI = (f32)wParam;
		result = DefWindowProcW(hwnd, message, wParam, lParam);
	} break;

	case WM_SIZE:
	case WM_PAINT: {
		if (window && window->repaint) {
			PAINTSTRUCT ps;
			BeginPaint(hwnd, &ps);
			window->repaint();
			EndPaint(hwnd, &ps);
		}
		result = DefWindowProcW(hwnd, message, wParam, lParam);
	} break;

	case WM_NCACTIVATE: {
		if (!window || !window->customBorder) {
			result = DefWindowProcW(hwnd, message, wParam, lParam);
		} else {
			result = 1;
		}
	} break;
	case WM_NCCALCSIZE: {
		if (!window || !window->customBorder) {
			result = DefWindowProcW(hwnd, message, wParam, lParam);
		} else {
			MARGINS m = { 0, 0, 0, 0 };
			RECT* r = (RECT*)lParam;
			if (IsZoomed(hwnd)) {
				int xPushIn = GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
				int yPushIn = GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
				r->left += xPushIn;
				r->top += yPushIn;
				r->bottom -= xPushIn;
				r->right -= yPushIn;
				m.cxLeftWidth = m.cxRightWidth = xPushIn;
				m.cyTopHeight = m.cyBottomHeight = yPushIn;
			}
			DwmExtendFrameIntoClientArea(hwnd, &m);
		}
	} break;
	case WM_NCHITTEST: {
		if (!window || !window->customBorder) {
			result = DefWindowProcW(hwnd, message, wParam, lParam);
		} else {
			POINT posMonitor;
			posMonitor.x = GET_X_LPARAM(lParam);
			posMonitor.y = GET_Y_LPARAM(lParam);
			POINT posClient = posMonitor;
			ScreenToClient(hwnd, &posClient);

			RECT frame_rect;
			GetWindowRect(hwnd, &frame_rect);
			b32 isOverWindow = (frame_rect.left <= posMonitor.x && posMonitor.x < frame_rect.right && frame_rect.top <= posMonitor.y && posMonitor.y < frame_rect.bottom);

			b32 isOverLeft = 0;
			b32 isOverRight = 0;
			b32 isOverTop = 0;
			b32 isOverBottom = 0;
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				if (!IsZoomed(hwnd)) {
					if (rect.left <= posClient.x && posClient.x < rect.left + window->customBorderEdgeThickness) {
						isOverLeft = 1;
					}
					if (rect.right - window->customBorderEdgeThickness <= posClient.x && posClient.x < rect.right) {
						isOverRight = 1;
					}
					if (rect.bottom - window->customBorderEdgeThickness <= posClient.y && posClient.y < rect.bottom) {
						isOverBottom = 1;
					}
					if (rect.top <= posClient.y && posClient.y < rect.top + window->customBorderEdgeThickness) {
						isOverTop = 1;
					}
				}
			}

			// check against title bar
			b32 isOverTitleBar = 0;
			{
				RECT rect;
				GetClientRect(hwnd, &rect);
				isOverTitleBar = (rect.left <= posClient.x && posClient.x < rect.right && rect.top <= posClient.y && posClient.y < rect.top + window->customBorderTitleThickness);
			}

			// check against title bar client areas
			b32 isOverTitleBarClientArea = 0;
			for (Win32TitleBarClientArea* area = window->firstTitleBarClientArea; area != nullptr; area = area->next) {
				Rect2D rect = area->rect;
				if (rect.min.x <= posClient.x && posClient.x < rect.max.x && rect.min.y <= posClient.y && posClient.y < rect.max.y) {
					isOverTitleBarClientArea = 1;
					break;
				}
			}

			// resolve hovering to result
			result = HTNOWHERE;
			if (isOverWindow) {
				// default to client area
				result = HTCLIENT;

				// title bar
				if (isOverTitleBar) {
					result = HTCAPTION;
				}

				// title bar client area
				if (isOverTitleBarClientArea) {
					result = HTCLIENT;
				}

				// normal edges
				if (isOverLeft) {
					result = HTLEFT;
				}
				if (isOverRight) {
					result = HTRIGHT;
				}
				if (isOverTop) {
					result = HTTOP;
				}
				if (isOverBottom) {
					result = HTBOTTOM;
				}

				// corners
				if (isOverLeft && isOverTop) {
					result = HTTOPLEFT;
				}
				if (isOverLeft && isOverBottom) {
					result = HTBOTTOMLEFT;
				}
				if (isOverRight && isOverTop) {
					result = HTTOPRIGHT;
				}
				if (isOverRight && isOverBottom) {
					result = HTBOTTOMRIGHT;
				}
			}
		}
	} break;

	default: {
		result = DefWindowProc(hwnd, message, wParam, lParam);
	} break;
	}

	if (event) {
		event->modifiers = OS_getModifiers();
		DLLPushBack(win32EventsList->first, win32EventsList->last, event);
		win32EventsList->count += 1;
	}

	ScratchEnd(scratch);

	return result;
}

void OS_gfxInit() {
	Arena* arena = arenaAlloc(Gigabytes(1));
	win32GfxState = PushStruct(arena, Win32GfxState);
	win32GfxState->arena = arena;
	win32GfxState->windowArena = arenaAlloc(Gigabytes(1));
	win32GfxState->hInstance = GetModuleHandle(nullptr);

	// Register window class
	WNDCLASSW wc = {};
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = OS_defaultWindowProc;
	wc.hInstance = win32GfxState->hInstance;
	wc.lpszClassName = OS_W32_GraphicalWindowClassName;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClassW(&wc);
}

OSWindowHandle OS_createWindow(OS_WindowFlags flags, vec2 size, String8 title) {
	Win32Window* window = win32GfxState->freeWindow;
	if (window) {
		StackPop(win32GfxState->freeWindow);
	} else {
		window = PushStructNoZero(win32GfxState->windowArena, Win32Window);
	}
	MemoryZeroStruct(window);
	DLLPushBack(win32GfxState->firstWindow, win32GfxState->lastWindow, window);

	HWND hwnd = nullptr;
	HDC hdc = nullptr;

	Temp scratch = ScratchBegin(nullptr);

	String16 title16 = Str16From8(scratch.arena, title);
	hwnd = CreateWindowExW(0, OS_W32_GraphicalWindowClassName, (LPCWSTR)title16.str, WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, (i32)size.x, (i32)size.y, nullptr, nullptr, win32GfxState->hInstance, nullptr);
	hdc = GetDC(hwnd);
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);

	ScratchEnd(scratch);

	window->hwnd = hwnd;
	window->hdc = hdc;
	window->width = (u32)size.x;
	window->height = (u32)size.y;

	if (flags & OS_WindowFlag_CustomBorder) {
		SetWindowTheme(hwnd, L" ", L" ");
		window->customBorder = true;
		window->paintArena = arenaAlloc(Kilobytes(4));
	}

	OSWindowHandle windowHandle = OS_handleFromWindow(window);

	return windowHandle;
}

void OS_destroyWindow(OSWindowHandle handle) {
	Win32Window* window = OS_windowFromHandle(handle);
	DLLRemove(win32GfxState->firstWindow, win32GfxState->lastWindow, window);
	StackPush(win32GfxState->freeWindow, window);
	if (window->hdc) {
		ReleaseDC(window->hwnd, window->hdc);
	}
	if (window->hwnd) {
		DestroyWindow(window->hwnd);
	}
	if (window->paintArena) {
		arenaRelease(window->paintArena);
	}
}

void OS_windowMinimize(OSWindowHandle handle) {
	Win32Window* window = OS_windowFromHandle(handle);
	ShowWindow(window->hwnd, SW_MINIMIZE);
}

void OS_windowMaximize(OSWindowHandle handle) {
	Win32Window* window = OS_windowFromHandle(handle);
	ShowWindow(window->hwnd, SW_MAXIMIZE);
}

void OS_windowRestore(OSWindowHandle handle) {
	Win32Window* window = OS_windowFromHandle(handle);
	ShowWindow(window->hwnd, SW_RESTORE);
}

bool OS_windowIsFocused(OSWindowHandle handle) {
	bool result = false;
	Win32Window* window = OS_windowFromHandle(handle);
	if (window) {
		result = GetForegroundWindow() == window->hwnd;
	}
	return result;
}

bool OS_windowIsMinimized(OSWindowHandle handle) {
	bool result = false;
	Win32Window* window = OS_windowFromHandle(handle);

	if (window) {
		result = !!IsIconic(window->hwnd);
	}

	return result;
}

bool OS_windowIsFullscreen(OSWindowHandle handle) {
	Win32Window* window = OS_windowFromHandle(handle);
	HWND hwnd = window->hwnd;
	DWORD windowStyle = GetWindowLong(hwnd, GWL_STYLE);
	b32 isFullscreen = true;
	if (!(windowStyle & WS_OVERLAPPEDWINDOW)) {
		isFullscreen = false;
	}
	return isFullscreen;
}

void OS_windowFirstPaint(OSWindowHandle handle) {
	Temp scratch = ScratchBegin();
	OS_EventList evts{};
	win32EventsList = &evts;
	win32EventsArena = scratch.arena;
	Win32Window* window = OS_windowFromHandle(handle);
	ShowWindow(window->hwnd, SW_SHOW);
	UpdateWindow(window->hwnd);
	ScratchEnd(scratch);
}

void OS_windowSetRepaint(OSWindowHandle handle, OS_RepaintFunction* repaint) {
	Win32Window* window = OS_windowFromHandle(handle);
	window->repaint = repaint;
}

Rect2D OS_rectFromWindow(OSWindowHandle handle) {
	Rect2D rect{};
	Win32Window* window = OS_windowFromHandle(handle);
	if (window) {
		RECT w32Rect{};
		if (GetWindowRect(window->hwnd, &w32Rect)) {
			rect.min = { (f32)w32Rect.left, (f32)w32Rect.top };
			rect.max = { (f32)w32Rect.right, (f32)w32Rect.bottom };
		}
	}
	return rect;
}

Rect2D OS_clientRectFromWindow(OSWindowHandle handle) {
	Rect2D rect{};
	Win32Window* window = OS_windowFromHandle(handle);
	if (window) {
		RECT w32Rect{};
		if (GetClientRect(window->hwnd, &w32Rect)) {
			rect.min = { (f32)w32Rect.left, (f32)w32Rect.top };
			rect.max = { (f32)w32Rect.right, (f32)w32Rect.bottom };
		}
	}
	return rect;
}
vec2 OS_mouseFromWindow(OSWindowHandle handle) {
	vec2 result = { -100, -100 };
	Win32Window* window = OS_windowFromHandle(handle);
	if (window) {
		POINT point;
		if (GetCursorPos(&point)) {
			if (ScreenToClient(window->hwnd, &point)) {
				result = { (f32)point.x, (f32)point.y };
			}
		}
	}
	return result;
}

void OS_windowClearCustomBorderData(OSWindowHandle handle) {
	Win32Window* window = OS_windowFromHandle(handle);
	arenaClear(window->paintArena);
	window->firstTitleBarClientArea = window->lastTitleBarClientArea = nullptr;
	window->customBorderTitleThickness = 0.0f;
	window->customBorderEdgeThickness = 0.0f;
}

void OS_windowPushCustomTitleBar(OSWindowHandle handle, f32 thickness) {
	Win32Window* window = OS_windowFromHandle(handle);
	window->customBorderTitleThickness = thickness;
}

void OS_windowPushCustomEdges(OSWindowHandle handle, f32 thickness) {
	Win32Window* window = OS_windowFromHandle(handle);
	window->customBorderEdgeThickness = thickness;
}

void OS_windowPushCustomTitlebarClientArea(OSWindowHandle handle, Rect2D rect) {
	Win32Window* window = OS_windowFromHandle(handle);
	Win32TitleBarClientArea* area = PushStruct(window->paintArena, Win32TitleBarClientArea);
	if (area) {
		area->rect = rect;
		QueuePush(window->firstTitleBarClientArea, window->lastTitleBarClientArea, area);
	}
}

OS_Modifiers OS_getModifiers() {
	OS_Modifiers modifiers{};
	if (GetKeyState(VK_CONTROL) & 0x8000) {
		modifiers |= OS_Modifier_Ctrl;
	}
	if (GetKeyState(VK_SHIFT) & 0x8000) {
		modifiers |= OS_Modifier_Shift;
	}
	if (GetKeyState(VK_MENU) & 0x8000) {
		modifiers |= OS_Modifier_Alt;
	}

	return modifiers;
}

OS_EventList OS_getEvents(Arena* arena) {
	OS_EventList list{};
	win32EventsArena = arena;
	win32EventsList = &list;
	for (MSG message; PeekMessage(&message, nullptr, 0, 0, PM_REMOVE);) {
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	win32EventsArena = nullptr;
	win32EventsList = nullptr;
	return list;
}

void OS_consumeEvent(OS_EventList* events, OS_Event* event) {
	DLLRemove(events->first, events->last, event);
	events->count -= 1;
	event->kind = OS_EventKind_Null;
}
