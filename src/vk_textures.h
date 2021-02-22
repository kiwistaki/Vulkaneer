#pragma once
#include "vk_types.h"
#include "quest_engine.h"

namespace Quest
{
	bool load_image_from_file(QuestEngine& engine, const char* file, AllocatedImage& outImage);
}
