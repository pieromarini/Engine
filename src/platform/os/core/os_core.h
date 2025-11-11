#pragma once

#include "core/core.h"

struct OSWindowHandle {
	u64 u64[1];
};

enum OS_CursorType {
	OS_CursorType_Null,
	OS_CursorType_Hidden,
	OS_CursorType_Pointer,
	OS_CursorType_Hand,
	OS_CursorType_WestEast,
	OS_CursorType_NorthSouth,
	OS_CursorType_NorthEastSouthWest,
	OS_CursorType_NorthWestSouthEast,
	OS_CursorType_AllCardinalDirections,
	OS_CursorType_IBar,
	OS_CursorType_Blocked,
	OS_CursorType_Loading,
	OS_CursorType_Pan,
	OS_CursorType_COUNT
};

u64 OS_pageSize();

void* OS_reserve(u64 size);
void OS_release(void* ptr, u64 size);

void OS_commit(void* ptr, u64 size);
void OS_decommit(void* ptr, u64 size);

void OS_abort();

static u64 OS_getOSTimerFreq();
static u64 OS_readOSTimer();
static u64 OS_readCPUTimer();
static u64 OS_estimateCPUTimerFreq();

void OS_init();

// Application entry point. Defined per platform
static void entryPoint();
