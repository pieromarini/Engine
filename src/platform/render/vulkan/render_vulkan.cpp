#include "render_vulkan.h"

#include "core/config.h"
#include "core/core.h"
#include "core/math/matrix.h"
#include "core/memory/arena.h"
#include "core/thread_context.h"
#include "platform/os/gfx/os_gfx_win32.h"
#include "platform/render/vulkan/render_vulkan_transitions.h"
#include "platform/render/vulkan/render_vulkan_shaders.h"
#include "vulkan/vulkan_core.h"


per_thread RenderVkState* renderVkState;

static Vertex cubeVertices[] = {
	// Front face
	{{-0.5f, -0.5f,  0.5f}, 0.0f, { 0.0f,  0.0f,  1.0f}, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{ 0.5f, -0.5f,  0.5f}, 1.0f, { 0.0f,  0.0f,  1.0f}, 0.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, 1.0f, { 0.0f,  0.0f,  1.0f}, 1.0f, {1.0f, 0.0f, 0.0f, 1.0f}},
	{{-0.5f,  0.5f,  0.5f}, 0.0f, { 0.0f,  0.0f,  1.0f}, 1.0f, {1.0f, 0.0f, 0.0f, 1.0f}},

	// Back face
	{{ 0.5f, -0.5f, -0.5f}, 0.0f, { 0.0f,  0.0f, -1.0f}, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, 1.0f, { 0.0f,  0.0f, -1.0f}, 0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, 1.0f, { 0.0f,  0.0f, -1.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, 0.0f, { 0.0f,  0.0f, -1.0f}, 1.0f, {1.0f, 1.0f, 1.0f, 1.0f}},

	// Left face
	{{-0.5f, -0.5f, -0.5f}, 0.0f, {-1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, 1.0f, {-1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f,  0.5f,  0.5f}, 1.0f, {-1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, 0.0f, {-1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 1.0f, 0.0f, 1.0f}},

	// Right face
	{{ 0.5f, -0.5f,  0.5f}, 0.0f, { 1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{ 0.5f, -0.5f, -0.5f}, 1.0f, { 1.0f,  0.0f,  0.0f}, 0.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, 1.0f, { 1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, 0.0f, { 1.0f,  0.0f,  0.0f}, 1.0f, {0.0f, 0.0f, 1.0f, 1.0f}},

	// Top face
	{{-0.5f,  0.5f,  0.5f}, 0.0f, { 0.0f,  1.0f,  0.0f}, 0.0f, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, 1.0f, { 0.0f,  1.0f,  0.0f}, 0.0f, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, 1.0f, { 0.0f,  1.0f,  0.0f}, 1.0f, {1.0f, 1.0f, 0.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, 0.0f, { 0.0f,  1.0f,  0.0f}, 1.0f, {1.0f, 1.0f, 0.0f, 1.0f}},

	// Bottom face
	{{-0.5f, -0.5f, -0.5f}, 0.0f, { 0.0f, -1.0f,  0.0f}, 0.0f, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{ 0.5f, -0.5f, -0.5f}, 1.0f, { 0.0f, -1.0f,  0.0f}, 0.0f, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{ 0.5f, -0.5f,  0.5f}, 1.0f, { 0.0f, -1.0f,  0.0f}, 1.0f, {1.0f, 0.0f, 1.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, 0.0f, { 0.0f, -1.0f,  0.0f}, 1.0f, {1.0f, 0.0f, 1.0f, 1.0f}},
};

static u32 cubeIndices[] = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	8, 9,10,10,11, 8,
	12,13,14,14,15,12,
	16,17,18,18,19,16,
	20,21,22,22,23,20
};

void immediateSubmit(void (*fn)(VkCommandBuffer cmd)) {
	VK_CHECK(vkResetFences(renderVkState->device, 1, &renderVkState->immFence));
	VK_CHECK(vkResetCommandBuffer(renderVkState->immCommandBuffer, 0));

	VkCommandBuffer cmd = renderVkState->immCommandBuffer;

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	fn(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkCommandBufferSubmitInfo cmdInfo{};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	cmdInfo.commandBuffer = cmd;
	cmdInfo.deviceMask = 0;

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

	submitInfo.waitSemaphoreInfoCount = 0;
	submitInfo.pWaitSemaphoreInfos = nullptr;

	submitInfo.signalSemaphoreInfoCount = 0;
	submitInfo.pSignalSemaphoreInfos = nullptr;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdInfo;

	VK_CHECK(vkQueueSubmit2(renderVkState->graphicsQueue, 1, &submitInfo, renderVkState->immFence));

	VK_CHECK(vkWaitForFences(renderVkState->device, 1, &renderVkState->immFence, true, 9999999999));
}

VkSemaphore createSemaphore(VkDevice device, VkSemaphoreCreateFlags flags) {
	VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	createInfo.flags = flags;

	VkSemaphore semaphore = nullptr;
	VK_CHECK(vkCreateSemaphore(device, &createInfo, nullptr, &semaphore));

	return semaphore;
}

VkFence createFence(VkDevice device, VkFenceCreateFlags flags) {
	VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	createInfo.flags = flags;

	VkFence fence = nullptr;
	VK_CHECK(vkCreateFence(device, &createInfo, nullptr, &fence));

	return fence;
}

VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount, VkQueryType queryType) {
	VkQueryPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	createInfo.queryType = queryType;
	createInfo.queryCount = queryCount;

	if (queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
		createInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
	}

	VkQueryPool queryPool = nullptr;
	VK_CHECK(vkCreateQueryPool(device, &createInfo, nullptr, &queryPool));

	return queryPool;
}

static bool isLayerSupported(const char* name) {
	u32 propertyCount = 0;
	VK_CHECK(vkEnumerateInstanceLayerProperties(&propertyCount, nullptr));

	Temp scratch = ScratchBegin();

	VkLayerProperties* properties = PushArray(scratch.arena, VkLayerProperties, propertyCount);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&propertyCount, properties));

	for (uint32_t i = 0; i < propertyCount; ++i)
		if (strcmp(name, properties[i].layerName) == 0)
			return true;

	ScratchEnd(scratch);

	return false;
}

bool isInstanceExtensionSupported(const char* name) {
	u32 propertyCount = 0;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr));

	Temp scratch = ScratchBegin();

	VkExtensionProperties* properties = PushArray(scratch.arena, VkExtensionProperties, propertyCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties));

	for (uint32_t i = 0; i < propertyCount; ++i)
		if (strcmp(name, properties[i].extensionName) == 0)
			return true;

	ScratchEnd(scratch);

	return false;
}

String8List getSwapchainExtensions(Arena* arena) {
	String8List extensions{};
#ifdef VK_USE_PLATFORM_WIN32_KHR
	Str8ListPush(arena, &extensions, Str8L(VK_KHR_SURFACE_EXTENSION_NAME));
	Str8ListPush(arena, &extensions, Str8L(VK_KHR_WIN32_SURFACE_EXTENSION_NAME));
#else
	printf("[getSwapchainExtensions] Only implemented for win32");
	OS_abort();
#endif
	return extensions;
}

VkInstance createInstance() {
	if (volkGetInstanceVersion() < API_VERSION) {
		fprintf(stderr, "ERROR: Vulkan 1.%d instance not found\n", VK_VERSION_MINOR(API_VERSION));
		return nullptr;
	}

	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = API_VERSION;

	VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

#if KHR_VALIDATION || SYNC_VALIDATION
	const char* debugLayers[] = {
		"VK_LAYER_KHRONOS_validation",
	};

	if (isLayerSupported("VK_LAYER_KHRONOS_validation")) {
		createInfo.ppEnabledLayerNames = debugLayers;
		createInfo.enabledLayerCount = sizeof(debugLayers) / sizeof(debugLayers[0]);
		printf("Enabled Vulkan validation layers (sync validation %s)\n", SYNC_VALIDATION ? "enabled" : "disabled");
	} else {
		printf("Warning: Vulkan debug layers are not available\n");
	}

	// sync validation
	VkValidationFeatureEnableEXT enabledValidationFeatures[] = {
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	};

	VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = sizeof(enabledValidationFeatures) / sizeof(enabledValidationFeatures[0]);
	validationFeatures.pEnabledValidationFeatures = enabledValidationFeatures;

	createInfo.pNext = &validationFeatures;
#endif

	Temp scratch = ScratchBegin();

	String8List extensionsList = getSwapchainExtensions(scratch.arena);

#ifdef DEBUG
	Str8ListPush(scratch.arena, &extensionsList, Str8L(VK_EXT_DEBUG_REPORT_EXTENSION_NAME));
#endif

	if (isInstanceExtensionSupported(VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
		Str8ListPush(scratch.arena, &extensionsList, Str8L(VK_EXT_DEBUG_UTILS_EXTENSION_NAME));

	CStringArray extensionsArray = CStringArrayFromList(scratch.arena, extensionsList);
	createInfo.ppEnabledExtensionNames = extensionsArray.strings;
	createInfo.enabledExtensionCount = extensionsArray.count;

	VkInstance instance = nullptr;
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));

	ScratchEnd(scratch);

	return instance;
}

static VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {
	const char* type =
		(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "ERROR" : (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) ? "WARNING" :
																																																		"INFO";

	char message[4096];
	snprintf(message, 4096, "%s: %s\n", type, pMessage);

	printf("%s", message);

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		Assert(!"Validation error encountered!");

	return VK_FALSE;
}

VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance) {
	if (!vkCreateDebugReportCallbackEXT)
		return nullptr;

	VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };
	createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
	createInfo.pfnCallback = debugReportCallback;

	VkDebugReportCallbackEXT callback = nullptr;
	VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, nullptr, &callback));

	return callback;
}

uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice) {
	uint32_t queueCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);

	Temp scratch = ScratchBegin();

	VkQueueFamilyProperties* queues = PushArray(scratch.arena, VkQueueFamilyProperties, queueCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queues);

	for (uint32_t i = 0; i < queueCount; ++i)
		if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			return i;

	ScratchEnd(scratch);

	return VK_QUEUE_FAMILY_IGNORED;
}

static bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex) {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
	return !!vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, familyIndex);
#else
	return false;
#endif
}

VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount) {
	VkPhysicalDevice preferred = nullptr;
	VkPhysicalDevice fallback = nullptr;

	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

		if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU)
			continue;

		printf("GPU%d: %s (Vulkan 1.%d)\n", i, props.deviceName, VK_VERSION_MINOR(props.apiVersion));

		uint32_t familyIndex = getGraphicsFamilyIndex(physicalDevices[i]);
		if (familyIndex == VK_QUEUE_FAMILY_IGNORED)
			continue;

		if (!supportsPresentation(physicalDevices[i], familyIndex))
			continue;

		if (props.apiVersion < API_VERSION)
			continue;

		if (!preferred && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			preferred = physicalDevices[i];
		}

		if (!fallback) {
			fallback = physicalDevices[i];
		}
	}

	VkPhysicalDevice result = preferred ? preferred : fallback;

	if (result) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(result, &props);

		printf("Selected GPU %s\n", props.deviceName);
	} else {
		fprintf(stderr, "ERROR: No compatible GPU found\n");
	}

	return result;
}

VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex) {
	float queuePriorities[] = { 1.0f };

	VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	queueInfo.queueFamilyIndex = familyIndex;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = queuePriorities;

	const char* extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkPhysicalDeviceFeatures2 features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
	features.features.multiDrawIndirect = true;
	features.features.pipelineStatisticsQuery = true;
	features.features.shaderInt16 = true;
	features.features.shaderInt64 = true;
	features.features.samplerAnisotropy = true;

	VkPhysicalDeviceVulkan11Features features11 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	features11.storageBuffer16BitAccess = true;
	features11.shaderDrawParameters = true;

	VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.drawIndirectCount = true;
	features12.storageBuffer8BitAccess = true;
	features12.uniformAndStorageBuffer8BitAccess = true;
	features12.shaderFloat16 = true;
	features12.shaderInt8 = true;
	features12.samplerFilterMinmax = true;
	features12.scalarBlockLayout = true;

	features12.bufferDeviceAddress = true;

	features12.descriptorIndexing = true;
	features12.shaderSampledImageArrayNonUniformIndexing = true;
	features12.descriptorBindingSampledImageUpdateAfterBind = true;
	features12.descriptorBindingUpdateUnusedWhilePending = true;
	features12.descriptorBindingPartiallyBound = true;
	features12.descriptorBindingVariableDescriptorCount = true;
	features12.runtimeDescriptorArray = true;

	VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;
	features13.maintenance4 = true;
	features13.shaderDemoteToHelperInvocation = true;// required for discard; under new glslang rules

	VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueInfo;

	createInfo.ppEnabledExtensionNames = extensions;
	createInfo.enabledExtensionCount = ArrayCount(extensions);

	createInfo.pNext = &features;
	features.pNext = &features11;
	features11.pNext = &features12;
	features12.pNext = &features13;

	VkDevice device = nullptr;
	VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

	return device;
}

VkSurfaceKHR createSurfaceWin32(OSWindowHandle windowHandle) {
	Win32Window* window = OS_windowFromHandle(windowHandle);

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = window->hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	VkSurfaceKHR surface{};
	VK_CHECK(vkCreateWin32SurfaceKHR(renderVkState->instance, &createInfo, nullptr, &surface));
	return surface;
}

VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
	uint32_t formatCount = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));
	Assert(formatCount > 0);

	Temp scratch = ScratchBegin();

	VkSurfaceFormatKHR* formats = PushArray(scratch.arena, VkSurfaceFormatKHR, formatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats));

	if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
		return VK_FORMAT_R8G8B8A8_UNORM;

	for (uint32_t i = 0; i < formatCount; ++i)
		if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || formats[i].format == VK_FORMAT_B8G8R8A8_UNORM)
			return formats[i].format;

	VkFormat format = formats[0].format;

	ScratchEnd(scratch);

	return format;
}

VkPresentModeKHR getPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
#if VSYNC
	return VK_PRESENT_MODE_FIFO_KHR;
#else
	uint32_t presentModeCount = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
	Assert(presentModeCount > 0);

	Temp scratch = ScratchBegin();

	VkPresentModeKHR* presentModes = PushArray(scratch.arena, VkPresentModeKHR, presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes));

	VkPresentModeKHR selectedMode = VK_PRESENT_MODE_FIFO_KHR;

	// Only return a few present modes.
	for (u32 i = 0; i < presentModeCount; ++i) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			selectedMode = VK_PRESENT_MODE_MAILBOX_KHR;
		else if (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
			selectedMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	ScratchEnd(scratch);

	return selectedMode;
#endif
}

RenderVkSwapchain* createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format, VkSwapchainKHR oldSwapchain) {
	// TODO(piero): Free list? Specific swapchain arena?
	RenderVkSwapchain* result = PushStruct(renderVkState->arena, RenderVkSwapchain);

	VkSurfaceCapabilitiesKHR surfaceCaps;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));

	Rect2D rect = OS_clientRectFromWindow(windowHandle);
	vec2 size = rect2DSize(rect);
	i32 width = (i32)size.x;
	i32 height = (i32)size.y;

	VkPresentModeKHR presentMode = getPresentMode(physicalDevice, surface);

	VkCompositeAlphaFlagBitsKHR surfaceComposite = (surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
																									? VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR
																									: ((surfaceCaps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
																										? VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR 
																										: VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR);

	VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	createInfo.surface = surface;
	createInfo.minImageCount = Max(3, surfaceCaps.minImageCount);
	createInfo.imageFormat = format;
	createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	createInfo.imageExtent.width = width;
	createInfo.imageExtent.height = height;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	createInfo.queueFamilyIndexCount = 1;
	createInfo.pQueueFamilyIndices = &familyIndex;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.compositeAlpha = surfaceComposite;
	createInfo.presentMode = presentMode;
	createInfo.oldSwapchain = oldSwapchain;

	VkSwapchainKHR swapchain = nullptr;
	VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain));

	uint32_t imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));

	VkImage* images = PushArray(renderVkState->arena, VkImage, imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images));

	result->swapchain = swapchain;
	result->images = images;
	result->width = width;
	result->height = height;
	result->imageCount = imageCount;

	return result;
}

void destroySwapchain(VkDevice device, RenderVkSwapchain* swapchain) {
	vkDestroySwapchainKHR(device, swapchain->swapchain, nullptr);
}

SwapchainStatus recreateSwapchain(RenderVkSwapchain* oldSwapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format) {
	Rect2D rect = OS_clientRectFromWindow(windowHandle);
	vec2 size = rect2DSize(rect);
	i32 width = (i32)size.x;
	i32 height = (i32)size.y;

	if (width == 0 || height == 0)
		return Swapchain_NotReady;

	if (oldSwapchain->width == width && oldSwapchain->height == height)
		return Swapchain_Ready;

	RenderVkSwapchain* newSwapchain = createSwapchain(physicalDevice, device, surface, familyIndex, windowHandle, format, oldSwapchain->swapchain);
	renderVkState->swapchain = newSwapchain;

	VK_CHECK(vkDeviceWaitIdle(device));

	// Create new render target
	// TODO: extract this out. Swapchain recreation shouldn't be tied to render-target recreation.
	destroyImage(renderVkState->device, renderVkState->drawImage);
	destroyImage(renderVkState->device, renderVkState->depthImage);

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	renderVkState->drawImage = createImage(device, memoryProperties, width, height, 1, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);
	renderVkState->depthImage = createImage(device, memoryProperties, width, height, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	renderVkState->drawExtent = { .width = (u32)width, .height = (u32)height };

	destroySwapchain(device, oldSwapchain);

	// Update descriptor set using drawImage
	VkDescriptorImageInfo imgInfo{
		.sampler = nullptr,
		.imageView = renderVkState->drawImage->imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};

	VkWriteDescriptorSet setWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext = nullptr,
		.dstSet = renderVkState->computeSet,
		.dstBinding = 0,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.pImageInfo = &imgInfo
	};
	vkUpdateDescriptorSets(renderVkState->device, 1, &setWrite, 0, nullptr);

	return Swapchain_Resized;
}

void recreateSwapchainIfNeeded(OSWindowHandle windowHandle, vec2 size) {
	u32 lastWidth = renderVkState->swapchain->width;
	u32 lastHeight = renderVkState->swapchain->height;

	// No need to resize.
	if ((u32)size.x == lastWidth && (u32)size.y == lastHeight) {
		return;
	}

	VkFormat swapchainFormat = getSwapchainFormat(renderVkState->physicalDevice, renderVkState->surface);
	recreateSwapchain(renderVkState->swapchain, renderVkState->physicalDevice, renderVkState->device, renderVkState->surface, renderVkState->graphicsQueueFamily, windowHandle, swapchainFormat);
}

void Render_startWindow(OSWindowHandle windowHandle, vec2 size) {
	if (renderVkState->swapchain->needsResize) {
		recreateSwapchainIfNeeded(windowHandle, size);
		renderVkState->swapchain->needsResize = false;
	}
}

void Render_endWindow(OSWindowHandle windowHandle) {

}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevel, u32 levelCount) {
	VkImageAspectFlags aspectMask = (format == VK_FORMAT_D32_SFLOAT) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

	VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	createInfo.image = image;
	createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	createInfo.format = format;
	createInfo.subresourceRange.aspectMask = aspectMask;
	createInfo.subresourceRange.baseMipLevel = mipLevel;
	createInfo.subresourceRange.levelCount = levelCount;
	createInfo.subresourceRange.layerCount = 1;

	VkImageView view = nullptr;
	VK_CHECK(vkCreateImageView(device, &createInfo, nullptr, &view));

	return view;
}

static u32 selectMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, u32 memoryTypeBits, VkMemoryPropertyFlags flags) {
	for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
		if ((memoryTypeBits & (1 << i)) != 0 && (memoryProperties.memoryTypes[i].propertyFlags & flags) == flags)
			return i;

	Assert(!"No compatible memory type found");
	return ~0u;
}

RenderVkImage* createImage(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageUsageFlags usage) {
	RenderVkImage* result = PushStruct(renderVkState->arena, RenderVkImage);

	VkImageCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = format;
	createInfo.extent = { .width = width, .height = height, .depth = 1 };
	createInfo.mipLevels = mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = usage;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VkImage image = nullptr;
	VK_CHECK(vkCreateImage(device, &createInfo, nullptr, &image));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(device, image, &memoryRequirements);

	// Create image on GPU local memory
	uint32_t memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	Assert(memoryTypeIndex != ~0u);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory memory = nullptr;
	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &memory));

	VK_CHECK(vkBindImageMemory(device, image, memory, 0));

	result->image = image;
	result->imageView = createImageView(device, image, format, 0, mipLevels);
	result->memory = memory;
	result->format = format;

	return result;
}

void destroyImage(VkDevice device, RenderVkImage* image) {
	vkDestroyImageView(device, image->imageView, nullptr);
	vkDestroyImage(device, image->image, nullptr);
	vkFreeMemory(device, image->memory, nullptr);
}

RenderVkBuffer createBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, u32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags) {
	RenderVkBuffer result{};

	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = size;
	createInfo.usage = usage;

	VkBuffer buffer{};
	VK_CHECK(vkCreateBuffer(device, &createInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	uint32_t memoryTypeIndex = selectMemoryType(memoryProperties, memoryRequirements.memoryTypeBits, memoryFlags);
	Assert(memoryTypeIndex != ~0u);

	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryTypeIndex;

	VkMemoryAllocateFlagsInfo flagInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };

	if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
		allocateInfo.pNext = &flagInfo;
		flagInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
		flagInfo.deviceMask = 1;
	}

	VkDeviceMemory memory{};
	VK_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, &memory));

	VK_CHECK(vkBindBufferMemory(device, buffer, memory, 0));

	void* data{};
	if (memoryFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
		VK_CHECK(vkMapMemory(device, memory, 0, size, 0, &data));
	}

	result.buffer = buffer;
	result.memory = memory;
	result.data = data;
	result.size = size;

	return result;
}

void uploadBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, const RenderVkBuffer& buffer, const RenderVkBuffer& scratch, void* data, u32 size) {
	// TODO(piero): This submits the Command Buffer and waits for device idle. Not optimal.
	Assert(size > 0);
	Assert(scratch.data);
	Assert(scratch.size >= size);

	memcpy(scratch.data, data, size);

	VK_CHECK(vkResetCommandPool(device, commandPool, 0));

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkBufferCopy region = { 0, 0, VkDeviceSize(size) };
	vkCmdCopyBuffer(commandBuffer, scratch.buffer, buffer.buffer, 1, &region);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

	VK_CHECK(vkDeviceWaitIdle(device));
}

void destroyBuffer(VkDevice device, const RenderVkBuffer& buffer) {
	vkDestroyBuffer(device, buffer.buffer, nullptr);
	vkFreeMemory(device, buffer.memory, nullptr);
}

VkDeviceAddress getBufferAddress(VkDevice device, const RenderVkBuffer& buffer) {
	VkBufferDeviceAddressInfo info = { VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
	info.buffer = buffer.buffer;

	VkDeviceAddress address = vkGetBufferDeviceAddress(device, &info);
	Assert(address != 0);

	return address;
}

VkSampler createSampler(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, VkSamplerReductionModeEXT reductionMode) {
	VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	createInfo.magFilter = filter;
	createInfo.minFilter = filter;
	createInfo.mipmapMode = mipmapMode;
	createInfo.addressModeU = addressMode;
	createInfo.addressModeV = addressMode;
	createInfo.addressModeW = addressMode;
	createInfo.minLod = 0;
	createInfo.maxLod = 16.f;
	createInfo.anisotropyEnable = mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.maxAnisotropy = mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR ? 4.f : 1.f;

	VkSamplerReductionModeCreateInfoEXT createInfoReduction = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };

	if (reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT) {
		createInfoReduction.reductionMode = reductionMode;

		createInfo.pNext = &createInfoReduction;
	}

	VkSampler sampler = nullptr;
	VK_CHECK(vkCreateSampler(device, &createInfo, nullptr, &sampler));
	return sampler;
}

// Descriptors
VkDescriptorSetLayout buildDescriptorLayout(VkDescriptorSetLayoutBinding* bindings, u32 numBindings, VkDescriptorSetLayoutCreateFlags flags) {
	VkDescriptorSetLayout result{};

	VkDescriptorSetLayoutCreateInfo info{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext = nullptr,
			.flags = flags,
			.bindingCount = numBindings,
			.pBindings = bindings,
	};

	VK_CHECK(vkCreateDescriptorSetLayout(renderVkState->device, &info, nullptr, &result));

	return result;
}

VkDescriptorPool buildDescriptorPool(u32 maxSets, VkDescriptorPoolSize* poolSizes, u32 poolSizesCount) {
	VkDescriptorPool result{};

	VkDescriptorPoolCreateInfo info {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.maxSets = maxSets,
			.poolSizeCount = poolSizesCount,
			.pPoolSizes = poolSizes
	};
	vkCreateDescriptorPool(renderVkState->device, &info, nullptr, &result);

	return result;
}

VkDescriptorSet buildDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout) {
	VkDescriptorSet result{};

	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &layout
	};

	VK_CHECK(vkAllocateDescriptorSets(renderVkState->device, &allocInfo, &result));

	return result;
}

// Pipelines
VkPipeline buildPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipelineShaderStageCreateInfo* shaderStages, u32 shaderStagesCount, VkPrimitiveTopology topology, VkPolygonMode mode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, b32 blendingEnabled) {
	VkPipeline result{};

	Temp scratch = ScratchBegin();

	VkPipelineColorBlendAttachmentState colorBlendAttachment {
		.blendEnable = VK_FALSE,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = topology,
			.primitiveRestartEnable = VK_FALSE
	};
	VkPipelineRasterizationStateCreateInfo rasterizer{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.polygonMode = mode,
			.cullMode = cullMode,
			.frontFace = frontFace,
			.lineWidth = 1.0f,
	};
	VkPipelineMultisampleStateCreateInfo multisampling{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 1.0f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE,
	};
	VkPipelineDepthStencilStateCreateInfo depthStencil{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
	};
	VkPipelineRenderingCreateInfo renderInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &colorAttachmentFormat,
			.depthAttachmentFormat = depthAttachmentFormat
	};
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};


	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
	};

	pipelineInfo.pNext = &renderInfo;
	pipelineInfo.stageCount = shaderStagesCount;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = pipelineLayout;

	VkDynamicState state[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
	};
	dynamicInfo.pDynamicStates = &state[0];
	dynamicInfo.dynamicStateCount = 2;

	pipelineInfo.pDynamicState = &dynamicInfo;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &result) != VK_SUCCESS) {
		printf("[Render] Failed to create graphics pipeline.");
		return VK_NULL_HANDLE;
	}

	ScratchEnd(scratch);

	return result;
}

// Init subsystem
void Render_init() {
	Arena* arena = arenaAlloc(Gigabytes(16));
	renderVkState = PushStruct(arena, RenderVkState);
	renderVkState->arena = arena;

	VK_CHECK(volkInitialize());

	VkInstance instance = createInstance();
	if (!instance) {
		printf("[Render] Invalid vulkan instance");
		return;
	}

	volkLoadInstanceOnly(instance);

	VkDebugReportCallbackEXT debugCallback = registerDebugCallback(instance);

	VkPhysicalDevice physicalDevices[16];
	u32 physicalDeviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
	VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

	VkPhysicalDevice physicalDevice = pickPhysicalDevice(physicalDevices, physicalDeviceCount);
	if (!physicalDevice) {
		if (debugCallback) {
			vkDestroyDebugReportCallbackEXT(instance, debugCallback, nullptr);
		}
		vkDestroyInstance(instance, nullptr);
		return;
	}

	// TODO(piero): Remove asserts
	VkPhysicalDeviceProperties props = {};
	vkGetPhysicalDeviceProperties(physicalDevice, &props);
	Assert(props.limits.timestampComputeAndGraphics);

	u32 familyIndex = getGraphicsFamilyIndex(physicalDevice);
	Assert(familyIndex != VK_QUEUE_FAMILY_IGNORED);

	VkDevice device = createDevice(instance, physicalDevice, familyIndex);
	Assert(device);

	volkLoadDevice(device);

	VkQueue queue{};
	vkGetDeviceQueue(device, familyIndex, 0, &queue);

	// Fill state
	renderVkState->instance = instance;
	renderVkState->physicalDevice = physicalDevice;
	renderVkState->device = device;

	renderVkState->graphicsQueueFamily = familyIndex;
	renderVkState->graphicsQueue = queue;

	initCommands();
	initSync();
	initDescriptors();

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	renderVkState->scratchBuffer = createBuffer(renderVkState->device, memoryProperties, Megabytes(128), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

void initCommands() {
	VkCommandPoolCreateInfo commandPoolInfo =  {};
	commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolInfo.pNext = nullptr;
	commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolInfo.queueFamilyIndex = renderVkState->graphicsQueueFamily;

	for (int i = 0; i < MAX_FRAMES; i++) {
		VK_CHECK(vkCreateCommandPool(renderVkState->device, &commandPoolInfo, nullptr, &renderVkState->frames[i].commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = {};
		cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cmdAllocInfo.pNext = nullptr;
		cmdAllocInfo.commandPool = renderVkState->frames[i].commandPool;
		cmdAllocInfo.commandBufferCount = 1;
		cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VK_CHECK(vkAllocateCommandBuffers(renderVkState->device, &cmdAllocInfo, &renderVkState->frames[i].commandBuffer));
	}

	// TEMP: Init immediate submit stuff
	VK_CHECK(vkCreateCommandPool(renderVkState->device, &commandPoolInfo, nullptr, &renderVkState->immCommandPool));
	VkCommandBufferAllocateInfo cmdAllocInfo = {};
	cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdAllocInfo.pNext = nullptr;
	cmdAllocInfo.commandPool = renderVkState->immCommandPool;
	cmdAllocInfo.commandBufferCount = 1;
	cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	VK_CHECK(vkAllocateCommandBuffers(renderVkState->device, &cmdAllocInfo, &renderVkState->immCommandBuffer));
}

void initSync() {
	for (int i = 0; i < MAX_FRAMES; i++) {
		renderVkState->frames[i].renderFence = createFence(renderVkState->device, VK_FENCE_CREATE_SIGNALED_BIT);
		renderVkState->frames[i].renderSemaphore = createSemaphore(renderVkState->device);
		renderVkState->frames[i].swapchainSemaphore = createSemaphore(renderVkState->device);
	}

	// TEMP: Init immediate submit stuff
	renderVkState->immFence = createFence(renderVkState->device, VK_FENCE_CREATE_SIGNALED_BIT);
}

void initDescriptors() {
	Temp scratch = ScratchBegin();

	u32 maxSets = 10;
	u32 poolSizesCount = 1;
	VkDescriptorPoolSize* poolSizes = PushArray(scratch.arena, VkDescriptorPoolSize, poolSizesCount);
	poolSizes[0] = {
		.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1 * maxSets
	};
	renderVkState->descriptorPool = buildDescriptorPool(maxSets, poolSizes, poolSizesCount);

	u32 bindingsCount = 1;
	VkDescriptorSetLayoutBinding* bindings = PushArray(scratch.arena, VkDescriptorSetLayoutBinding, bindingsCount);
	bindings[0] = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.pImmutableSamplers = nullptr
	};
	renderVkState->computeLayout = buildDescriptorLayout(bindings, bindingsCount, 0);

	renderVkState->computeSet = buildDescriptorSet(renderVkState->descriptorPool, renderVkState->computeLayout);

	ScratchEnd(scratch);
}

void initPipelines() {
	// Compute pipeline
	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &renderVkState->computeLayout;
	computeLayout.setLayoutCount = 1;

	VK_CHECK(vkCreatePipelineLayout(renderVkState->device, &computeLayout, nullptr, &renderVkState->computePipelineLayout));

	VkShaderModule computeDrawShader = nullptr;
	if (!loadShaderModule("../res/shaders/gradient.comp.spv", renderVkState->device, &computeDrawShader)) {
		printf("Error when building the compute shader");
	}

	VkPipelineShaderStageCreateInfo stageinfo = shaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeDrawShader);

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = renderVkState->computePipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;

	VK_CHECK(vkCreateComputePipelines(renderVkState->device, VK_NULL_HANDLE,1 , &computePipelineCreateInfo, nullptr, &renderVkState->computePipeline));

	vkDestroyShaderModule(renderVkState->device, computeDrawShader, nullptr);

	// Triangle pipeline
	VkShaderModule meshVertexShader = nullptr;
	if (!loadShaderModule("../res/shaders/triangle.vert.spv", renderVkState->device, &meshVertexShader)) {
		printf("Error when building the mesh vertex shader");
	}
	VkShaderModule meshFragmentShader = nullptr;
	if (!loadShaderModule("../res/shaders/triangle.frag.spv", renderVkState->device, &meshFragmentShader)) {
		printf("Error when building the mesh fragment shader");
	}

	VkPushConstantRange range {
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0,
		.size = sizeof(MeshPushConstants)
	};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.flags = 0,
		.setLayoutCount = 0,
		.pSetLayouts = nullptr,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &range
	};

	VK_CHECK(vkCreatePipelineLayout(renderVkState->device, &pipelineLayoutInfo, nullptr, &renderVkState->meshPipelineLayout));

	VkPipelineShaderStageCreateInfo shaderStages[] = {
		shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertexShader),
		shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, meshFragmentShader)
	};
	renderVkState->meshPipeline = buildPipeline(renderVkState->device, renderVkState->meshPipelineLayout, shaderStages, ArrayCount(shaderStages), VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE, renderVkState->drawImage->format, renderVkState->depthImage->format, false);

	vkDestroyShaderModule(renderVkState->device, meshVertexShader, nullptr);
	vkDestroyShaderModule(renderVkState->device, meshFragmentShader, nullptr);
}

void Render_equipWindow(OSWindowHandle windowHandle) {
	VkDevice device = renderVkState->device;
	VkPhysicalDevice physicalDevice = renderVkState->physicalDevice;

	renderVkState->window = windowHandle;
	renderVkState->surface = createSurfaceWin32(windowHandle);

	VkBool32 presentSupported = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, renderVkState->graphicsQueueFamily, renderVkState->surface, &presentSupported));
	if (!presentSupported) {
		printf("[Render] No surface with presentation support for Win32. Exiting");
		OS_abort();
	}

	VkFormat swapchainFormat = getSwapchainFormat(physicalDevice, renderVkState->surface);

	RenderVkSwapchain* swapchain = createSwapchain(physicalDevice, device, renderVkState->surface, renderVkState->graphicsQueueFamily, renderVkState->window, swapchainFormat);
	renderVkState->swapchain = swapchain;

	// Create render target
	Rect2D rect = OS_clientRectFromWindow(windowHandle);
	vec2 size = rect2DSize(rect);
	u32 width = (u32)size.x;
	u32 height = (u32)size.y;

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	VkImageUsageFlags drawImageUsages{};
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
	drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	renderVkState->drawImage = createImage(device, memoryProperties, width, height, 1, VK_FORMAT_R16G16B16A16_SFLOAT, drawImageUsages);
	renderVkState->depthImage = createImage(device, memoryProperties, width, height, 1, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	renderVkState->drawExtent = { .width = width, .height = height };

	initPipelines();

	// Write to descriptor set
	VkDescriptorImageInfo imgInfo{
		.sampler = nullptr,
			.imageView = renderVkState->drawImage->imageView,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};

	VkWriteDescriptorSet setWrite{
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = nullptr,
			.dstSet = renderVkState->computeSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &imgInfo
	};
	vkUpdateDescriptorSets(renderVkState->device, 1, &setWrite, 0, nullptr);
	
	// TEMP: Create mesh buffers
	VkBufferUsageFlags vertexUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
 	VkBufferUsageFlags indexUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	renderVkState->meshVertexBuffer = createBuffer(renderVkState->device, memoryProperties, sizeof(cubeVertices), vertexUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	renderVkState->meshIndexBuffer = createBuffer(renderVkState->device, memoryProperties, sizeof(cubeIndices), indexUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkCommandPool commandPool = renderVkState->frames[0].commandPool;
	VkCommandBuffer commandBuffer = renderVkState->frames[0].commandBuffer;

	uploadBuffer(renderVkState->device, commandPool, commandBuffer, renderVkState->graphicsQueue, renderVkState->meshVertexBuffer, renderVkState->scratchBuffer, cubeVertices, sizeof(cubeVertices));
	uploadBuffer(renderVkState->device, commandPool, commandBuffer, renderVkState->graphicsQueue, renderVkState->meshIndexBuffer, renderVkState->scratchBuffer, cubeIndices, sizeof(cubeIndices));

	renderVkState->meshAddress = getBufferAddress(renderVkState->device, renderVkState->meshVertexBuffer);

	// Set Push Constants
	renderVkState->meshPushConstants = PushStruct(renderVkState->arena, MeshPushConstants);
	renderVkState->meshPushConstants->vertexAddress = renderVkState->meshAddress;
}

inline FrameData& currentFrame() {
	return renderVkState->frames[renderVkState->frameNumber % MAX_FRAMES];
}

void Render_update() {
	VK_CHECK(vkWaitForFences(renderVkState->device, 1, &currentFrame().renderFence, true, 1000000000));

	u32 swapchainImageIndex{};
	VkResult e = vkAcquireNextImageKHR(renderVkState->device, renderVkState->swapchain->swapchain, 1000000000, currentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (e == VK_ERROR_OUT_OF_DATE_KHR) {
		renderVkState->swapchain->needsResize = true;
		return;
	}

	VK_CHECK(vkResetFences(renderVkState->device, 1, &currentFrame().renderFence));

	// Update scene values
	float aspectRatio = (float)renderVkState->drawExtent.width / (float)renderVkState->drawExtent.height;
	mat4 view = matrixMakeLookAt({0.0f, 1.0f, 2.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
	mat4 projection = matrixMakePerspective(RadFromDeg(70.0f), aspectRatio, 10000.0f, 0.1f);

	// Invert Y-axis
	projection.elements[1][1] *= -1;

	renderVkState->meshPushConstants->mvp = projection * view;

	VkCommandBuffer cmd = currentFrame().commandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Record commands
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	transitionImage(cmd, renderVkState->drawImage->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	VkClearColorValue clearValue = { { 0.0f, 0.0f, 1.0f, 1.0f } };

	VkImageSubresourceRange clearRange {};
	clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearRange.baseMipLevel = 0;
	clearRange.levelCount = VK_REMAINING_MIP_LEVELS;
	clearRange.baseArrayLayer = 0;
	clearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, renderVkState->computePipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, renderVkState->computePipelineLayout, 0, 1, &renderVkState->computeSet, 0, nullptr);

	// Draw with compute
	vkCmdDispatch(cmd, Ceil(renderVkState->drawExtent.width / 16.0), Ceil(renderVkState->drawExtent.height / 16.0), 1);

	transitionImage(cmd, renderVkState->drawImage->image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	transitionImage(cmd, renderVkState->depthImage->image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	// Draw geometry
	VkRenderingAttachmentInfo colorAttachment = attachmentInfo(renderVkState->drawImage->imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(renderVkState->depthImage->imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkRenderingInfo renderInfo = renderingInfo(renderVkState->drawExtent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, renderVkState->meshPipeline);
	vkCmdPushConstants(cmd, renderVkState->meshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants), renderVkState->meshPushConstants);

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = (f32)renderVkState->drawExtent.width;
	viewport.height = (f32)renderVkState->drawExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = renderVkState->drawExtent.width;
	scissor.extent.height = renderVkState->drawExtent.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	vkCmdBindIndexBuffer(cmd, renderVkState->meshIndexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(cmd, ArrayCount(cubeIndices), 1, 0, 0, 0);

	vkCmdEndRendering(cmd);

	transitionImage(cmd, renderVkState->drawImage->image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	transitionImage(cmd, renderVkState->swapchain->images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copyImageToImage(cmd, renderVkState->drawImage->image, renderVkState->swapchain->images[swapchainImageIndex], renderVkState->drawExtent, renderVkState->drawExtent);

	transitionImage(cmd, renderVkState->swapchain->images[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	VK_CHECK(vkEndCommandBuffer(cmd));

	// Submit
	VkCommandBufferSubmitInfo cmdInfo{};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	cmdInfo.pNext = nullptr;
	cmdInfo.commandBuffer = cmd;
	cmdInfo.deviceMask = 0;

	VkSemaphoreSubmitInfo waitInfo{};
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	waitInfo.pNext = nullptr;
	waitInfo.semaphore = currentFrame().swapchainSemaphore;
	waitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR;
	waitInfo.deviceIndex = 0;
	waitInfo.value = 1;

	VkSemaphoreSubmitInfo signalInfo{};
	signalInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	signalInfo.pNext = nullptr;
	signalInfo.semaphore = currentFrame().renderSemaphore;
	signalInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
	signalInfo.deviceIndex = 0;
	signalInfo.value = 1;

	VkSubmitInfo2 submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	submitInfo.pNext = nullptr;

	submitInfo.waitSemaphoreInfoCount = 1;
	submitInfo.pWaitSemaphoreInfos = &waitInfo;

	submitInfo.signalSemaphoreInfoCount = 1;
	submitInfo.pSignalSemaphoreInfos = &signalInfo;

	submitInfo.commandBufferInfoCount = 1;
	submitInfo.pCommandBufferInfos = &cmdInfo;

	VK_CHECK(vkQueueSubmit2(renderVkState->graphicsQueue, 1, &submitInfo, currentFrame().renderFence));

	// Present
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &renderVkState->swapchain->swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &currentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VkResult presentResult = vkQueuePresentKHR(renderVkState->graphicsQueue, &presentInfo);
	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR) {
		renderVkState->swapchain->needsResize = true;
	}

	renderVkState->frameNumber++;
}
