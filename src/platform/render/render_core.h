#pragma once

#include "platform/os/core/os_core.h"

void Render_init();
void Render_equipWindow(OSWindowHandle windowHandle);
void Render_update();

void Render_startWindow(OSWindowHandle windowHandle, vec2 size);
void Render_endWindow(OSWindowHandle windowHandle);

void Render_loadScene(String8 path);
