#include "Gambo.h"

int main(int argc, char* argv[])
{
	auto emu = std::make_unique<Gambo>();
	emu->Run();

	return 0;
}