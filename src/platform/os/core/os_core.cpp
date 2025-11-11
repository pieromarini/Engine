#include <intrin.h>

#include "os_core.h"

static u64 OS_readCPUTimer() {
	return __rdtsc();
}

static u64 OS_estimateCPUTimerFreq() {
	u64 millisecondsToWait = 100;
	u64 osFreq = OS_getOSTimerFreq();

	u64 cpuStart = OS_readCPUTimer();
	u64 osStart = OS_readOSTimer();
	u64 osEnd = 0;
	u64 osElapsed = 0;
	u64 osWaitTime = osFreq * millisecondsToWait / 1000;

	while (osElapsed < osWaitTime) {
		osEnd = OS_readOSTimer();
		osElapsed = osEnd - osStart;
	}

	u64 cpuEnd = OS_readCPUTimer();
	u64 cpuElapsed = cpuEnd - cpuStart;

	u64 cpuFreq = 0;
	if (osElapsed) {
		cpuFreq = osFreq * cpuElapsed / osElapsed;
	}

	return cpuFreq;
}
