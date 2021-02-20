#include "quest_engine.h"

int main(int argc, char** argv)
{
	QuestEngine engine;
	engine.init();
	engine.run();
	engine.cleanup();
	return 0;
}
