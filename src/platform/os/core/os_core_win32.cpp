#include "core/entry_point.h"
#include "core/thread_context.h"
#include "os_core.h"
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
// NOTE(piero): These conflict with our Min/Max macros
#define NOMINMAX
#include <windows.h>

u64 OS_pageSize() {
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	return info.dwPageSize;
}

void* OS_reserve(u64 size) {
	u64 gbSize = size;
	gbSize += Gigabytes(1) - 1;
	gbSize -= gbSize % Gigabytes(1);
	void* ptr = VirtualAlloc(nullptr, gbSize, MEM_RESERVE, PAGE_NOACCESS);
	return ptr;
}

void OS_release(void* ptr, u64 size) {
	VirtualFree(ptr, size, MEM_RELEASE);
}

void OS_commit(void* ptr, u64 size) {
	u64 pageAlignedSize = size;
	pageAlignedSize += OS_pageSize() - 1;
	pageAlignedSize -= pageAlignedSize % OS_pageSize();
	VirtualAlloc(ptr, pageAlignedSize, MEM_COMMIT, PAGE_READWRITE);
}

void OS_decommit(void* ptr, u64 size) {
	VirtualFree(ptr, size, MEM_DECOMMIT);
}

void OS_abort() {
	ExitProcess(1);
}

int wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
	ThreadCtx tCtx = ThreadCtx_alloc();
	ThreadCtx_set(&tCtx);

#if DEBUG
	// Create console in debug mode
	AllocConsole();
	FILE* fp = nullptr;
	freopen_s(&fp, "CONOUT$", "w", stdout);
	freopen_s(&fp, "CONOUT$", "w", stderr);
	freopen_s(&fp, "CONIN$", "r", stdin);
#endif

	mainEntryPoint(__argc, __argv);

#if DEBUG
	FreeConsole();
#endif

	ThreadCtx_set(&tCtx);
	ThreadCtx_release();

  return 0;
}
