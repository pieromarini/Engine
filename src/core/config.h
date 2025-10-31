#pragma once

#ifndef VSYNC
#define VSYNC 0
#endif

#ifndef USE_VALIDATION
#define USE_VALIDATION true
#endif

#if !DEBUG
#define CONFIG_VALIDATION 0
#define CONFIG_SYNC_VALIDATION 0
#endif
