#pragma once
#include <vk_types.h>
#include <functional>
#include <deque>


struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function)
	{
		deletors.push_back(function);
	}

	void flush()
	{
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)();
		}
		deletors.clear();
	}
};

class QuestEngine
{
public:
	void init();
	void cleanup();
	void draw();
	void run();

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_default_renderpass();
	void init_framebuffers();
	void init_sync_structures();
	void init_pipelines();

	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);

public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };

	DeletionQueue _mainDeletionQueue;

	VkExtent2D _windowExtent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkRenderPass _renderPass;
	std::vector<VkFramebuffer> _framebuffers;

	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;
};

class PipelineBuilder
{
public:
	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);

public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
};
