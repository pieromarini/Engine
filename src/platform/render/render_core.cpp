#include "render_core.h"

#include "vulkan/render_vulkan.h"

// TODO(piero): Implement a way to select the renderer we want to use.
void Render_init() {
	Render_Vk_init();
}

void Render_equipWindow(OSWindowHandle windowHandle) {
	Render_Vk_equipWindow(windowHandle);
}

void Render_update() {
	Render_Vk_update();
}

void Render_startWindow(OSWindowHandle windowHandle, vec2 size) {
	Render_VK_updateWindowSize(windowHandle, size);

	// TODO
}

void Render_endWindow(OSWindowHandle windowHandle) {
	// TODO
}
