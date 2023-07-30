#pragma once
#include "GamboDefine.h"

class RAM;

class VramViewer
{
public:
	VramViewer(RAM* c);
	~VramViewer();

	const std::array<SDL_Color, 256 * 256>& GetView();
	void SetTileMapBaseAddr(int addr);
	void SetTileDataBaseAddr(int addr);

	int GetTileMapBaseAddr();
	int GetTileDataBaseAddr();

private:
	u8 Read(u16 addr);

	int _tileMapBaseAddr = 0x9C00;
	int _tileDataBaseAddr = 0x8000;

	RAM* ram;
	std::array<SDL_Color, 256 * 256> bg0;
	std::array<SDL_Color, 256 * 256> bg1;
};