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
	static int mode = -1;
	static int currScanlineCycles = 0;

	static u8& LCDC = bus->ram[HWAddr::LCDC];
	static bool LCDAndPPUEnable;			// 0=Off, 1=On
	static bool WindowTileMapArea;			// 0=9800-9BFF, 1=9C00-9FFF
	static bool WindowEnable;				// 0=Off, 1=On
	static bool BGAndWindowTileDataArea;	// 0=8800-97FF, 1=8000-8FFF
	static bool BGTileMapArea;				// 0=9800-9BFF, 1=9C00-9FFF
	static bool OBJSize;					// 0=8x8, 1=8x16
	static bool OBJEnable;					// 0=Off, 1=On
	static bool BGAndWindowEnable;			// BGAndWindowPriority for CGB; 0=Off, 1=On
	static u8& SCY = bus->ram[HWAddr::SCY];	// viewport y position
	static u8& SCX = bus->ram[HWAddr::SCX];	// viewport x position
	static u8& LY = bus->ram[HWAddr::LY];	// LCD Y coordinate
	static u8& LYC = bus->ram[HWAddr::LYC];	// LY compare
	static u8& WY = bus->ram[HWAddr::WY];	// window Y position
	static u8& WX = bus->ram[HWAddr::WX];	// window X position + 7
	static u8& BGP = bus->ram[HWAddr::BGP];	// BG pallette data
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
	static u8& DMA = bus->ram[HWAddr::DMA];


	LCDAndPPUEnable			= LCDC & (1 << 7);
	WindowTileMapArea		= LCDC & (1 << 6);
	WindowEnable			= LCDC & (1 << 5);
	BGAndWindowTileDataArea	= LCDC & (1 << 4);
	BGTileMapArea			= LCDC & (1 << 3);
	OBJSize					= LCDC & (1 << 2);
	OBJEnable				= LCDC & (1 << 1);
	BGAndWindowEnable		= LCDC & (1 << 0);
	BGColorForIndex3		= BGP & 0b11000000;
	BGColorForIndex2		= BGP & 0b00110000;
	BGColorForIndex1		= BGP & 0b00001100;
	BGColorForIndex0		= BGP & 0b00000011;
	OBP0ColorForIndex3		= OBP0 & 0b11000000;
	OBP0ColorForIndex2		= OBP0 & 0b00110000;
	OBP0ColorForIndex1		= OBP0 & 0b00001100;
	OBP1ColorForIndex3		= OBP1 & 0b11000000;
	OBP1ColorForIndex2		= OBP1 & 0b00110000;
	OBP1ColorForIndex1		= OBP1 & 0b00001100;

	if (!LCDAndPPUEnable)
	{
		return;
	}

	if (DoDMATransfer)
	{
		static int DMATransferCycles = 0;
		if (DMATransferCycles == 0) { DMATransferCycles = 160; }

		DoDMATransfer = false;
	}


	if (LY > 143)
	{
		mode = 1; // vblank
	}
	else if (currScanlineCycles < 80)
	{
		mode = 2; // OAM scan
	}
	else if (currScanlineCycles < 172) // 289
	{
		mode = 3; // drawing pixels
	}
	else if (currScanlineCycles < 456)
	{
		mode = 0; // hblank
	}



	switch (mode)
	{
		case 2:
		{
			// OAM scan
			u16 ybase = (SCY + LY);   // calculates the effective scanline
			u16 addr = (0x9800 | (BGTileMapArea << 10) | ((ybase & 0xf8) << 2) | ((SCX & 0xf8) >> 3));

			break;
		}

		case 3:
		{
			break;
		}

		case 0:
		{
			break;
		}

		case 1:
		{
			break;
		}

		default:
			throw;
	}

	currScanlineCycles = (currScanlineCycles + 1) % 456;
	if (currScanlineCycles == 0)
	{
		LY = (LY + 1) % 153;
	}

	static bool frameComplete;
	frameComplete = false;

	cycles = (cycles + 1) % 70224;
	if (cycles == 0)
	{
		frameComplete = true;

		static SDL_Color* target = new SDL_Color[DMGScreenWidth * DMGScreenHeight];
		static int rowByteLength = 0;

		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<> dist(0, 3);
		for (size_t row = 0; row < DMGScreenWidth; row++)
		{
			for (size_t col = 0; col < DMGScreenHeight; col++)
			{
				target[row + (col * DMGScreenWidth)] = GameBoyColors[dist(gen)];
			}
		}

		SDL_UpdateTexture(dmgScreen, NULL, target, DMGScreenWidth * BytesPerPixel);
	}
}

bool PPU::FrameComplete()
{
	return cycles == 0;
}