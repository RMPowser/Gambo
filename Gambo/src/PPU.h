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
	PPU(GamboCore* c);
	~PPU();

	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Tick();
	void Reset();
	bool FrameComplete() const;
	const std::array<SDL_Color, GamboScreenSize>& GetScreen() const;
	bool IsEnabled() const;
	PPUMode GetMode() const;
	void SetDoDMATransfer(bool b);

private:
	u8 GetBits(u8 reg, u8 bitIndex, u8 bitMask) const;

	GamboCore* core;
	PPUMode mode;
	bool doDMATransfer;
	int DMATransferCycles;
	int cycles;
	bool frameComplete;
	bool blankOnFirstFrame;
	bool isEnabled;
	bool windowEnabled;
	int currScanlineCycles;
	std::array<SDL_Color, GamboScreenSize> screen;
};