#include "quest_engine.h"
#include "vk_types.h"
#include "vk_initializers.h"

#include <SDL.h>
#include <SDL_vulkan.h>

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

	//everything went fine
	_isInitialized = true;
}

void QuestEngine::cleanup()
{
	if (_isInitialized) {
		SDL_DestroyWindow(_window);
	}
}

void QuestEngine::draw()
{
	//nothing yet
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
