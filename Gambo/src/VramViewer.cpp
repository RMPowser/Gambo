#include "VramViewer.h"
#include "Ram.h"
#include "PPU.h"

VramViewer::VramViewer(RAM* r)
	: ram(r)
{
	bg0.fill({ 0, 0, 0, 255 });
	bg1.fill({ 0, 0, 0, 255 });
}

VramViewer::~VramViewer()
{
}

const std::array<SDL_Color, 256 * 256>& VramViewer::GetView(u16 baseAddr)
{
	const u8 LCDC = Read(HWAddr::LCDC);
	const u8 SCY = Read(HWAddr::SCY);	// viewport y position
	const u8 SCX = Read(HWAddr::SCX);	// viewport x position
	const u8 BGP = Read(HWAddr::BGP);	// BG pallette data
	const u8 WY = Read(HWAddr::WY);	// window Y position
	const u8 WX = Read(HWAddr::WX);	// window X position + 7

	const int lineWidth = 256;

	int pixelsToDraw = 4;

	bool usingWindow = false;

	auto tileMapBitSelect = usingWindow ? LCDCBits::WindowTileMapArea : LCDCBits::BGTileMapArea;
	int bgDataAddr = GetBits(LCDC, (u8)tileMapBitSelect, 0x1) ? 0x9C00 : 0x9800;

	u16 tileDataBaseAddr = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? 0x8000 : 0x9000;
	bool isSigned = GetBits(LCDC, (u8)LCDCBits::TileDataArea, 0b1) ? false : true;


	for (int y = 0; y < 256; y++)
	{
		u8 tileRow = y / 8;

		// which of the 8 vertical pixels of the current tile is the scanline on?
		u8 tilePixelRow = (y % 8) * 2; // each row takes up two bytes of memory

		for (int x = 0; x < 256; x++)
		{
			u16 tileColumn = (x / 8);

			// get the tile id number. Remember it can be signed or unsigned
			u16 tileIdAddr = bgDataAddr + (tileRow * 32) + tileColumn; // there are 32 rows of tiles in memory
			int tileId = isSigned ? (s8)Read(tileIdAddr) : Read(tileIdAddr);

			u16 tileDataAddr = tileDataBaseAddr + (tileId * 16); // 16 bits per row of pixels within the tile

			// get the two bytes that hold the color data for this pixel
			u8 data0 = Read(tileDataAddr + tilePixelRow);
			u8 data1 = Read(tileDataAddr + tilePixelRow + 1);

			// pixel 0 in the tile is bit 7 of both data0 and data1. Pixel 1 is bit 6 etc..
			u8 colorBitIndex = 7 - (x % 8);

			// combine data0 and data1 to get the color id for this pixel in the tile
			bool colorBit0 = data0 & (1 << colorBitIndex);
			bool colorBit1 = data1 & (1 << colorBitIndex);
			u8 colorIndex = ((int)colorBit1 << 1) | (int)colorBit0;

			// now we have the color id, get the actual color from BG palette 0xFF47
			u8 color = GetBits(BGP, colorIndex * 2, 0b11);

			// we can finally draw a pixel
			int index = (y * lineWidth) + x;
			bg0[index] = GameBoyColors[color];
		}
	}

	return bg0;
}

u8 VramViewer::Read(u16 addr)
{
	return ram->Read(addr);
}
