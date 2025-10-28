#if defined(PLATFORM_WINDOWS)
#include "core/os_core_win32.cpp"
#include "gfx/os_gfx_win32.cpp"
#elif defined(PLATFORM_LINUX)
#include "core/os_core_linux.cpp"
#include "gfx/os_gfx_linux.cpp"
#endif
