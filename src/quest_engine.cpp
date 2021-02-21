#include "quest_engine.h"
#include "vk_types.h"
#include "vk_initializers.h"

#include <VkBootstrap.h>

#include <SDL.h>
#include <SDL_vulkan.h>

#include <iostream>

using namespace std;
#define VK_CHECK(x)														\
	do																	\
	{																	\
		VkResult err = x;												\
		if (err)														\
		{																\
			std::cout <<"Detected Vulkan error: " << err << std::endl;	\
			abort();													\
		}																\
	} while (0)



void QuestEngine::init()
{
	// initialize SDL
	SDL_Init(SDL_INIT_VIDEO);
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);
	_window = SDL_CreateWindow(
		"Quest Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		_windowExtent.width,
		_windowExtent.height,
		window_flags
	);

	init_vulkan();
	init_swapchain();
	init_commands();
	init_default_renderpass();
	init_framebuffers();
	init_sync_structures();
	init_pipelines();

	//everything went fine
	_isInitialized = true;
}

void QuestEngine::cleanup()
{
	if (_isInitialized)
	{
		VK_CHECK(vkDeviceWaitIdle(_device));

		_mainDeletionQueue.flush();

		vkDestroyDevice(_device, nullptr);
		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);

		SDL_DestroyWindow(_window);
	}
}

void QuestEngine::draw()
{
	VK_CHECK(vkWaitForFences(_device, 1, &_renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &_renderFence));

	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, _presentSemaphore, nullptr, &swapchainImageIndex));
	VK_CHECK(vkResetCommandBuffer(_mainCommandBuffer, 0));

	VkCommandBuffer cmd = _mainCommandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo = Quest::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue clearValue;
	clearValue.color = { { 0.05f, 0.05f, 0.05f, 1.0f } };

	VkRenderPassBeginInfo rpInfo = Quest::renderpass_begin_info(_renderPass, _windowExtent, _framebuffers[swapchainImageIndex]);
	rpInfo.clearValueCount = 1;
	rpInfo.pClearValues = &clearValue;
	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
	{
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _trianglePipeline);
		vkCmdDraw(cmd, 3, 1, 0, 0);
	}
	vkCmdEndRenderPass(cmd);
	VK_CHECK(vkEndCommandBuffer(cmd));

	//Submit
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submit = Quest::submit_info(&cmd);
	submit.pWaitDstStageMask = &waitStage;
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &_presentSemaphore;
	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &_renderSemaphore;
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _renderFence));

	//Present
	VkPresentInfoKHR presentInfo = Quest::present_info();
	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pWaitSemaphores = &_renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pImageIndices = &swapchainImageIndex;
	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	_frameNumber++;
}

void QuestEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	//main loop
	while (!bQuit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT) bQuit = true;
		}
		draw();
	}
}

void QuestEngine::init_vulkan()
{
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Quest Application")
		.request_validation_layers(true)
		.require_api_version(1, 2, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkb_inst = inst_ret.value();
	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, _instance, &_surface);

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	VkPhysicalDeviceFeatures feats{};
	feats.pipelineStatisticsQuery = true;
	feats.multiDrawIndirect = true;
	feats.drawIndirectFirstInstance = true;
	feats.samplerAnisotropy = true;
	feats.fillModeNonSolid = true;
	selector.set_required_features(feats);

	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 2)
		.set_surface(_surface)
		.add_required_extension(VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME)
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	vkb::Device vkbDevice = deviceBuilder.build().value();
	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;
	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
}

void QuestEngine::init_swapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };
	VkSurfaceFormatKHR desiredSurfaceFormat = { VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_format(desiredSurfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.build()
		.value();

	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();
	_swapchainImageFormat = vkbSwapchain.image_format;

	_mainDeletionQueue.push_function([=]()
	{
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	});
}

void QuestEngine::init_commands()
{
	VkCommandPoolCreateInfo commandPoolInfo = Quest::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_commandPool));
	
	VkCommandBufferAllocateInfo cmdAllocInfo = Quest::command_buffer_allocate_info(_commandPool, 1);
	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_mainCommandBuffer));

	_mainDeletionQueue.push_function([=]()
	{
		vkDestroyCommandPool(_device, _commandPool, nullptr);
	});
}

void QuestEngine::init_default_renderpass()
{
	VkAttachmentDescription color_attachment = {};
	color_attachment.format = _swapchainImageFormat;
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount = 1;
	render_pass_info.pAttachments = &color_attachment;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;
	VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

	_mainDeletionQueue.push_function([=]()
	{
		vkDestroyRenderPass(_device, _renderPass, nullptr);
	});
}

void QuestEngine::init_framebuffers()
{
	VkFramebufferCreateInfo fb_info = Quest::framebuffer_create_info(_renderPass, { _windowExtent.width, _windowExtent.height });
	const uint32_t swapchain_imagecount = static_cast<uint32_t>(_swapchainImages.size());
	_framebuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	for (uint32_t i = 0; i < swapchain_imagecount; i++) {

		fb_info.pAttachments = &_swapchainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_framebuffers[i]));

		_mainDeletionQueue.push_function([=]()
		{
			vkDestroyFramebuffer(_device, _framebuffers[i], nullptr);
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		});
	}
}

void QuestEngine::init_sync_structures()
{
	VkFenceCreateInfo fenceCreateInfo = Quest::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
	VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_renderFence));

	VkSemaphoreCreateInfo semaphoreCreateInfo = Quest::semaphore_create_info();
	VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_presentSemaphore));
	VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_renderSemaphore));
	
	_mainDeletionQueue.push_function([=]()
	{
		vkDestroySemaphore(_device, _presentSemaphore, nullptr);
		vkDestroySemaphore(_device, _renderSemaphore, nullptr);
		vkDestroyFence(_device, _renderFence, nullptr);
	});
}

void QuestEngine::init_pipelines()
{
	VkShaderModule triangleVertexShader;
	VkShaderModule triangleFragShader;
	if (!load_shader_module("../../shaders/triangle.vert.spv", &triangleVertexShader))
		std::cout << "Error when building the triangle vertex shader module" << std::endl;
	else
		std::cout << "Triangle vertex shader succesfully loaded" << std::endl;
	
	if (!load_shader_module("../../shaders/triangle.frag.spv", &triangleFragShader))
		std::cout << "Error when building the triangle fragment shader module" << std::endl;
	else
		std::cout << "Triangle fragment shader succesfully loaded" << std::endl;

	VkPipelineLayoutCreateInfo pipeline_layout_info = Quest::pipeline_layout_create_info();
	VK_CHECK(vkCreatePipelineLayout(_device, &pipeline_layout_info, nullptr, &_trianglePipelineLayout));

	PipelineBuilder pipelineBuilder;
	pipelineBuilder._shaderStages.push_back(Quest::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triangleVertexShader));
	pipelineBuilder._shaderStages.push_back(Quest::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));
	pipelineBuilder._vertexInputInfo = Quest::vertex_input_state_create_info();
	pipelineBuilder._inputAssembly = Quest::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder._viewport.x = 0.0f;
	pipelineBuilder._viewport.y = 0.0f;
	pipelineBuilder._viewport.width = (float)_windowExtent.width;
	pipelineBuilder._viewport.height = (float)_windowExtent.height;
	pipelineBuilder._viewport.minDepth = 0.0f;
	pipelineBuilder._viewport.maxDepth = 1.0f;
	pipelineBuilder._scissor.offset = { 0, 0 };
	pipelineBuilder._scissor.extent = _windowExtent;
	pipelineBuilder._rasterizer = Quest::rasterization_state_create_info(VK_POLYGON_MODE_FILL);
	pipelineBuilder._multisampling = Quest::multisampling_state_create_info();
	pipelineBuilder._colorBlendAttachment = Quest::color_blend_attachment_state();
	pipelineBuilder._pipelineLayout = _trianglePipelineLayout;
	_trianglePipeline = pipelineBuilder.build_pipeline(_device, _renderPass);

	_mainDeletionQueue.push_function([=]()
	{
		vkDestroyPipeline(_device, _trianglePipeline, nullptr);
		vkDestroyPipelineLayout(_device, _trianglePipelineLayout, nullptr);
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, triangleVertexShader, nullptr);
	});
}

bool QuestEngine::load_shader_module(const char* filePath, VkShaderModule* outShaderModule)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		return false;
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}
	*outShaderModule = shaderModule;
	return true;
}

//////////////////////////////////////////////////////////////////////////////
///PipelineBuilder
//////////////////////////////////////////////////////////////////////////////
VkPipeline PipelineBuilder::build_pipeline(VkDevice device, VkRenderPass pass)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &_scissor;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;
	pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
	{
		std::cout << "failed to create pipline\n";
		return VK_NULL_HANDLE;
	}
	else
	{
		return newPipeline;
	}
}
