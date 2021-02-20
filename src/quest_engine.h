#pragma once
#include <vk_types.h>

class QuestEngine
{
public:
	void init();
	void cleanup();
	void draw();
	void run();

public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };

	VkExtent2D _windowExtent{ 1700 , 900 };
	struct SDL_Window* _window{ nullptr };

};
