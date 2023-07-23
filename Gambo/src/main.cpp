#include "Frontend.h"

int main(int argc, char* argv[])
{
	auto frontend = std::make_unique<Frontend>();
	frontend->Run();
	return 0;
}