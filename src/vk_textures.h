#pragma once
#include "vk_types.h"
#include "vulkaneer.h"

namespace vkn
{
	bool load_image_from_file(Vulkaneer& engine, const char* file, AllocatedImage& outImage);
}
