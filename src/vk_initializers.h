#pragma once 
#include "vk_types.h"

namespace Quest
{
	VkBufferMemoryBarrier buffer_barrier(VkBuffer buffer, uint32_t queue);
	VkBufferCreateInfo buffer_create_info(uint32_t size, VkBufferUsageFlags usage);
	VkPipelineColorBlendAttachmentState color_blend_attachment_state();
	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolResetFlags flags = 0);
	VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
	VkDescriptorSetLayoutBinding descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
	VkDeviceQueueCreateInfo device_queue_create_info(uint32_t queueFamilyIndex, uint32_t queueCount = 1);
	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);
	VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass, VkExtent2D extent);
	VkImageMemoryBarrier image_barrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);
	VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);
	VkMemoryAllocateInfo memory_allocate_info(VkDeviceSize size, uint32_t type);
	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();
	VkPipelineLayoutCreateInfo pipeline_layout_create_info();
	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPresentInfoKHR present_info();
	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);
	VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);
	VkSamplerCreateInfo sampler_create_info(VkFilter filters, VkSamplerAddressMode samplerAdressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);
	VkSubmitInfo submit_info(VkCommandBuffer* cmd);
	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
	VkWriteDescriptorSet write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo* bufferInfo, uint32_t binding);
	VkWriteDescriptorSet write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
}
