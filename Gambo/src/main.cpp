#include "Frontend.h"
#include "GamboCore.h"

int main(int argc, char* argv[])
{
	auto frontend = std::make_shared<Frontend>();


	std::thread frontendThread([frontend]() { frontend->Run(); });

	auto gamboCore = std::make_unique<GamboCore>(frontend);
	gamboCore->Run();

	frontendThread.join();
	frontend->CleanUp();
	return 0;
}