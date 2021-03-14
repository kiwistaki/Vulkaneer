#pragma once
#include "vk_types.h"
#include "vk_mesh.h"

#include <deque>
#include <functional>
#include <unordered_map>
#include <glm/glm.hpp>

struct Material
{
	VkDescriptorSet textureSet{ VK_NULL_HANDLE };
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
	Mesh* mesh;
	Material* material;
	glm::mat4 transformMatrix;
};

struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct GPUObjectData
{
	glm::mat4 modelMatrix;
};

struct GPUSceneData
{
	glm::vec4 fogColor; // w is for exponent
	glm::vec4 fogDistances; //x for min, y for max, zw unused.
	glm::vec4 ambientColor;
	glm::vec4 sunlightDirection; //w for sun power
	glm::vec4 sunlightColor;
};

struct GPUCameraData
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct UploadContext
{
	VkFence _uploadFence;
	VkCommandPool _commandPool;
};

struct FrameData
{
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	AllocatedBuffer cameraBuffer;
	VkDescriptorSet globalDescriptor;
	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;
};

struct Texture
{
	AllocatedImage image;
	VkImageView imageView;
};

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

constexpr unsigned int FRAME_OVERLAP = 3;

class Vulkaneer
{
public:
	void init();
	void cleanup();
	void draw();
	void run();

	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	Material* get_material(const std::string& name);
	Mesh* get_mesh(const std::string& name);

	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

	FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; }
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_default_renderpass();
	void init_framebuffers();
	void init_sync_structures();
	void init_descriptors();
	void init_pipelines();
	void init_scene();

	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);

	void load_images();
	void load_meshes();
	void upload_mesh(Mesh& mesh);

	size_t pad_uniform_buffer_size(size_t originalSize);

public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };

	DeletionQueue _mainDeletionQueue;
	VmaAllocator _allocator;

	VkExtent2D _windowExtent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkPhysicalDeviceProperties _gpuProperties;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;
	VkFormat _depthFormat;
	AllocatedImage _depthImage;
	VkImageView _depthImageView;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkRenderPass _renderPass;
	std::vector<VkFramebuffer> _framebuffers;

	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _objectSetLayout;
	VkDescriptorSetLayout _singleTextureSetLayout;
	VkDescriptorPool _descriptorPool;

	FrameData _frames[FRAME_OVERLAP];
	UploadContext _uploadContext;

	GPUSceneData _sceneParameters;
	AllocatedBuffer _sceneParameterBuffer;

	std::vector<RenderObject> _renderables;
	std::unordered_map<std::string, Mesh> _meshes;
	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Texture> _loadedTextures;
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
	VkPipelineDepthStencilStateCreateInfo _depthStencil;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
};
