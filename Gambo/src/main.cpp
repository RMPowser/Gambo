#include "Frontend.h"
#include "SDL3\SDL_main.h"

int main(int argc, char* argv[])
{
	auto frontend = std::make_unique<Frontend>();
	frontend->Run();
	return 0;
}