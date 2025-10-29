#pragma once

#include <cstdio>
#include "core/memory/arena.h"
#include "platform/os/core/os_core.h"

// TODO(piero): move this win32 define
#define VK_USE_PLATFORM_WIN32_KHR
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#define MAX_FRAMES 2

enum SwapchainStatus {
	Swapchain_Ready,
	Swapchain_Resized,
	Swapchain_NotReady,
};

struct RenderVkSwapchain {
	VkSwapchainKHR swapchain;
	u32 width;
	u32 height;
	VkImage* images;
	u32 imageCount;
};

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* data;
	size_t size;
};

struct RenderVkImage {
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
};

struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore swapchainSemaphore;
	VkSemaphore renderSemaphore;
	VkFence renderFence;
};

struct RenderVkState {
	Arena* arena;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	// sync
	VkSemaphore acquireSemaphores[MAX_FRAMES];
	VkFence frameFences[MAX_FRAMES];

	VkSemaphore* presentSemaphores;
	u32 presentSemaphoresCount;

	VkImageView* swapchainImageViews;
	u32 swapchainImageViewsCount;

	VkQueue graphicsQueue;
	u32 graphicsQueueFamily;

	VkSurfaceKHR surface;
	OSWindowHandle window;
	RenderVkSwapchain* swapchain;

	Arena* frameArena;
	FrameData frames[MAX_FRAMES];
	u32 frameNumber;
};

#define VK_CHECK(call) \
	do \
	{ \
		VkResult result_ = call; \
		Assert(result_ == VK_SUCCESS); \
	} while (0)

#define VK_CHECK_FORCE(call) \
	do \
	{ \
		VkResult result_ = call; \
		if (result_ != VK_SUCCESS) \
		{ \
			fprintf(stderr, "%s:%d: %s failed with error %d\n", __FILE__, __LINE__, #call, result_); \
			abort(); \
		} \
	} while (0)

#define VK_CHECK_SWAPCHAIN(call) \
	do \
	{ \
		VkResult result_ = call; \
		Assert(result_ == VK_SUCCESS || result_ == VK_SUBOPTIMAL_KHR || result_ == VK_ERROR_OUT_OF_DATE_KHR); \
	} while (0)

#define VK_CHECK_QUERY(call) \
	do \
	{ \
		VkResult result_ = call; \
		Assert(result_ == VK_SUCCESS || result_ == VK_NOT_READY); \
	} while (0)

#ifdef DEBUG
#define KHR_VALIDATION 1
#else
#define KHR_VALIDATION CONFIG_RELVAL
#endif

#define SYNC_VALIDATION 1

#define API_VERSION VK_API_VERSION_1_3

static bool isLayerSupported(const char* name);
bool isInstanceExtensionSupported(const char* name);

VkSemaphore createSemaphore(VkDevice device, VkSemaphoreCreateFlags flags = 0);
VkFence createFence(VkDevice device, VkFenceCreateFlags flags = 0);
VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount, VkQueryType queryType);

static u32 selectMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, u32 memoryTypeBits, VkMemoryPropertyFlags flags);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevel, u32 levelCount);
RenderVkImage* createImage(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageUsageFlags usage);
void destroyImage(VkDevice device, RenderVkImage* image);

VkSampler createSampler(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, VkSamplerReductionModeEXT reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT);

static bool isLayerSupported(const char* name);
bool isInstanceExtensionSupported(const char* name);
const char** getSwapchainExtensions(uint32_t* count);

VkInstance createInstance();
VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);
uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice);
static bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex);
VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount);
VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex);

// Platform-specific surface creation
void Render_Vk_createSurfaceWin32(OSWindowHandle windowHandle);

// swapchain
static VkPresentModeKHR getPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
RenderVkSwapchain* createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format, VkSwapchainKHR oldSwapchain = nullptr);
void destroySwapchain(VkDevice device, RenderVkSwapchain* swapchain);
SwapchainStatus updateSwapchain(RenderVkSwapchain* oldSwapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format);
void Render_VK_updateWindowSize(OSWindowHandle windowHandle, vec2 size);

void Render_Vk_init();
void Render_Vk_initCommands();
void Render_Vk_initSync();
void Render_Vk_update();

void Render_Vk_equipWindow(OSWindowHandle windowHandle);
