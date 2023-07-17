#include "PPU.h"
#include "GamboDefine.h"
#include "GamboCore.h"
#include <random>

enum class LCDCBits
{
	BGAndWindowEnable			= 0,
	OBJEnable					= 1,
	OBJSize						= 2,
	BGTileMapArea				= 3,
	TileDataArea				= 4,
	WindowEnable				= 5,
	WindowTileMapArea			= 6,
	LCDEnable					= 7,
};

enum class BGPBits
{
	BGColorForIndex0 = 0,
	BGColorForIndex1 = 2,
	BGColorForIndex2 = 4,
	BGColorForIndex3 = 6,
};

enum class OBPBits
{
	OBPColorForIndex1 = 0,
	OBPColorForIndex2 = 2,
	OBPColorForIndex3 = 4,
};

enum class STATBits
{
	modeFlag							= 0,
	LYC_equals_LYFlag					= 2,
	Mode0StatInterruptEnable			= 3,
	Mode1StatInterruptEnable			= 4,
	Mode2StatInterruptEnable			= 5,
	LYC_equals_LYStatInterruptEnable	= 6,
	alwaysSet							= 7,
};

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

void PPU::Tick()
{
	u8 LCDC = core->Read(HWAddr::LCDC);
	isEnabled = GetBits(LCDC, (u8)LCDCBits::LCDEnable, 0b1);
	windowEnabled = GetBits(LCDC, (u8)LCDCBits::WindowEnable, 0b1);

	static int callCount = 0;

	const u8 SCY	= core->Read(HWAddr::SCY);	// viewport y position
	const u8 SCX	= core->Read(HWAddr::SCX);	// viewport x position
	const u8 LYC	= core->Read(HWAddr::LYC);	// LY compare
	const u8 WY		= core->Read(HWAddr::WY);	// window Y position
	const u8 WX		= core->Read(HWAddr::WX);	// window X position + 7
	const u8 BGP	= core->Read(HWAddr::BGP);	// BG pallette data
	const u8 OBP0	= core->Read(HWAddr::OBP0);	// OBJ pallette data
	const u8 OBP1	= core->Read(HWAddr::OBP1);	// OBJ pallette data
	const u8 DMA	= core->Read(HWAddr::DMA);
	u8 LY			= core->Read(HWAddr::LY);	// LCD Y coordinate
	u8 STAT			= core->Read(HWAddr::STAT);
	u8 IF			= core->Read(HWAddr::IF);

	//STAT bit 7 is always 1
	STAT |= 0b10000000;

	if (doDMATransfer && DMATransferCycles == 0)
	{
		DMATransferCycles = 160;
	}

	if (DMATransferCycles > 0)
	{
		DMATransferCycles--;

		if (DMATransferCycles == 0)
		{
			doDMATransfer = false;

			static u16 startAddr;
			startAddr = DMA << 8;

			for (u16 currAddr = startAddr; currAddr < startAddr + 160; currAddr++)
			{
				static u8 data;
				data = core->Read(currAddr);
				Write(HWAddr::OAM + (currAddr - startAddr), data);
			}
		}
	}

	if (isEnabled)
	{
		if (LY >= 144)
		{
			if (mode != PPUMode::VBlank)
			{
				mode = PPUMode::VBlank;
				if (GetBits(STAT, (u8)STATBits::Mode1StatInterruptEnable, 0b1))
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
		}
		else if (currScanlineCycles < 80)
		{
			if (mode != PPUMode::OAMScan)
			{
				mode = PPUMode::OAMScan;
				if (GetBits(STAT, (u8)STATBits::Mode2StatInterruptEnable, 0b1))
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
		}
		else if (currScanlineCycles < 172) // 289
		{
			mode = PPUMode::Draw;
		}
		else if (currScanlineCycles < 456)
		{
			if (mode != PPUMode::HBlank)
			{
				mode = PPUMode::HBlank;
				if (GetBits(STAT, (u8)STATBits::Mode0StatInterruptEnable, 0b1))
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
		}

		// clear and then set new mode
		STAT &= ~0b00000011;
		STAT |= (u8)mode;

		switch (mode)
		{
			case PPUMode::HBlank:
				break;

			case PPUMode::VBlank:
				break;

			case PPUMode::OAMScan:
				break;

			case PPUMode::Draw:
				if (currScanlineCycles == 80)
				{
					// are we using the window?
					bool usingWindow = (GetBits(LCDC, (u8)LCDCBits::WindowEnable, 0b1) && WY <= LY) ? true : false;

					// which bg map?
					u16 BGAddr;
					if (usingWindow)
					{
						BGAddr = GetBits(LCDC, (u8)LCDCBits::WindowTileMapArea, 0b1) ? 0x9C00 : 0x9800;
					}
					else
					{
						BGAddr = GetBits(LCDC, (u8)LCDCBits::BGTileMapArea, 0b1) ? 0x9C00 : 0x9800;
					}

					// which tile data set are we using?
					u16 tileDataBaseAddr = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? 0x8000 : 0x9000;
					bool isSigned = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? false : true;

					// calculate the tile row on the screen
					u8 tileRowOffset;
					if (usingWindow)
					{
						tileRowOffset = (u8)(LY + WY) / (u8)8;
					}
					else
					{
						tileRowOffset = (u8)(SCY + LY) / (u8)8;
					}

					// which of the 8 vertical pixels of the current tile is the scanline on?
					u8 pixelRowOffset = (LY % 8) * 2; // each row takes up two bytes of memory

					// time to start drawing the scanline
					u8 xPos;
					for (int pixel = 0; pixel < GamboScreenWidth; pixel++)
					{
						if (usingWindow && pixel >= WX)
						{
							xPos = pixel - WX;
						}
						else
						{
							xPos = pixel + SCX;
						}

						// calculate the tile column
						u16 tileColOffset = (xPos / 8);

						// get the tile id number. Remember it can be signed or unsigned
						u16 tileIdAddr = BGAddr + (tileRowOffset * 32) + tileColOffset; // there are 32 rows of tiles in memory
						int tileId = isSigned ? (s8)core->Read(tileIdAddr) : core->Read(tileIdAddr);

						// calculate the address of the tile data
						u16 tileDataAddr = tileDataBaseAddr + (tileId * 16); // 16 bits per row of pixels within the tile

						// calculate the vertical position within the tile data
						u8 data0 = core->Read(tileDataAddr + pixelRowOffset);
						u8 data1 = core->Read(tileDataAddr + pixelRowOffset + 1);

						// pixel 0 in the tile is bit 7 of both data1 and data2. Pixel 1 is bit 6 etc..
						u8 colorBitIndex = 7 - (xPos % 8);

						// combine data2 and data1 to get the color id for this pixel in the tile
						bool colorBit0 = data0 & (1 << colorBitIndex);
						bool colorBit1 = data1 & (1 << colorBitIndex);
						u8 colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;

						// now we have the color id, get the actual color from BG palette 0xFF47
						u8 color;
						switch (colorIndex)
						{
							case 0:
								color = GetBits(BGP, (u8)BGPBits::BGColorForIndex0, 0b11);
								break;

							case 1:
								color = GetBits(BGP, (u8)BGPBits::BGColorForIndex1, 0b11);
								break;

							case 2:
								color = GetBits(BGP, (u8)BGPBits::BGColorForIndex2, 0b11);
								break;

							case 3:
								color = GetBits(BGP, (u8)BGPBits::BGColorForIndex3, 0b11);
								break;

							default:

								break;
						}

						// we can finally draw a pixel
						size_t index = ((size_t)LY * GamboScreenWidth) + pixel;
						screen[index] = blankOnFirstFrame ? GameBoyColors[White] : GameBoyColors[color];
					}
				}
				break;
			default:
				throw;
		}

		currScanlineCycles = (currScanlineCycles + 1) % 456;
		if (currScanlineCycles == 0)
		{
			LY = (LY + 1) % 154;
			if (LY == 144)
			{
				IF |= InterruptFlags::VBlank; // request vblank interrupt
			}

			if (LY == LYC)
			{
				STAT |= 0x0000100;
				if (GetBits(STAT, (u8)STATBits::LYC_equals_LYStatInterruptEnable, 0b1))
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
			else
			{
				STAT &= ~0x0000100;
			}
		}

		cycles = (cycles + 1) % 70224;

		if (cycles == 0)
		{
			frameComplete = true;
			blankOnFirstFrame = false;
		}
	}
	else
	{
		// disable all of this since the lcd is disabled
		blankOnFirstFrame = true;
		cycles = 0;
		currScanlineCycles = 0;
		LY = 0;
		STAT &= ~0b01111111; 
	}

	callCount = (callCount + 1) % 70224;
	if (callCount == 0)
	{
		frameComplete = true;
	}

	Write(HWAddr::LY, LY);
	Write(HWAddr::STAT, STAT);
	Write(HWAddr::IF, IF);
}

void PPU::Reset()
{
	mode = PPUMode::VBlank;
	doDMATransfer = false;
	DMATransferCycles = 0;
	cycles = 0;
	frameComplete = false;
	blankOnFirstFrame = false;
	isEnabled = true;
	windowEnabled = false;
	currScanlineCycles = 0;
	screen.fill({ 0, 0, 0, 255 });
}

bool PPU::FrameComplete() const
{
	return frameComplete;
}

const std::array<SDL_Color, GamboScreenSize>& PPU::GetScreen() const
{
	return screen;
}

bool PPU::IsEnabled() const
{
	return isEnabled;
}

PPUMode PPU::GetMode() const
{
	return mode;
}

void PPU::SetDoDMATransfer(bool b)
{
	doDMATransfer = true;
}

u8 PPU::GetBits(u8 reg, u8 bitIndex, u8 bitMask) const
{
	return (reg & (bitMask << bitIndex)) >> bitIndex;
}
