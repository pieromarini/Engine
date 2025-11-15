#pragma once

#include <cstdio>
#include "core/core_strings.h"
#include "core/memory/arena.h"
#include "platform/os/core/os_core.h"

#include "parsers/gltf/parser_gltf_inc.h"

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
	b32 needsResize;
};

struct RenderVkImage {
	VkImage image;
	VkImageView imageView;
	VkDeviceMemory memory;
	VkFormat format;
};

struct RenderVkBuffer {
	VkBuffer buffer;
	VkDeviceMemory memory;
	void* data;
	u32 size;
};

struct FrameData {
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;

	VkSemaphore swapchainSemaphore;
	VkSemaphore renderSemaphore;
	VkFence renderFence;
};

struct MeshPushConstants {
	mat4 viewProj;
	VkDeviceAddress vertexAddress;
};

// Indirect command
struct MeshDrawCommand {
	uint32_t drawId;
	VkDrawIndexedIndirectCommand indirect;
};

struct alignas(16) MeshDrawData {
	mat4 transform;
	u32 materialIndex;
	f32 padding[3];
};

struct GPUMesh {
	GPUMesh* next;
	GPUMesh* prev;

	RenderVkBuffer vertexBuffer;
	RenderVkBuffer indexBuffer;
	VkDeviceAddress vertexAddress;

	u32 vertexCount;
	u32 indexCount;

	RenderVkBuffer drawCommandBuffer;
	u32 drawCommandCount;

	RenderVkBuffer drawDataBuffer;
	RenderVkBuffer materialDataBuffer;
	u32 materialCount;
};

struct RenderVkState {
	Arena* arena;
	Arena* sceneArena;
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue graphicsQueue;
	u32 graphicsQueueFamily;

	VkSurfaceKHR surface;
	OSWindowHandle window;
	RenderVkSwapchain* swapchain;

	RenderVkImage* drawImage;
	RenderVkImage* depthImage;
	VkExtent2D drawExtent;

	MeshPushConstants* meshPushConstants;

	// Descriptors
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout drawDataLayout;
	VkDescriptorSet drawDataDescriptorSet;

	// Pipelines
	VkPipeline meshPipeline;
	VkPipelineLayout meshPipelineLayout;

	// Renderable meshes
	GPUMesh* firstMesh;
	GPUMesh* lastMesh;
	u32 meshCount;

	// Immediate Submit
	VkFence immFence;
	VkCommandPool immCommandPool;
	VkCommandBuffer immCommandBuffer;

	RenderVkBuffer scratchBuffer;

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
#define SYNC_VALIDATION 1
#else
#define KHR_VALIDATION CONFIG_VALIDATION
#define SYNC_VALIDATION CONFIG_SYNC_VALIDATION
#endif

#define API_VERSION VK_API_VERSION_1_3

static bool isLayerSupported(const char* name);
bool isInstanceExtensionSupported(const char* name);

VkSemaphore createSemaphore(VkDevice device, VkSemaphoreCreateFlags flags = 0);
VkFence createFence(VkDevice device, VkFenceCreateFlags flags = 0);
VkQueryPool createQueryPool(VkDevice device, uint32_t queryCount, VkQueryType queryType);

static u32 selectMemoryType(const VkPhysicalDeviceMemoryProperties& memoryProperties, u32 memoryTypeBits, VkMemoryPropertyFlags flags);

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, u32 mipLevel, u32 levelCount);
RenderVkImage* createImage(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageUsageFlags usage);
RenderVkImage* createImage(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, u8* data, u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageUsageFlags usage);
void destroyImage(VkDevice device, RenderVkImage* image);

RenderVkBuffer createBuffer(VkDevice device, VkPhysicalDeviceMemoryProperties& memoryProperties, u32 size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryFlags);
void uploadBuffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue, const RenderVkBuffer& buffer, const RenderVkBuffer& scratch, void* data, u32 size);
void destroyBuffer(VkDevice device, const RenderVkBuffer& buffer);

VkDeviceAddress getBufferAddress(VkDevice device, const RenderVkBuffer& buffer);

VkSampler createSampler(VkDevice device, VkFilter filter, VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode, VkSamplerReductionModeEXT reductionMode = VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT);

static bool isLayerSupported(const char* name);
bool isInstanceExtensionSupported(const char* name);
String8List getSwapchainExtensions(Arena* arena);

// Platform-specific surface creation
VkSurfaceKHR createSurfaceWin32(OSWindowHandle windowHandle);

// swapchain
VkPresentModeKHR getPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
VkFormat getSwapchainFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
RenderVkSwapchain* createSwapchain(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format, VkSwapchainKHR oldSwapchain = nullptr);
void destroySwapchain(VkDevice device, RenderVkSwapchain* swapchain);
SwapchainStatus recreateSwapchain(RenderVkSwapchain* oldSwapchain, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, u32 familyIndex, OSWindowHandle windowHandle, VkFormat format);

// Compares the passed in size with the stored state window's size to see if a swapchain recreation is necessary
void recreateSwapchainIfNeeded(OSWindowHandle windowHandle, vec2 size);

// init helpers
void initCommands();
void initSync();
void initDescriptors();
void initPipelines();

VkInstance createInstance();
VkDebugReportCallbackEXT registerDebugCallback(VkInstance instance);
uint32_t getGraphicsFamilyIndex(VkPhysicalDevice physicalDevice);
static bool supportsPresentation(VkPhysicalDevice physicalDevice, uint32_t familyIndex);
VkPhysicalDevice pickPhysicalDevice(VkPhysicalDevice* physicalDevices, uint32_t physicalDeviceCount);
VkDevice createDevice(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t familyIndex);

// Descriptors
VkDescriptorSetLayout buildDescriptorLayout(VkDescriptorSetLayoutBinding* bindings, u32 numBindings, VkDescriptorSetLayoutCreateFlags flags);
VkDescriptorPool buildDescriptorPool(u32 maxSets, VkDescriptorPoolSize* poolSizes, u32 poolSizesCount);
VkDescriptorSet buildDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout);

// Pipelines
VkPipeline buildPipeline(VkDevice device, VkPipelineLayout pipelineLayout, VkPipelineShaderStageCreateInfo* shaderStages, u32 shaderStagesCount, VkPrimitiveTopology topology, VkPolygonMode mode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkFormat colorAttachmentFormat, VkFormat depthAttachmentFormat, b32 blendingEnabled);
