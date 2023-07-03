#include "Frontend.h"
#include "GamboCore.h"

int main(int argc, char* argv[])
{
	auto gamboCore = std::make_unique<GamboCore>(std::make_shared<Frontend>());
	gamboCore->Run();

	return 0;
}