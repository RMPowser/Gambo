#include "Frontend.h"
#include "GamboCore.h"

int main(int argc, char* argv[])
{
	Frontend frontend;
	GamboCore emu(&frontend);
	emu.Run();

	return 0;
}