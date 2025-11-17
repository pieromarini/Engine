#pragma once

#include "core/math/core_math.h"
#include "platform/os/core/os_core.h"

void Render_init();
void Render_equipWindow(OSWindowHandle windowHandle);
void Render_update();

void Render_startWindow(OSWindowHandle windowHandle, vec2 size);
void Render_endWindow(OSWindowHandle windowHandle);

void Render_loadScene(String8 path);

// TODO(piero): I don't know how I feel with this API.
//              Maybe we should just store these as part of a window. How to handle rendering UI (orthographic) + 3D (normally perspective)? Store 2 sets?
//              Or the UI projection is a hardcoded orthographic projection from 0 -> windowWidth, 0 -> windowHeight
void Render_setViewMatrix(vec3 cameraPosition, f32 pitch, f32 yaw);
void Render_setProjectionMatrix(f32 fov, f32 aspectRatio, f32 near, f32 far);
