#include "core/config.h"
#include "core/core.h"
#include "core/memory/arena.h"
#include "core/thread_context.h"
#include "platform/os/gfx/os_gfx_win32.h"
#include "platform/render/vulkan/render_vulkan_transitions.h"
#include "vulkan/vulkan_core.h"

#include "render_vulkan.h"

per_thread RenderVkState* renderVkState;

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

void Render_Vk_createSurfaceWin32(OSWindowHandle windowHandle) {
	Win32Window* window = OS_windowFromHandle(windowHandle);

	VkWin32SurfaceCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = window->hwnd;
	createInfo.hinstance = GetModuleHandle(nullptr);

	VkSurfaceKHR surface{};
	VK_CHECK(vkCreateWin32SurfaceKHR(renderVkState->instance, &createInfo, nullptr, &surface));
	renderVkState->surface = surface;
	renderVkState->window = windowHandle;
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

static VkPresentModeKHR getPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
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

SwapchainStatus updateSwapchain(RenderVkSwapchain* oldSwapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format) {
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

	destroySwapchain(device, oldSwapchain);

	return Swapchain_Resized;
}

void Render_VK_updateWindowSize(OSWindowHandle windowHandle, vec2 size) {
	u32 lastWidth = renderVkState->swapchain->width;
	u32 lastHeight = renderVkState->swapchain->height;

	// No need to resize.
	if ((u32)size.x == lastWidth && (u32)size.y == lastHeight) {
		return;
	}

	vkDeviceWaitIdle(renderVkState->device);
	VkFormat swapchainFormat = getSwapchainFormat(renderVkState->physicalDevice, renderVkState->surface);
	updateSwapchain(renderVkState->swapchain, renderVkState->physicalDevice, renderVkState->device, renderVkState->surface, renderVkState->graphicsQueueFamily, windowHandle, swapchainFormat);
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

	return result;
}

void destroyImage(VkDevice device, RenderVkImage* image) {
	vkDestroyImageView(device, image->imageView, nullptr);
	vkDestroyImage(device, image->image, nullptr);
	vkFreeMemory(device, image->memory, nullptr);
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

void Render_Vk_init() {
	// Init state
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

	Render_Vk_initCommands();
	Render_Vk_initSync();
}

void Render_Vk_initCommands() {
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
}

void Render_Vk_initSync() {
	for (int i = 0; i < MAX_FRAMES; i++) {
		renderVkState->frames[i].renderFence = createFence(renderVkState->device, VK_FENCE_CREATE_SIGNALED_BIT);
		renderVkState->frames[i].renderSemaphore = createSemaphore(renderVkState->device);
		renderVkState->frames[i].swapchainSemaphore = createSemaphore(renderVkState->device);
	}
}

void Render_Vk_equipWindow(OSWindowHandle windowHandle) {
	VkDevice device = renderVkState->device;
	VkPhysicalDevice physicalDevice = renderVkState->physicalDevice;

	Render_Vk_createSurfaceWin32(windowHandle);

	VkBool32 presentSupported = 0;
	VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, renderVkState->graphicsQueueFamily, renderVkState->surface, &presentSupported));
	if (!presentSupported) {
		printf("[Render] No surface with present support. Exiting");
		OS_abort();
	}

	VkFormat swapchainFormat = getSwapchainFormat(physicalDevice, renderVkState->surface);

	RenderVkSwapchain* swapchain = createSwapchain(physicalDevice, device, renderVkState->surface, renderVkState->graphicsQueueFamily, renderVkState->window, swapchainFormat);
	renderVkState->swapchain = swapchain;
}

inline FrameData& currentFrame() {
	return renderVkState->frames[renderVkState->frameNumber % MAX_FRAMES];
}

void Render_Vk_update() {
	VK_CHECK(vkWaitForFences(renderVkState->device, 1, &currentFrame().renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(renderVkState->device, 1, &currentFrame().renderFence));

	u32 swapchainImageIndex{};
	VK_CHECK(vkAcquireNextImageKHR(renderVkState->device, renderVkState->swapchain->swapchain, 1000000000, currentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex));

	VkCommandBuffer cmd = currentFrame().commandBuffer;
	VK_CHECK(vkResetCommandBuffer(cmd, 0));

	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	// Record commands
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	transitionImage(cmd, renderVkState->swapchain->images[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

	VkClearColorValue clearValue = { { 0.0f, 0.0f, 1.0f, 1.0f } };

	VkImageSubresourceRange clearRange {};
	clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearRange.baseMipLevel = 0;
	clearRange.levelCount = VK_REMAINING_MIP_LEVELS;
	clearRange.baseArrayLayer = 0;
	clearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	vkCmdClearColorImage(cmd, renderVkState->swapchain->images[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

	transitionImage(cmd, renderVkState->swapchain->images[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

	VK_CHECK(vkQueuePresentKHR(renderVkState->graphicsQueue, &presentInfo));

	renderVkState->frameNumber++;
}
