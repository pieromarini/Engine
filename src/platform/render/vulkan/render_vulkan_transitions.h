#pragma once

#include "core/core.h"

#include <volk/volk.h>

inline void copyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize, VkExtent2D dstSize) {
	VkImageBlit2 blitRegion{ .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2 };

	blitRegion.srcOffsets[1].x = (i32)srcSize.width;
	blitRegion.srcOffsets[1].y = (i32)srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = (i32)dstSize.width;
	blitRegion.dstOffsets[1].y = (i32)dstSize.height;
	blitRegion.dstOffsets[1].z = 1;

	blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.srcSubresource.baseArrayLayer = 0;
	blitRegion.srcSubresource.layerCount = 1;
	blitRegion.srcSubresource.mipLevel = 0;

	blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	blitRegion.dstSubresource.baseArrayLayer = 0;
	blitRegion.dstSubresource.layerCount = 1;
	blitRegion.dstSubresource.mipLevel = 0;

	VkBlitImageInfo2 blitInfo{ .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2, .pNext = nullptr };
	blitInfo.dstImage = destination;
	blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	blitInfo.srcImage = source;
	blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	blitInfo.filter = VK_FILTER_LINEAR;
	blitInfo.regionCount = 1;
	blitInfo.pRegions = &blitRegion;

	vkCmdBlitImage2(cmd, &blitInfo);
}

inline VkImageMemoryBarrier2 imageBarrier(VkImage image, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkImageLayout oldLayout, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageLayout newLayout, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, uint32_t baseMipLevel = 0, uint32_t levelCount = VK_REMAINING_MIP_LEVELS) {
	VkImageMemoryBarrier2 result = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };

	result.srcStageMask = srcStageMask;
	result.srcAccessMask = srcAccessMask;
	result.dstStageMask = dstStageMask;
	result.dstAccessMask = dstAccessMask;
	result.oldLayout = oldLayout;
	result.newLayout = newLayout;
	result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.image = image;
	result.subresourceRange.aspectMask = aspectMask;
	result.subresourceRange.baseMipLevel = baseMipLevel;
	result.subresourceRange.levelCount = levelCount;
	result.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

	return result;
}

inline VkBufferMemoryBarrier2 bufferBarrier(VkBuffer buffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) {
	VkBufferMemoryBarrier2 result = { .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

	result.srcStageMask = srcStageMask;
	result.srcAccessMask = srcAccessMask;
	result.dstStageMask = dstStageMask;
	result.dstAccessMask = dstAccessMask;
	result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	result.buffer = buffer;
	result.offset = 0;
	result.size = VK_WHOLE_SIZE;

	return result;
}

inline void pipelineBarrier(VkCommandBuffer commandBuffer, VkDependencyFlags dependencyFlags, size_t bufferBarrierCount, const VkBufferMemoryBarrier2* bufferBarriers, size_t imageBarrierCount, const VkImageMemoryBarrier2* imageBarriers) {
	VkDependencyInfo dependencyInfo = { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.dependencyFlags = dependencyFlags;
	dependencyInfo.bufferMemoryBarrierCount = (u32)bufferBarrierCount;
	dependencyInfo.pBufferMemoryBarriers = bufferBarriers;
	dependencyInfo.imageMemoryBarrierCount = (u32)imageBarrierCount;
	dependencyInfo.pImageMemoryBarriers = imageBarriers;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

inline void stageBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) {
	VkMemoryBarrier2 memoryBarrier = { .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2 };
	memoryBarrier.srcStageMask = srcStageMask;
	memoryBarrier.srcAccessMask = srcAccessMask;
	memoryBarrier.dstStageMask = dstStageMask;
	memoryBarrier.dstAccessMask = dstAccessMask;

	VkDependencyInfo dependencyInfo = { .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
	dependencyInfo.memoryBarrierCount = 1;
	dependencyInfo.pMemoryBarriers = &memoryBarrier;

	vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
}

inline void stageBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 srcStageMask, VkPipelineStageFlags2 dstStageMask) {
	VkAccessFlags2 accessFlags = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
	stageBarrier(commandBuffer, srcStageMask, accessFlags, dstStageMask, accessFlags);
}

inline void stageBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stageMask) {
	stageBarrier(commandBuffer, stageMask, stageMask);
}
