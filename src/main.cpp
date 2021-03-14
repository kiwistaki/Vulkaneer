#include "vulkaneer.h"

int main(int argc, char** argv)
{
	Vulkaneer engine;
	engine.init();
	engine.run();
	engine.cleanup();
	return 0;
}
