#include "PPU.h"
#include "GamboDefine.h"
#include "GamboCore.h"
#include "CPU.h"
#include <random>


PPU::PPU(GamboCore* c)
	: core(c)
{
	Reset();
}

PPU::~PPU()
{
}

u8 PPU::Read(u16 addr)
{
	return core->Read(addr);
}

void PPU::Write(u16 addr, u8 data)
{
	core->Write(addr, data);
}

bool PPU::Tick(u8 cycles)
{
	const u8& LCDC = core->Read(HWAddr::LCDC);
	const u8& DMA	= core->ram[HWAddr::DMA];
	u8& STAT		= core->ram[HWAddr::STAT];

	bool vblank = false;

	modeCounter += cycles;

	//STAT bit 7 is always 1
	STAT |= 0b10000000;

	if (doDMATransfer)
	{
		doDMATransfer = false;

		u16 startAddr = DMA << 8;
		u8 data;
		for (u16 currAddr = startAddr; currAddr < startAddr + 160; currAddr++)
		{
			data = core->Read(currAddr);
			Write(HWAddr::OAM + (currAddr - startAddr), data);
		}
	}

	if (isEnabled)
	{
		switch (mode)
		{
			case PPUMode::HBlank:
			{
				if (modeCounter >= 204)
				{
					modeCounter -= 204;
					mode = PPUMode::OAMScan;
					LY++;

					if (LY == 144)
					{
						blankFrame = false;
						mode = PPUMode::VBlank;
						lineNumberDuringVBlank = 0;
						modeCounterForVBlank = modeCounter;
						core->cpu->RequestInterrupt(InterruptFlags::VBlank);

						if (GetBits(STAT, (u8)STATBits::Mode1StatInterruptEnable, 0b1))
							core->cpu->RequestInterrupt(InterruptFlags::LCDStat);

						vblank = true;

						windowLine = 0;
					}
					else
					{
						if (GetBits(STAT, (u8)STATBits::Mode2StatInterruptEnable, 0b1))
							core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
					}
				}
				break;
			}
			case PPUMode::VBlank:
			{
				modeCounterForVBlank += cycles;
				if (modeCounterForVBlank >= 456)
				{
					modeCounterForVBlank -= 456;
					lineNumberDuringVBlank++;

					if (lineNumberDuringVBlank <= 9)
						LY++;
				}

				if (modeCounter >= 4104 && modeCounterForVBlank >= 4 && LY == 153)
				{
					LY = 0;
				}

				if (modeCounter >= 4560)
				{
					modeCounter -= 4560;
					mode = PPUMode::OAMScan;
					if (GetBits(STAT, (u8)STATBits::Mode2StatInterruptEnable, 0b1))
						core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
				}
				break;
			}
			case PPUMode::OAMScan:
			{
				if (modeCounter >= 80)
				{
					modeCounter -= 80;
					mode = PPUMode::Draw;
					scanlineComplete = false;
				}
				break;
			}
			case PPUMode::Draw:
			{
				if (pixelCounter < GamboScreenWidth)
				{
					tileCycleCounter += cycles;
					
					if (isEnabled && GetBits(LCDC, (u8)LCDCBits::LCDEnable, 0b1))
					{
						while (tileCycleCounter >= 3)
						{
							DrawBG();
							pixelCounter += 4;
							tileCycleCounter -= 3;

							if (pixelCounter >= GamboScreenWidth)
							{
								break;
							}
						}
					}
				}

				if (modeCounter >= 160 && !scanlineComplete)
				{
					DrawSL();
					scanlineComplete = true;
				}

				if (modeCounter >= 172)
				{
					pixelCounter = 0;
					modeCounter -= 172;
					mode = PPUMode::HBlank;
					tileCycleCounter = 0;

					if (GetBits(STAT, (u8)STATBits::Mode0StatInterruptEnable, 0b1))
						core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
				}
				break;
			}
		}

		// write mode to STAT and update LY
		core->ram[HWAddr::STAT] = (STAT & 0b11111100) | ((u8)mode & 0b11);
		core->ram[HWAddr::LY] = LY;
		CheckForLYCStatInterrupt();
	}
	else // lcd and ppu are disabled
	{
		if (screenEnableDelayCycles > 0)
		{
			if ((screenEnableDelayCycles -= cycles) <= 0)
			{
				screenEnableDelayCycles = 0;
				isEnabled = true;
				blankFrame = true;
				mode = PPUMode::HBlank;
				modeCounter = 0;
				modeCounterForVBlank = 0;
				LY = 0;
				windowLine = 0;
				lineNumberDuringVBlank = 0;
				pixelCounter = 0;
				tileCycleCounter = 0;
				
				core->ram[HWAddr::LY] = LY;

				if (GetBits(STAT, (u8)STATBits::Mode2StatInterruptEnable, 0b1))
					core->cpu->RequestInterrupt(InterruptFlags::LCDStat);

				CheckForLYCStatInterrupt();
			}
		}
		else if (modeCounter >= 70224) // cycles for a full screen
		{
			modeCounter -= 70224;
			vblank = true;
		}
	}

	return vblank;
}

void PPU::Reset()
{
	mode = PPUMode::VBlank;
	doDMATransfer = false;
	blankFrame = true;
	isEnabled = true;
	modeCounter = 0;
	modeCounterForVBlank = 0;
	lineNumberDuringVBlank = 0;
	windowLine = 0;
	pixelCounter = 0;
	tileCycleCounter = 0;
	scanlineComplete = false;
	LY = 0; 
	screenEnableDelayCycles = 244;
	screen.fill({ 0, 0, 0, 255 });
}

const std::array<SDL_Color, GamboScreenSize>& PPU::GetScreen() const
{
	return screen;
}

void PPU::Enable()
{
	if (!isEnabled)
	{
		screenEnableDelayCycles = 244;
	}
}

void PPU::Disable()
{
	isEnabled = false;
	
	LY = 0;
	core->ram[HWAddr::LY] = LY;

	mode = PPUMode::HBlank;
	core->ram[HWAddr::STAT] = (core->ram[HWAddr::STAT] & 0b11111100) | ((u8)mode & 0b11);

	modeCounter = 0;
	modeCounterForVBlank = 0;
	blankFrame = true;
}

bool PPU::IsEnabled() const
{
	return isEnabled;
}

void PPU::ResetWindowLine()
{
	if (windowLine == 0 && LY < 144 && LY > core->ram[HWAddr::WY])
		windowLine = 144;
}

PPUMode PPU::GetMode() const
{
	return mode;
}

void PPU::SetDoDMATransfer(bool b)
{
	doDMATransfer = true;
}

void PPU::CheckForLYCStatInterrupt()
{
	if (isEnabled)
	{
		const u8& LYC = core->ram[HWAddr::LYC];
		u8& STAT = core->ram[HWAddr::STAT];

		if (LY == LYC)
		{
			STAT |= (1 << (u8)STATBits::LYC_equals_LYFlag);
			if (GetBits(STAT, (u8)STATBits::LYC_equals_LYStatInterruptEnable, 0b1))
				core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
		}
		else
		{
			STAT &= ~(1 << (u8)STATBits::LYC_equals_LYFlag);
		}
	}
}

void PPU::DrawBG()
{
	const u8& LCDC = core->ram[HWAddr::LCDC];
	const u8& SCY = core->ram[HWAddr::SCY];	// viewport y position
	const u8& SCX = core->ram[HWAddr::SCX];	// viewport x position
	const u8& BGP = core->ram[HWAddr::BGP];	// BG pallette data
	const u8& WY = core->ram[HWAddr::WY];	// window Y position
	const u8& WX = core->ram[HWAddr::WX];	// window X position + 7

	const int lineWidth = (LY * GamboScreenWidth);

	if (GetBits(LCDC, (u8)LCDCBits::BGAndWindowEnable, 0b1))
	{
		int pixelsToDraw = 4;

		bool usingWindow = (GetBits(LCDC, (u8)LCDCBits::WindowEnable, 0b1) && WY <= LY);

		auto tileMapBitSelect = usingWindow ? LCDCBits::WindowTileMapArea : LCDCBits::BGTileMapArea;
		int bgDataAddr = GetBits(LCDC, (u8)tileMapBitSelect, 0x1) ? 0x9C00 : 0x9800;

		u16 tileDataBaseAddr = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? 0x8000 : 0x9000;
		bool isSigned = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? false : true;

		u8 pixelY = usingWindow ? (LY + WY) : (LY + SCY);
		u8 tileRow = pixelY / 8;

		// which of the 8 vertical pixels of the current tile is the scanline on?
		u8 tilePixelRow = (pixelY % 8) * 2; // each row takes up two bytes of memory

		// time to start drawing the pixels
		int startingPixel = pixelCounter;
		for (int pixel = startingPixel; pixel < startingPixel + pixelsToDraw; pixel++)
		{
			u8 xPos;
			if (usingWindow && pixel >= WX)
				xPos = pixel - WX;
			else
				xPos = pixel + SCX;

			u16 tileColumn = (xPos / 8);

			// get the tile id number. Remember it can be signed or unsigned
			u16 tileIdAddr = bgDataAddr + (tileRow * 32) + tileColumn; // there are 32 rows of tiles in memory
			int tileId = isSigned ? (s8)core->Read(tileIdAddr) : core->Read(tileIdAddr);

			u16 tileDataAddr = tileDataBaseAddr + (tileId * 16); // 16 bits per row of pixels within the tile

			// get the two bytes that hold the color data for this pixel
			u8 data0 = core->Read(tileDataAddr + tilePixelRow);
			u8 data1 = core->Read(tileDataAddr + tilePixelRow + 1);

			// pixel 0 in the tile is bit 7 of both data0 and data1. Pixel 1 is bit 6 etc..
			u8 colorBitIndex = 7 - (xPos % 8);

			// combine data0 and data1 to get the color id for this pixel in the tile
			bool colorBit0 = data0 & (1 << colorBitIndex);
			bool colorBit1 = data1 & (1 << colorBitIndex);
			u8 colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;

			// now we have the color id, get the actual color from BG palette 0xFF47
			u8 color = GetBits(BGP, colorIndex * 2, 0b11);

			// we can finally draw a pixel
			int index = lineWidth + pixel;
			screen[index] = blankFrame ? SDL_Color{ 255, 255, 255, 255 } : GameBoyColors[color];
		}
	}
	else
	{
		for (int x = 0; x < 4; x++)
		{
			int index = lineWidth + pixelCounter + x;
			screen[index] = SDL_Color{ 255, 255, 255, 255 };
		}
	}
}

void PPU::DrawSL()
{

}