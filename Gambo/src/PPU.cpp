#include "SDL2/SDL.h"
#include "PPU.h"
#include "Bus.h"
#include <random>


PPU::PPU(Bus* b)
	:bus(b)
{
}

PPU::~PPU()
{
	SAFE_DELETE_ARRAY(screen);
}

u8 PPU::Read(u16 addr)
{
	return bus->Read(addr);
}

void PPU::Write(u16 addr, u8 data)
{
	bus->Write(addr, data);
}

void PPU::Clock(SDL_Texture* dmgScreen)
{
	frameComplete = false;
	static int currScanlineCycles = 0;

	static u8& LCDC = bus->ram[HWAddr::LCDC];
	static bool LCDEnable;					// 0=Off, 1=On
	static bool WindowTileMapArea;			// 0=9800-9BFF, 1=9C00-9FFF
	static bool WindowEnable;				// 0=Off, 1=On
	static bool BGAndWindowTileDataArea;	// 0=8800-97FF, 1=8000-8FFF
	static bool BGTileMapArea;				// 0=9800-9BFF, 1=9C00-9FFF
	static bool OBJSize;					// 0=8x8, 1=8x16
	static bool OBJEnable;					// 0=Off, 1=On
	static bool BGAndWindowEnable;			// BGAndWindowPriority for CGB; 0=Off, 1=On
	static u8& SCY	= bus->ram[HWAddr::SCY];	// viewport y position
	static u8& SCX	= bus->ram[HWAddr::SCX];	// viewport x position
	static u8& LY	= bus->ram[HWAddr::LY];		// LCD Y coordinate
	static u8& LYC	= bus->ram[HWAddr::LYC];	// LY compare
	static u8& WY	= bus->ram[HWAddr::WY];		// window Y position
	static u8& WX	= bus->ram[HWAddr::WX];		// window X position + 7
	static u8& BGP	= bus->ram[HWAddr::BGP];	// BG pallette data
	static u8 BGColorForIndex3;
	static u8 BGColorForIndex2;
	static u8 BGColorForIndex1;
	static u8 BGColorForIndex0;
	static u8& OBP0 = bus->ram[HWAddr::OBP0];	// BG pallette data
	static u8& OBP1 = bus->ram[HWAddr::OBP1];	// BG pallette data
	static u8 OBP0ColorForIndex3;
	static u8 OBP0ColorForIndex2;
	static u8 OBP0ColorForIndex1;
	static u8 OBP0ColorForIndex0 = ColorIndex::Transparent;
	static u8 OBP1ColorForIndex3;
	static u8 OBP1ColorForIndex2;
	static u8 OBP1ColorForIndex1;
	static u8 OBP1ColorForIndex0 = ColorIndex::Transparent;
	static u8& DMA	= bus->ram[HWAddr::DMA];
	static u8& STAT	= bus->ram[HWAddr::STAT];
	static u8 modeFlag;
	static bool LYC_equals_LYFlag;
	static bool Mode0StatInterruptEnable;
	static bool Mode1StatInterruptEnable;
	static bool Mode2StatInterruptEnable;
	static bool LYC_equals_LYStatInterruptEnable;
	static u8& IF = bus->ram[HWAddr::IF];


	LCDEnable				= LCDC & (1 << 7);
	WindowTileMapArea		= LCDC & (1 << 6);
	WindowEnable			= LCDC & (1 << 5);
	BGAndWindowTileDataArea	= LCDC & (1 << 4);
	BGTileMapArea			= LCDC & (1 << 3);
	OBJSize					= LCDC & (1 << 2);
	OBJEnable				= LCDC & (1 << 1);
	BGAndWindowEnable		= LCDC & (1 << 0);
	BGColorForIndex3	= (BGP & 0b11000000) >> 6;
	BGColorForIndex2	= (BGP & 0b00110000) >> 4;
	BGColorForIndex1	= (BGP & 0b00001100) >> 2;
	BGColorForIndex0	= (BGP & 0b00000011) >> 0;
	OBP0ColorForIndex3		= (OBP0 & 0b11000000) >> 6;
	OBP0ColorForIndex2		= (OBP0 & 0b00110000) >> 4;
	OBP0ColorForIndex1		= (OBP0 & 0b00001100) >> 2;
	OBP1ColorForIndex3		= (OBP1 & 0b11000000) >> 6;
	OBP1ColorForIndex2		= (OBP1 & 0b00110000) >> 4;
	OBP1ColorForIndex1		= (OBP1 & 0b00001100) >> 2;
	LYC_equals_LYStatInterruptEnable	= STAT & 0b01000000;
	Mode2StatInterruptEnable			= STAT & 0b00100000;
	Mode1StatInterruptEnable			= STAT & 0b00010000;
	Mode0StatInterruptEnable			= STAT & 0b00001000;
	LYC_equals_LYFlag					= STAT & 0b00000100;
	modeFlag							= STAT & 0b00000011;

	//STAT bit 7 is always 1
	STAT |= 0b10000000;

	static int DMATransferCycles = 0;
	if (DoDMATransfer && DMATransferCycles == 0)
	{
		DMATransferCycles = 160;
	}

	if (DMATransferCycles > 0)
	{
		DMATransferCycles--;

		if (DMATransferCycles == 0)
		{
			DoDMATransfer = false;

			static u16 startAddr;
			startAddr = DMA << 8;

			for (u16 currAddr = startAddr; currAddr < startAddr + 160; currAddr++)
			{
				static u8 data;
				data = bus->ram[currAddr];
				Write(HWAddr::OAM + (currAddr - startAddr), data);
			}
		}
	}

	if (LCDEnable)
	{
		if (LY >= 144)
		{
			if (mode != Mode::VBlank)
			{
				mode = Mode::VBlank;
				if (Mode1StatInterruptEnable)
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
		}
		else if (currScanlineCycles < 80)
		{
			if (mode != Mode::OAMScan)
			{
				mode = Mode::OAMScan;
				if (Mode2StatInterruptEnable)
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
		}
		else if (currScanlineCycles < 172) // 289
		{
			mode = Mode::Draw;
		}
		else if (currScanlineCycles < 456)
		{
			if (mode != Mode::HBlank)
			{
				mode = Mode::HBlank;
				if (Mode0StatInterruptEnable)
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
		}

		STAT &= ~0b00000011;
		STAT |= (u8)mode;



		switch (mode)
		{
			case PPU::Mode::OAMScan:
			{
				break;
			}

			case PPU::Mode::Draw:
				if (currScanlineCycles == 80)
				{
					// are we using the window?
					static bool usingWindow;
					usingWindow = (WindowEnable && WY <= LY) ? true : false;

					// which bg map?
					static u16 BGAddr;
					if (usingWindow)
					{
						BGAddr = WindowTileMapArea ? 0x9C00 : 0x9800;
					}
					else
					{
						BGAddr = BGTileMapArea ? 0x9C00 : 0x9800;
					}

					// which tile data set are we using?
					static u16 tileDataBaseAddr;
					static bool isSigned;
					tileDataBaseAddr	= BGAndWindowTileDataArea ? 0x8000 : 0x9000;
					isSigned			= BGAndWindowTileDataArea ?  false :   true;

					// calculate the tile row on the screen
					static u8 tileRowOffset;
					if (usingWindow)
					{
						tileRowOffset = (u8)(LY + WY) / (u8)8;
					}
					else
					{
						tileRowOffset = (u8)(SCY + LY) / (u8)8;
					}

					// which of the 8 vertical pixels of the current tile is the scanline on?
					static u8 pixelRowOffset;
					pixelRowOffset = LY % 8 * 2; // each row takes up two bytes of memory

					// time to start drawing the scanline
					for (int pixel = 0; pixel < DMGScreenWidth; pixel++)
					{
						static u8 xPos;

						if (usingWindow && pixel >= WX)
						{
							xPos = pixel - WX;
						}
						else
						{
							xPos = pixel + SCX;
						}

						// calculate the tile column
						static u16 tileColOffset;
						tileColOffset = (xPos / 8);

						// get the tile id number. Remember it can be signed or unsigned
						static u16 tileIdAddr;
						static int tileId;
						tileIdAddr = BGAddr + (tileRowOffset * 32) + tileColOffset; // there are 32 rows of tiles in memory
						tileId = isSigned ? (s8)(bus->ram[tileIdAddr]) : bus->ram[tileIdAddr];

						// calculate the address of the tile data
						static u16 tileDataAddr;
						tileDataAddr = tileDataBaseAddr + (tileId * 16); // 16 bits per row of pixels within the tile

						// calculate the vertical position within the tile data
						static u8 data0;
						static u8 data1;
						data0 = bus->ram[tileDataAddr + pixelRowOffset];
						data1 = bus->ram[tileDataAddr + pixelRowOffset + 1];

						// pixel 0 in the tile is bit 7 of both data1 and data2. Pixel 1 is bit 6 etc..
						static u8 colorBitIndex;
						colorBitIndex = 7 - (xPos % 8);

						// combine data2 and data1 to get the color id for this pixel in the tile
						static u8 colorIndex;
						static bool colorBit0;
						static bool colorBit1;
						colorBit0 = data0 & (1 << colorBitIndex);
						colorBit1 = data1 & (1 << colorBitIndex);
						colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;

						// now we have the color id, get the actual color from BG palette 0xFF47
						static u8 color;
						switch (colorIndex)
						{
							case 0:
								color = BGColorForIndex0;
								break;

							case 1:
								color = BGColorForIndex1;
								break;

							case 2:
								color = BGColorForIndex2;
								break;

							case 3:
								color = BGColorForIndex3;
								break;

							default:

								break;
						}

						screen[(LY * DMGScreenWidth) + pixel] = GameBoyColors[color];
					}
				}
				break;
			case PPU::Mode::HBlank:
				break;
			case PPU::Mode::VBlank:
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
				if (LYC_equals_LYStatInterruptEnable)
				{
					IF |= InterruptFlags::LCDStat; // request LCDStat interrupt
				}
			}
			else
			{
				STAT &= ~0x0000100;
			}
		}
	}
	else
	{
		// disable all of this since the lcd is disabled
		LY = 0;
		LYC = 0;
		STAT &= ~0b01111111; 
	}

	cycles = (cycles + 1) % 70224;
	if (cycles == 0)
	{
		SDL_UpdateTexture(dmgScreen, NULL, screen, DMGScreenWidth* BytesPerPixel);
		frameComplete = true;
	}
}

bool PPU::FrameComplete()
{
	return frameComplete;
}