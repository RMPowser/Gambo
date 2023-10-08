#include "PPU.h"
#include "GamboDefine.h"
#include "GamboCore.h"
#include "CPU.h"
#include "RAM.h"
#include <random>

SDL_Color blankingColor = { 255, 255, 255, 255 };

PPU::PPU(GamboCore* c)
	: core(c)
{
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

u8& PPU::Get(u16 addr)
{
	return core->ram->Get(addr);
}

bool PPU::Tick(u8 cycles)
{
	const u8& LCDC	= Get(HWAddr::LCDC);
	const u8& DMA	= Get(HWAddr::DMA);
	u8& STAT		= Get(HWAddr::STAT);

	bool vblank = false;

	cyclesCounter += cycles;

	//STAT bit 7 is always 1
	STAT |= 0b10000000;

	if (doDMATransfer)
	{
		doDMATransfer = false;

		u16 startAddr = DMA << 8;
		for (u16 currAddr = startAddr; currAddr < startAddr + 160; currAddr++)
		{
			u8 data = Read(currAddr);
			Write(HWAddr::OAM + (currAddr - startAddr), data);
		}
	}

	if (isEnabled)
	{
		switch (mode)
		{
			case PPUMode::HBlank:
			{
				if (cyclesCounter >= 204)
				{
					cyclesCounter -= 204;
					mode = PPUMode::OAMScan;
					LY++;

					if (LY == 144)
					{
						blankFrame = false;
						mode = PPUMode::VBlank;
						modeCounterForVBlank = cyclesCounter;
						core->cpu->RequestInterrupt(InterruptFlags::VBlank);

						if (GetBits(STAT, (u8)STATBits::Mode1StatInterruptEnable, 0b1))
							core->cpu->RequestInterrupt(InterruptFlags::LCDStat);

						vblank = true;

						windowLY = 0;
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
					LY++;
				}

				if (cyclesCounter >= 4104 && LY >= 154)
				{
					LY = 0;
				}

				if (cyclesCounter >= 4560)
				{
					cyclesCounter -= 4560;
					mode = PPUMode::OAMScan;
					if (GetBits(STAT, (u8)STATBits::Mode2StatInterruptEnable, 0b1))
						core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
				}
				break;
			}
			case PPUMode::OAMScan:
			{
				if (cyclesCounter >= 80)
				{
					cyclesCounter -= 80;
					SCX = Get(HWAddr::SCX);
					mode = PPUMode::Draw;
					scanlineComplete = false;

					// 8x8 or 8x16?
					objHeight = GetBits(LCDC, (u8)LCDCBits::OBJSize, 0b1) ? 16 : 8;
					objsToDraw.clear();

					for (u16 i = 0; i < OAMSize; i += sizeof(OAM_entry))
					{
						auto entry = reinterpret_cast<OAM_entry&>(core->ram->Get(HWAddr::OAM + i));

						// do we want to draw this obj?
						int tileRow = LY - (entry.ypos - 16);
						if (tileRow >= 0 && tileRow < objHeight)
						{
							objsToDraw.push_back(entry);

							// only draw the first ten entries per scaline
							if (objsToDraw.size() >= 10)
								break;
						}
					}

					std::reverse(objsToDraw.begin(), objsToDraw.end());
				}
				break;
			}
			case PPUMode::Draw:
			{
				if (pixelCounter < GamboScreenWidth && LY <= GamboScreenHeight)
				{
					for (int i = 0; i < cycles; i++)
					{
						DrawBGOrWindowPixel();
						DrawObjPixel();
						pixelCounter++;
						if (pixelCounter >= GamboScreenWidth)
							break;
					}
				}

				if (cyclesCounter >= GamboScreenWidth && !scanlineComplete)
				{
					scanlineComplete = true;
				}

				if (cyclesCounter >= 172)
				{
					pixelCounter = 0;
					cyclesCounter -= 172;
					mode = PPUMode::HBlank;

					if (GetBits(STAT, (u8)STATBits::Mode0StatInterruptEnable, 0b1))
						core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
				}
				break;
			}
		}

	}
	else // lcd and ppu are disabled
	{
		if (cyclesCounter >= 70224) // cycles for a full screen
		{
			cyclesCounter -= 70224;
			vblank = true;
		}
	}

	// write mode to STAT and update LY
	Get(HWAddr::STAT) = (STAT & 0b11111100) | ((u8)mode & 0b11);
	Get(HWAddr::LY) = LY;
	CheckForLYCStatInterrupt();

	return vblank;
}

void PPU::Reset()
{
	mode = PPUMode::VBlank;
	doDMATransfer = false;
	blankFrame = true;
	isEnabled = false;
	cyclesCounter = 0;
	modeCounterForVBlank = 0;
	pixelCounter = 0;
	scanlineComplete = false;
	LY = 0; 
	windowLY = 0;
	screen.fill(blankingColor);

	Get(HWAddr::LY) = LY;
	Get(HWAddr::STAT) = (Get(HWAddr::STAT) & 0b11111100) | ((u8)mode & 0b11);
}

const std::array<SDL_Color, GamboScreenSize>& PPU::GetScreen() const
{
	return screen;
}

void PPU::Enable()
{
	Reset();
	isEnabled = true;

	if (GetBits(Get(HWAddr::STAT), (u8)STATBits::Mode2StatInterruptEnable, 0b1))
		core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
}

void PPU::Disable()
{
	Reset();
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

void PPU::CheckForLYCStatInterrupt()
{
	if (isEnabled)
	{
		u8& STAT = Get(HWAddr::STAT);

		if (LY == Get(HWAddr::LYC))
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

void PPU::DrawBGOrWindowPixel()
{
	const u8& LCDC = Get(HWAddr::LCDC);	// LCD control
	const u8& SCY = Get(HWAddr::SCY);	// scroll y
	const u8& WX = Get(HWAddr::WX);		// window X position + 7
	const u8& WY = Get(HWAddr::WY);		// window Y position
	const u8& BGP = Get(HWAddr::BGP);	// BG palette data

	const int pixelIndex = (LY * GamboScreenWidth) + pixelCounter;

	// if background and window are enabled
	if (GetBits(LCDC, (u8)LCDCBits::BGAndWindowEnable, 0b1))
	{
		// early out if blankFrame
		if (blankFrame)
		{
			screen[pixelIndex] = blankingColor;
			return;
		}

		// check if the window is enabled. future behavior depends on this.
		bool usingWindow = (GetBits(LCDC, (u8)LCDCBits::WindowEnable, 0b1) && WY <= LY);

		// figure out which tile map we're using according to the previous check.
		auto tileMapBitSelect = usingWindow ? LCDCBits::WindowTileMapArea : LCDCBits::BGTileMapArea;
		u16 tileMapAddr = GetBits(LCDC, (u8)tileMapBitSelect, 0x1) ? 0x9C00 : 0x9800;

		// figure out the base address for the tile data we need
		u16 tileDataBaseAddr = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1)	? 0x8000 : 0x9000;
		bool isSigned = tileDataBaseAddr == 0x9000;

		// this is the x,y coordinates of the pixel in the 256x256pixel tile map. also update the top 5 bits of SCX here
		u8 pixelMapPosX = (usingWindow && pixelCounter >= WX - 7) ? pixelCounter : pixelCounter + (SCX | (Get(HWAddr::SCX) & 0b11111000));
		u8 pixelMapPosY = usingWindow ? (windowLY) : (LY + SCY);
		
		// this is the x,y indices of the tile within the map
		u8 tileX = pixelMapPosX / 8;
		u8 tileY = pixelMapPosY / 8;

		// get the tile id number from the tile map. Remember it can be signed or unsigned depending on the tile data base address
		u16 tileIdAddr = tileMapAddr + ((tileY * 32) + tileX); // there are 32 rows of tiles in the tile map
		s16 tileId = isSigned ? (s8)core->Read(tileIdAddr) : core->Read(tileIdAddr);

		// this is the address of the actual graphic data for the tile
		u16 tileDataAddr = tileDataBaseAddr + (tileId * 16); // 16 bits per row of pixels within the tile

		// this is the position of the pixel data within the tile data
		u8 tilePixelDataOffset = (pixelMapPosY % 8) * 2; // each row takes up two bytes of memory

		// get the two bytes that hold the color data for this pixel
		u8 data0 = core->Read(tileDataAddr + tilePixelDataOffset);
		u8 data1 = core->Read(tileDataAddr + tilePixelDataOffset + 1);

		// pixel 0 in the tile is bit 7 of both data0 and data1. Pixel 1 is bit 6 of both. Pixel 2 is bit 5, etc...
		u8 colorBitIndex = 7 - (pixelMapPosX % 8);

		// combine data0 and data1 to get the color id for this pixel
		bool colorBit0 = data0 & (1 << colorBitIndex);
		bool colorBit1 = data1 & (1 << colorBitIndex);
		u8 colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;

		// now that we have the color id, get the actual color from the BG palette reg 0xFF47
		u8 color = GetBits(BGP, colorIndex * 2, 0b11); // each color is a 2bit value

		// we can finally draw a pixel
		screen[pixelIndex] = GameBoyColors[color];

		if (usingWindow && pixelCounter >= GamboScreenWidth - 1)
			windowLY++;
	}
	else
	{
		// if the screen is off, just draw a white pixel
		screen[pixelIndex] = blankingColor;
	}
}

void PPU::DrawObjPixel()
{
	const u8& LCDC = Get(HWAddr::LCDC);	// LCD control
	const u8& OBP0 = Get(HWAddr::OBP0); // obj palette 0
	const u8& OBP1 = Get(HWAddr::OBP1); // obj palette 1

	const int pixelIndex = (LY * GamboScreenWidth) + pixelCounter;

	// if background and window are enabled
	if (GetBits(LCDC, (u8)LCDCBits::OBJEnable, 0b1))
	{
		// early out if blankFrame
		if (blankFrame)
			return;

		// find the obj we need to draw at this pixel, if any
		for (auto& obj : objsToDraw)
		{
			s8 pixelIndexToDrawWithinTileRow = pixelCounter - (obj.xpos - ObjWidth);
			if (pixelIndexToDrawWithinTileRow >= 0 && pixelIndexToDrawWithinTileRow < 8)
			{
				// early out if BG is over Obj
				if (GetBits(obj.flags, 7, 0b1) && (
					*reinterpret_cast<u32*>(&screen[pixelIndex]) == *reinterpret_cast<u32*>(&GameBoyColors[1]) ||
					*reinterpret_cast<u32*>(&screen[pixelIndex]) == *reinterpret_cast<u32*>(&GameBoyColors[2]) ||
					*reinterpret_cast<u32*>(&screen[pixelIndex]) == *reinterpret_cast<u32*>(&GameBoyColors[3])))
				{
					continue;
				}

				// gather flags
				const u8 palette = GetBits(obj.flags, 4, 0b1) ? OBP1 : OBP0;
				const bool isXFlip = GetBits(obj.flags, 5, 0b1);
				const bool isYFlip = GetBits(obj.flags, 6, 0b1);

				// base address for obj tile data is always 0x8000
				u16 tileDataBaseAddr = 0x8000;

				// this is the row within the tile we want to draw
				u8 tileRow = LY - (obj.ypos - 16);

				// check if the row is part of the second tile if 8x16 is enabled
				bool isSecondTile = objHeight == 16 && tileRow >= 8;

				// adjust the row to be within the bounds of one tile according to the previous check
				if (isSecondTile)
					tileRow -= 8;
					
				// check if tile data should be interpreted as flipped
				if (isXFlip)
					pixelIndexToDrawWithinTileRow = 7 - pixelIndexToDrawWithinTileRow;
				if (isYFlip)
				{
					tileRow = 7 - tileRow; 
					isSecondTile = !isSecondTile;
				}

				// this is the position of the pixel data within the tile data
				u8 tilePixelDataOffset = tileRow * 2; // each row takes up two bytes of memory

				// this is the address of the actual graphic data for the tile the obj is currently using
				u16 tileDataAddr = tileDataBaseAddr + ((obj.tileIndex + isSecondTile) * 16); // 16 bits per row of pixels within the tile

				// get the two bytes that hold the color data for this pixel
				u8 data0 = core->Read(tileDataAddr + tilePixelDataOffset);
				u8 data1 = core->Read(tileDataAddr + tilePixelDataOffset + 1);

				// pixel 0 in the tile is bit 7 of both data0 and data1. Pixel 1 is bit 6 of both. Pixel 2 is bit 5, etc...
				u8 colorBitIndex = 7 - pixelIndexToDrawWithinTileRow;

				// combine data0 and data1 to get the color id for this pixel
				bool colorBit0 = data0 & (1 << colorBitIndex);
				bool colorBit1 = data1 & (1 << colorBitIndex);
				u8 colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;

				// if the colorIndex is 0, just use whatever is in the bg already 
				if (colorIndex == 0)
					continue;

				// now that we have the color id, get the actual color from the BG palette reg 0xFF47
				u8 color = GetBits(palette, colorIndex * 2, 0b11); // each color is a 2bit value

				// draw an actual color
				screen[pixelIndex] = GameBoyColors[color];
			}
		}
	}
}