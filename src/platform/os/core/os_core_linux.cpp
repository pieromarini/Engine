#include "core/entry_point.h"
#include "core/thread_context.h"
#include "os_core.h"
#include "platform/os/gfx/os_gfx.h"

#include <unistd.h>
#include <x86intrin.h>
#include <sys/mman.h>
#include <sys/time.h>

u64 OS_pageSize() {
  u64 result = getpagesize();
  return result;
}

void* OS_reserve(u64 size) {
  u64 gbSize = size;
  gbSize += Gigabytes(1) - 1;
  gbSize -= gbSize % Gigabytes(1);
  mem = mmap(nullptr, gbSize, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

void OS_release(void* ptr, u64 size) {
  munmap(ptr, size);
}

void OS_commit(void* ptr, u64 size) {
  u64 pageAlignedSize = size;
  pageAlignedSize += OS_pageSize() - 1;
  pageAlignedSize -= pageAlignedSize % OS_pageSize();
  u32 err = mprotect(ptr, pageAlignedSize, PROT_WRITE | PROT_READ);
}

void OS_decommit(void* ptr, u64 size) {
  u32 err = mprotect(ptr, size, PROT_NONE);
  assert(err == 0);
}

void OS_abort() {
  _exit(0);
}

static u64 OS_getOSTimerFreq() {
  return 1000000;
}

static u64 OS_readOSTimer() {
  timeval value;
  gettimeofday(&value, 0);
  
  u64 Result = OS_getOSTimerFreq() * (u64)value.tv_sec + (u64)value.tv_usec;
  return Result;
}

void OS_init() {
  OS_gfxInit();
}

int main(int argc, char** argv) {
  ThreadCtx tCtx = ThreadCtx_alloc();
  ThreadCtx_set(&tCtx);

  mainEntryPoint(__argc, __argv);

  ThreadCtx_set(&tCtx);
  ThreadCtx_release();

  return 0;
}
