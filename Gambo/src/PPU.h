#pragma once
#include "GamboDefine.h"

class GamboCore;

enum class PPUMode
{
	HBlank, // horizontal blank. 85-208 cycles depending on duration of previous mode 3
	VBlank, // verital blank. 4560 cycles.
	OAMScan, // oam scan for coords of sprites that overlap this line. 80 cycles
	Draw, // reading oam and vram to generate line. 168-291 depending on sprite count
};

class PPU
{
public:
	bool DoDMATransfer = false;
	PPUMode mode = PPUMode::OAMScan;

	PPU(GamboCore* c);
	~PPU();

	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Clock();
	void Reset();
	bool FrameComplete();
	SDL_Color* screen = new SDL_Color[GamboScreenSize];

private:
	GamboCore* core = nullptr;
	int cycles = 0;
	bool frameComplete = false;
	bool blankOnFirstFrame = false;
};