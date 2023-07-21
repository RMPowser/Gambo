#pragma once
#include "GamboDefine.h"

class RAM;

class VramViewer
{
public:
	VramViewer(RAM* c);
	~VramViewer();

	const std::array<SDL_Color, 256 * 256>& GetView(u16 baseAddr);

private:
	u8 Read(u16 addr);

	RAM* ram;
	std::array<SDL_Color, 256 * 256> bg0;
	std::array<SDL_Color, 256 * 256> bg1;
};