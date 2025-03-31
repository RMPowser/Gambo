#include "PPU.h"
#include "GamboDefine.h"
#include "GamboCore.h"
#include "CPU.h"
#include "RAM.h"
#include <random>

SDL_Color blankingColor = { 220, 220, 15, 255 };

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

bool PPU::RunFor(int cycles)
{
	static int scxExtraCycles = 0;
	static int objExtraCycles = 0;
	static bool firstEnterDrawMode = true;
	static bool firstEnterOAMScan = true;

	const u8& LCDC	= Get(HWAddr::LCDC);
	u8& STAT		= Get(HWAddr::STAT);

	bool vblank = false;

	cyclesCounter += cycles;

	//STAT bit 7 is always 1
	STAT |= 0b10000000;

	while (cycles > 0)
	{
		if (GetBits(LCDC, LCDCBits::LCDEnable))
		{
			switch (mode)
			{
				case PPUMode::HBlank:
				{
					if (cyclesCounter >= 204 - scxExtraCycles)
					{
						cyclesCounter -= 204;
						cycles = cyclesCounter;
						LY++;
						core->ram->Set(HWAddr::LY, LY);
						CheckForLYCStatInterrupt();


						if (LY == 144)
						{
							isBlankFrame = false;
							modeCounterForVBlank = cyclesCounter;

							mode = PPUMode::VBlank;
							core->ram->Set(HWAddr::STAT, (STAT & 0b11111100) | ((u8)mode & 0b11));

							vblank = true;
							core->cpu->RequestInterrupt(InterruptFlags::VBlank);

							if (GetBits(STAT, STATBits::Mode1StatInterruptEnable))
								core->cpu->RequestInterrupt(InterruptFlags::LCDStat);


							windowLY = 0;
							WYEqualsLYTriggered = false;
						}
						else
						{
							firstEnterOAMScan = true;

							mode = PPUMode::OAMScan;
							core->ram->Set(HWAddr::STAT, (STAT & 0b11111100) | ((u8)mode & 0b11));

							if (GetBits(STAT, STATBits::Mode2StatInterruptEnable))
								core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
						}
					}
					break;
				}
				case PPUMode::VBlank:
				{
					modeCounterForVBlank++;
					if (modeCounterForVBlank >= 456 && LY > 0)
					{
						modeCounterForVBlank -= 456;
						cycles = modeCounterForVBlank;
						LY++;
						core->ram->Set(HWAddr::LY, LY);
						CheckForLYCStatInterrupt();
					}

					if (cyclesCounter >= 4104 && LY >= 153)
					{
						LY = 0;
						core->ram->Set(HWAddr::LY, LY);
						CheckForLYCStatInterrupt();
					}

					if (cyclesCounter >= 4560)
					{
						cyclesCounter -= 4560;
						cycles = cyclesCounter;

						firstEnterOAMScan = true;
						mode = PPUMode::OAMScan;
						core->ram->Set(HWAddr::STAT, (STAT & 0b11111100) | ((u8)mode & 0b11));

						if (GetBits(STAT, STATBits::Mode2StatInterruptEnable))
							core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
					}
					break;
				}
				case PPUMode::OAMScan:
				{
					if (firstEnterOAMScan)
					{
						firstEnterOAMScan = false;
						if (!WYEqualsLYTriggered)
						{
							WYEqualsLYTriggered = Get(HWAddr::WY) == LY;
						}
					}

					if (cyclesCounter >= 80)
					{
						cyclesCounter -= 80;
						cycles = cyclesCounter;
						SCX = Get(HWAddr::SCX);

						mode = PPUMode::Draw;
						core->ram->Set(HWAddr::STAT, (STAT & 0b11111100) | ((u8)mode & 0b11));

						scanlineComplete = false;
						firstEnterDrawMode = true;

						// 8x8 or 8x16?
						objHeight = GetBits(LCDC, LCDCBits::OBJSize) ? 16 : 8;
						objs.clear();

						std::vector<OAM_entry> entries;

						for (u16 i = 0; i < OAMSize; i += sizeof(OAM_entry))
						{
							entries.push_back(reinterpret_cast<OAM_entry&>(Get(HWAddr::OAM + i)));
						}

						for (auto& entry : entries)
						{
							int objX = (int)entry.xpos - 8;
							int objY = (int)entry.ypos - 16;
							if (objY > LY 
								|| objY + objHeight <= LY
								|| objX < -7
								|| objX >= GamboScreenWidth)
							{
								continue;
							}

							objs.push_back(entry);

							// TODO: determine the extra cycles this object will take


							// only draw the first ten entries per scanline
							if (objs.size() >= 10)
								break;
						}

						// sort by lowest xpos and preserve ordering for ties
						std::stable_sort(objs.begin(), objs.end(),
							[](const OAM_entry& x, const OAM_entry& y)
							{
								return x.xpos < y.xpos;
							});
					}
					break;
				}
				case PPUMode::Draw:
				{
					// background scrolling costs SCX % 8 cycles at the start of drawing mode
					if (firstEnterDrawMode)
					{
						firstEnterDrawMode = false;
						scxExtraCycles = SCX % 8;
					}

					if ((cyclesCounter >= 12 + scxExtraCycles) && pixelCounter < GamboScreenWidth)
					{
						int drawCount = cycles;
						for (int i = 0; i < drawCount; i++)
						{
							DrawBGOrWindowPixel();
							DrawObjPixel();
							pixelCounter++;
							cycles--;
							if (pixelCounter >= GamboScreenWidth)
							{
								scanlineComplete = true;
								break;
							}
						}
						cycles++;
					}

					if (cyclesCounter >= 172 + scxExtraCycles)
					{
						pixelCounter = 0;
						cyclesCounter -= 172;
						cycles = cyclesCounter;

						mode = PPUMode::HBlank;
						core->ram->Set(HWAddr::STAT, (STAT & 0b11111100) | ((u8)mode & 0b11));

						if (GetBits(STAT, STATBits::Mode0StatInterruptEnable))
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
				cycles = cyclesCounter;
				vblank = true;
			}
		}

		cycles--;
	}

	return vblank;
}

void PPU::Reset()
{
	mode = PPUMode::HBlank;
	isBlankFrame = true;
	cyclesCounter = 0;
	modeCounterForVBlank = 0;
	pixelCounter = 0;
	scanlineComplete = false;
	LY = 0; 
	windowLY = 0;
	screen.fill(blankingColor);
	WYEqualsLYTriggered = false;

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

	if (GetBits(Get(HWAddr::STAT), STATBits::Mode2StatInterruptEnable))
		core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
}

void PPU::Disable()
{
	Reset();
}

PPUMode PPU::GetMode() const
{
	return mode;
}

void PPU::CheckForLYCStatInterrupt()
{
	if (GetBits(Get(HWAddr::LCDC), LCDCBits::LCDEnable))
	{
		u8& STAT = Get(HWAddr::STAT);

		if (LY == Get(HWAddr::LYC))
		{
			SetBit(STAT, STATBits::LYC_equals_LYFlag, true);
			if (GetBits(STAT, STATBits::LYC_equals_LYStatInterruptEnable))
				core->cpu->RequestInterrupt(InterruptFlags::LCDStat);
		}
		else
		{
			SetBit(STAT, STATBits::LYC_equals_LYFlag, false);
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
		if (isBlankFrame)
		{
			screen[pixelIndex] = blankingColor;
			return;
		}

		// check if the window is enabled. future behavior depends on this.
		bool usingWindow = GetBits(LCDC, (u8)LCDCBits::WindowEnable, 0b1) && WYEqualsLYTriggered && (pixelCounter + 7 >= WX);

		// figure out which tile map we're using according to the previous check.
		auto tileMapBitSelect = usingWindow ? LCDCBits::WindowTileMapArea : LCDCBits::BGTileMapArea;
		u16 tileMapAddr = GetBits(LCDC, (u8)tileMapBitSelect, 0x1) ? 0x9C00 : 0x9800;

		// figure out the base address for the tile data we need
		u16 tileDataBaseAddr = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? 0x8000 : 0x9000;
		bool isSigned = tileDataBaseAddr == 0x9000;

		// this is the x,y coordinates of the pixel in the 256x256pixel tile map. also update the top 5 bits of SCX here
		u8 pixelMapPosX = usingWindow ? (pixelCounter + 7 - WX) : pixelCounter + (SCX | (Get(HWAddr::SCX) & 0b11111000));
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
		screen[pixelIndex] = GameBoyColors[GetBits(BGP, 0, 0b11)];;
	}
}

void PPU::DrawObjPixel()
{
	const u8& LCDC = Get(HWAddr::LCDC);	// LCD control
	const u8& BGP = Get(HWAddr::BGP); // background palette
	const u8& OBP0 = Get(HWAddr::OBP0); // obj palette 0
	const u8& OBP1 = Get(HWAddr::OBP1); // obj palette 1

	const int pixelIndex = (LY * GamboScreenWidth) + pixelCounter;

	// if objects are enabled
	if (GetBits(LCDC, (u8)LCDCBits::OBJEnable, 0b1))
	{
		// early out if blankFrame
		if (isBlankFrame)
			return;

		// find the obj we need to draw at this pixel, if any
		for (const OAM_entry& obj : objs)
		{
			int pixelIndexToDrawWithinTileRow = pixelCounter - (obj.xpos - ObjWidth);
			if (pixelIndexToDrawWithinTileRow >= 0 && pixelIndexToDrawWithinTileRow < 8)
			{
				// gather flags
				const u8 palette = GetBits(obj.flags, OAM_entry::Flags::DMG_Palette) ? OBP1 : OBP0;
				const bool isXFlip = GetBits(obj.flags, OAM_entry::Flags::X_Flip);
				const bool isYFlip = GetBits(obj.flags, OAM_entry::Flags::Y_Flip);

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

					if (objHeight == 16)
					{
						isSecondTile = !isSecondTile;
					}
				}

				// this is the position of the pixel data within the tile data
				u8 tilePixelDataOffset = tileRow * 2; // each row takes up two bytes of memory

				// Bit 0 of tileindex for 8x16 objects should be ignored
				u8 tileIndex = objHeight == 16
					? ((obj.tileIndex & 0b11111110) + isSecondTile)
					: (obj.tileIndex + isSecondTile);

				// this is the address of the actual graphic data for the tile the obj is currently using
				u16 tileDataAddr = tileDataBaseAddr + (tileIndex * 16); // 16 bits per row of pixels within the tile

				// get the two bytes that hold the color data for this pixel
				u8 data0 = core->Read(tileDataAddr + tilePixelDataOffset);
				u8 data1 = core->Read(tileDataAddr + tilePixelDataOffset + 1);

				// pixel 0 in the tile is bit 7 of both data0 and data1. Pixel 1 is bit 6 of both. Pixel 2 is bit 5, etc...
				u8 colorBitIndex = 7 - pixelIndexToDrawWithinTileRow;

				// combine data0 and data1 to get the color id for this pixel
				bool colorBit0 = data0 & (1 << colorBitIndex);
				bool colorBit1 = data1  & (1 << colorBitIndex);
				u8 colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;
				
				// dont draw transparent pixels
				if (colorIndex != 0)
				{
					int& bgPixel = reinterpret_cast<int&>(screen[pixelIndex]);
					int& color0 = reinterpret_cast<int&>(GameBoyColors[GetBits(BGP, 0, 0b11)]);

					// early out if BG is over this pixel unless BG pixel is BGP color 0
					if (GetBits(obj.flags, OAM_entry::Flags::Priority) && bgPixel != color0)
					{
						return;
					}

					// now that we have the color id, get the actual color from the palette
					u8 color = GetBits(palette, colorIndex * 2, 0b11); // each color is a 2bit value

					// draw an actual color
					screen[pixelIndex] = GameBoyColors[color];

					return;
				}
			}
		}
	}
}