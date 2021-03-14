﻿#include "vk_shaders.h"
#include "vk_initializers.h"

#include <spirv_reflect.h>

#include <algorithm>
#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

bool vkn::load_shader_module(VkDevice device,const char* filePath, ShaderModule* outShaderModule)
{
	std::ifstream file(filePath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
		return false;

	size_t fileSize = (size_t)file.tellg();
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	VkShaderModule shaderModule;
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.codeSize = buffer.size() * sizeof(uint32_t);
	createInfo.pCode = buffer.data();
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		return false;

	outShaderModule->code = std::move(buffer);
	outShaderModule->module = shaderModule;
	return true;
}

// FNV-1a 32bit hashing algorithm.
constexpr uint32_t fnv1a_32(char const* s, std::size_t count)
{
	return ((count ? fnv1a_32(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
}

uint32_t vkn::hash_descriptor_layout_info(VkDescriptorSetLayoutCreateInfo* info)
{
	std::stringstream ss;
	ss << info->flags;
	ss << info->bindingCount;

	for (auto i = 0u; i < info->bindingCount; i++)
	{
		const VkDescriptorSetLayoutBinding &binding = info->pBindings[i];
		ss << binding.binding;
		ss << binding.descriptorCount;
		ss << binding.descriptorType;
		ss << binding.stageFlags;
	}

	auto str = ss.str();
	return fnv1a_32(str.c_str(),str.length());
}

void ShaderEffect::add_stage(ShaderModule* shaderModule, VkShaderStageFlagBits stage)
{
	ShaderStage newStage = { shaderModule,stage };
	stages.push_back(newStage);
}

struct DescriptorSetLayoutData
{
	uint32_t set_number;
	VkDescriptorSetLayoutCreateInfo create_info;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
};

void ShaderEffect::reflect_layout(VkDevice device, ReflectionOverrides* overrides, int overrideCount)
{
	std::vector<DescriptorSetLayoutData> set_layouts;
	std::vector<VkPushConstantRange> constant_ranges;

	for (auto& s : stages)
	{
		SpvReflectShaderModule spvmodule;
		SpvReflectResult result = spvReflectCreateShaderModule(s.shaderModule->code.size() * sizeof(uint32_t), s.shaderModule->code.data(), &spvmodule);
	
		uint32_t count = 0;
		result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, NULL);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		std::vector<SpvReflectDescriptorSet*> sets(count);
		result = spvReflectEnumerateDescriptorSets(&spvmodule, &count, sets.data());
		assert(result == SPV_REFLECT_RESULT_SUCCESS);	

		for (size_t i_set = 0; i_set < sets.size(); ++i_set)
		{
			const SpvReflectDescriptorSet& refl_set = *(sets[i_set]);
			DescriptorSetLayoutData layout = {};
			layout.bindings.resize(refl_set.binding_count);

			for (uint32_t i_binding = 0; i_binding < refl_set.binding_count; ++i_binding)
			{
				const SpvReflectDescriptorBinding& refl_binding = *(refl_set.bindings[i_binding]);
				VkDescriptorSetLayoutBinding& layout_binding = layout.bindings[i_binding];
				layout_binding.binding = refl_binding.binding;
				layout_binding.descriptorType = static_cast<VkDescriptorType>(refl_binding.descriptor_type);

				for (int ov = 0; ov < overrideCount; ov++)
				{
					if (strcmp(refl_binding.name, overrides[ov].name) == 0)
						layout_binding.descriptorType = overrides[ov].overridenType;
				}

				layout_binding.descriptorCount = 1;
				for (uint32_t i_dim = 0; i_dim < refl_binding.array.dims_count; ++i_dim)
					layout_binding.descriptorCount *= refl_binding.array.dims[i_dim];

				layout_binding.stageFlags = static_cast<VkShaderStageFlagBits>(spvmodule.shader_stage);

				ReflectedBinding reflected;
				reflected.binding = layout_binding.binding;
				reflected.set = refl_set.set;
				reflected.type = layout_binding.descriptorType;
				bindings[refl_binding.name] = reflected;
			}

			layout.set_number = refl_set.set;
			layout.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layout.create_info.bindingCount = refl_set.binding_count;
			layout.create_info.pBindings = layout.bindings.data();
			set_layouts.push_back(layout);
		}

		result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, NULL);
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		std::vector<SpvReflectBlockVariable*> pconstants(count);
		result = spvReflectEnumeratePushConstantBlocks(&spvmodule, &count, pconstants.data());
		assert(result == SPV_REFLECT_RESULT_SUCCESS);

		if (count > 0)
		{
			VkPushConstantRange pcs{};
			pcs.offset = pconstants[0]->offset;
			pcs.size = pconstants[0]->size;
			pcs.stageFlags = s.stage;
			constant_ranges.push_back(pcs);
		}
	}

	std::array<DescriptorSetLayoutData,4> merged_layouts;
	for (int i = 0; i < 4; i++)
	{
		DescriptorSetLayoutData &ly = merged_layouts[i];
		ly.set_number = i;
		ly.create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		std::unordered_map<int,VkDescriptorSetLayoutBinding> binds;
		for (auto& s : set_layouts)
		{
			if (s.set_number == i)
			{
				for (auto& b : s.bindings)
				{
					auto it = binds.find(b.binding);
					if (it == binds.end())
						binds[b.binding] = b;
					else
						binds[b.binding].stageFlags |= b.stageFlags;
				}
			}
		}

		for (auto [k, v] : binds)
		{
			ly.bindings.push_back(v);
		}

		//sort the bindings, for hash purposes
		std::sort(ly.bindings.begin(), ly.bindings.end(), [](VkDescriptorSetLayoutBinding& a, VkDescriptorSetLayoutBinding& b)
		{
			return a.binding < b.binding;
		});

		ly.create_info.bindingCount = (uint32_t)ly.bindings.size();
		ly.create_info.pBindings = ly.bindings.data();
		ly.create_info.flags = 0;
		ly.create_info.pNext = 0;

		if (ly.create_info.bindingCount > 0)
		{
			setHashes[i] = vkn::hash_descriptor_layout_info(&ly.create_info);
			vkCreateDescriptorSetLayout(device, &ly.create_info, nullptr, &setLayouts[i]);
		}
		else
		{
			setHashes[i] = 0;
			setLayouts[i] = VK_NULL_HANDLE;
		}
	}

	std::array<VkDescriptorSetLayout,4> compactedLayouts;
	int s = 0;
	for (int i = 0; i < 4; i++)
	{
		if (setLayouts[i] != VK_NULL_HANDLE)
		{
			compactedLayouts[s] = setLayouts[i];
			s++;
		}
	}

	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkn::pipeline_layout_create_info();
	mesh_pipeline_layout_info.pPushConstantRanges = constant_ranges.data();
	mesh_pipeline_layout_info.pushConstantRangeCount = (uint32_t)constant_ranges.size();
	mesh_pipeline_layout_info.setLayoutCount = s;
	mesh_pipeline_layout_info.pSetLayouts = compactedLayouts.data();
	vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &builtLayout);
}


void ShaderEffect::fill_stages(std::vector<VkPipelineShaderStageCreateInfo>& pipelineStages)
{
	for (auto& s : stages)
		pipelineStages.push_back(vkn::pipeline_shader_stage_create_info(s.stage, s.shaderModule->module));
}

void ShaderDescriptorBinder::bind_buffer(const char* name, const VkDescriptorBufferInfo& bufferInfo)
{
	bind_dynamic_buffer(name, -1, bufferInfo);
}


void ShaderDescriptorBinder::bind_dynamic_buffer(const char* name, uint32_t offset, const VkDescriptorBufferInfo& bufferInfo)
{
	auto found = shaders->bindings.find(name);
	if (found != shaders->bindings.end())
	{
		const ShaderEffect::ReflectedBinding& bind = (*found).second;

		for (auto& write : bufferWrites)
		{
			if (write.dstBinding == bind.binding
				&& write.dstSet == bind.set)
			{
				if (write.bufferInfo.buffer != bufferInfo.buffer ||
					write.bufferInfo.range != bufferInfo.range ||
					write.bufferInfo.offset != bufferInfo.offset)
				{
					write.bufferInfo = bufferInfo;
					write.dynamic_offset = offset;
					cachedDescriptorSets[write.dstSet] = VK_NULL_HANDLE;
				}
				else
				{
					//already in the write list, but matches buffer
					write.dynamic_offset = offset;
				}
				return;
			}
		}

		BufferWriteDescriptor newWrite;
		newWrite.dstSet = bind.set;
		newWrite.dstBinding = bind.binding;
		newWrite.descriptorType = bind.type;
		newWrite.bufferInfo = bufferInfo;
		newWrite.dynamic_offset = offset;
		cachedDescriptorSets[bind.set] = VK_NULL_HANDLE;
		bufferWrites.push_back(newWrite);
	}
}

void ShaderDescriptorBinder::apply_binds(VkCommandBuffer cmd)
{
	for (int i = 0; i < 2; i++)
	{
		//there are writes for this set
		if (cachedDescriptorSets[i] != VK_NULL_HANDLE)
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, shaders->builtLayout, i, 1, &cachedDescriptorSets[i], setOffsets[i].count, setOffsets[i].offsets.data());
	}
}

void ShaderDescriptorBinder::build_sets(VkDevice device, vkn::DescriptorAllocator& allocator)
{
	std::array<std::vector<VkWriteDescriptorSet>, 4> writes{};

	std::sort(bufferWrites.begin(), bufferWrites.end(), [](BufferWriteDescriptor& a, BufferWriteDescriptor& b)
	{
		if (b.dstSet == a.dstSet)
			return a.dstSet < b.dstSet;
		else
			return a.dstBinding < b.dstBinding;
	});
	
	//reset the dynamic offsets
	for (auto& s : setOffsets)
		s.count = 0;

	for (BufferWriteDescriptor& w : bufferWrites)
	{
		uint32_t set = w.dstSet;
		VkWriteDescriptorSet write = vkn::write_descriptor_buffer(w.descriptorType, VK_NULL_HANDLE, &w.bufferInfo, w.dstBinding);
		writes[set].push_back(write);

		//dynamic offsets
		if (w.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || w.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
		{
			DynOffsets& offsetSet = setOffsets[set];
			offsetSet.offsets[offsetSet.count] = w.dynamic_offset;
			offsetSet.count++;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		//there are writes for this set
		if (writes[i].size() > 0)
		{
			if (cachedDescriptorSets[i] == VK_NULL_HANDLE)
			{
				//alloc
				auto layout = shaders->setLayouts[i];
				VkDescriptorSet newDescriptor;
				allocator.allocate(&newDescriptor, layout);

				for (auto& w : writes[i]) 
					w.dstSet = newDescriptor;

				vkUpdateDescriptorSets(device, (uint32_t)writes[i].size(), writes[i].data(), 0, nullptr);
				cachedDescriptorSets[i] = newDescriptor;
			}
		}
	}
}

void ShaderDescriptorBinder::set_shader(ShaderEffect* newShader)
{
	//invalidate nonequal layouts
	if (shaders &&  shaders != newShader)
	{
		for (int i = 0; i < 4; i++)
		{
			if (newShader->setHashes[i] != shaders->setHashes[i])
				cachedDescriptorSets[i] = VK_NULL_HANDLE;
			else if (newShader->setHashes[i] == 0)
				cachedDescriptorSets[i] = VK_NULL_HANDLE;
		}
	}
	else
	{
		for (int i = 0; i < 4; i++)
			cachedDescriptorSets[i] = VK_NULL_HANDLE;
	}
	shaders = newShader;
}

ShaderModule* ShaderCache::get_shader(const std::string& path)
{
	auto it = module_cache.find(path);
	if (it == module_cache.end())
	{
		ShaderModule newShader;
		bool result = vkn::load_shader_module(_device, path.c_str(), &newShader);
		if (!result)
		{
			std::cout << "Error when compiling shader " << path << std::endl;
			return nullptr;
		}
		module_cache[path] = newShader;
	}
	return &module_cache[path];
}
