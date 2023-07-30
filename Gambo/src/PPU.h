#pragma once
#include "GamboDefine.h"

class GamboCore;

enum class LCDCBits
{
	BGAndWindowEnable = 0,
	OBJEnable = 1,
	OBJSize = 2,
	BGTileMapArea = 3,
	TileDataArea = 4,
	WindowEnable = 5,
	WindowTileMapArea = 6,
	LCDEnable = 7,
};

enum class OBPBits
{
	OBPColorForIndex1 = 0,
	OBPColorForIndex2 = 2,
	OBPColorForIndex3 = 4,
};

enum class STATBits
{
	modeFlag = 0,
	LYC_equals_LYFlag = 2,
	Mode0StatInterruptEnable = 3,
	Mode1StatInterruptEnable = 4,
	Mode2StatInterruptEnable = 5,
	LYC_equals_LYStatInterruptEnable = 6,
	alwaysSet = 7,
};

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

	bool Tick(u8 cycles);
	void Reset();
	const std::array<SDL_Color, GamboScreenSize>& GetScreen() const;
	void Enable();
	void Disable();
	bool IsEnabled() const;
	PPUMode GetMode() const;
	void SetDoDMATransfer(bool b);

private:
	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	u8& Get(u16 addr);

	void CheckForLYCStatInterrupt();
	void DrawBGOrWindowPixel(); // draw background
	void DrawSL(); // draw scanline

	GamboCore* core;
	PPUMode mode;
	bool doDMATransfer;
	int blankFrame;
	bool isEnabled;
	int cyclesCounter;
	int modeCounterForVBlank;
	int pixelCounter;				// keeps track of the pixel on the current scanline. resets every scanline.
	bool scanlineComplete;
	int LY;							// this is read only which is why we keep a local copy and write it into ram
	int windowLY;					// same as LY but for the window. internal only, meaning not accessible to any other components of the game boy.
	std::array<SDL_Color, GamboScreenSize> screen;
};