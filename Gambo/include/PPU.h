#pragma once
#include "GamboDefine.h"

class Bus;

class PPU
{
public:
	enum class Mode
	{
		HBlank, // horizontal blank. 85-208 cycles depending on duration of previous mode 3
		VBlank, // verital blank. 4560 cycles.
		OAMScan, // oam scan for coords of sprites that overlap this line. 80 cycles
		Draw, // reading oam and vram to generate line. 168-291 depending on sprite count
	};

	Bus* bus = nullptr;
	bool DoDMATransfer = false;
	Mode mode = Mode::OAMScan;

	PPU(Bus* b);
	~PPU();

	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Clock(SDL_Texture* dmgScreen);
	bool FrameComplete();

private:
	 SDL_Color* screen = new SDL_Color[DMGScreenWidth * DMGScreenHeight];
	int cycles = 0;
};