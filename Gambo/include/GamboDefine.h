#pragma once
#include "SDL.h"
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

#include <cstdint>
#include <vector>
#include <string>

typedef uint64_t u64;
typedef  int64_t s64;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint8_t u8;
typedef  int8_t s8;

// addresses of hardware registers
namespace HWAddr 
{
	static constexpr unsigned short OAM = 0xFE00;
	static constexpr unsigned short WindowTileMap0 = 0x9800;
	static constexpr unsigned short WindowTileMap1 = 0x9C00;
	static constexpr unsigned short BGAndWindTileData0 = 0x8800;
	static constexpr unsigned short BGAndWindTileData1 = 0x8000;
	static constexpr unsigned short BGTileMap0 = 0x9800;
	static constexpr unsigned short BGTileMap1 = 0x9C00;
	static constexpr unsigned short P1    = 0xFF00;
	static constexpr unsigned short SB    = 0xFF01;
	static constexpr unsigned short SC    = 0xFF02;
	static constexpr unsigned short DIV   = 0xFF04;
	static constexpr unsigned short TIMA  = 0xFF05;
	static constexpr unsigned short TMA   = 0xFF06;
	static constexpr unsigned short TAC   = 0xFF07;
	static constexpr unsigned short IF    = 0xFF0F;
	static constexpr unsigned short NR10  = 0xFF10;
	static constexpr unsigned short NR11  = 0xFF11;
	static constexpr unsigned short NR12  = 0xFF12;
	static constexpr unsigned short NR13  = 0xFF13;
	static constexpr unsigned short NR14  = 0xFF14;
	static constexpr unsigned short NR21  = 0xFF16;
	static constexpr unsigned short NR22  = 0xFF17;
	static constexpr unsigned short NR23  = 0xFF18;
	static constexpr unsigned short NR24  = 0xFF19;
	static constexpr unsigned short NR30  = 0xFF1A;
	static constexpr unsigned short NR31  = 0xFF1B;
	static constexpr unsigned short NR32  = 0xFF1C;
	static constexpr unsigned short NR33  = 0xFF1D;
	static constexpr unsigned short NR34  = 0xFF1E;
	static constexpr unsigned short NR41  = 0xFF20;
	static constexpr unsigned short NR42  = 0xFF21;
	static constexpr unsigned short NR43  = 0xFF22;
	static constexpr unsigned short NR44  = 0xFF23;
	static constexpr unsigned short NR50  = 0xFF24;
	static constexpr unsigned short NR51  = 0xFF25;
	static constexpr unsigned short NR52  = 0xFF26;
	static constexpr unsigned short LCDC  = 0xFF40;
	static constexpr unsigned short STAT  = 0xFF41;
	static constexpr unsigned short SCY   = 0xFF42;
	static constexpr unsigned short SCX   = 0xFF43;
	static constexpr unsigned short LY    = 0xFF44;
	static constexpr unsigned short LYC   = 0xFF45;
	static constexpr unsigned short DMA   = 0xFF46;
	static constexpr unsigned short BGP   = 0xFF47;
	static constexpr unsigned short OBP0  = 0xFF48;
	static constexpr unsigned short OBP1  = 0xFF49;
	static constexpr unsigned short WY    = 0xFF4A;
	static constexpr unsigned short WX    = 0xFF4B;
	static constexpr unsigned short KEY1  = 0xFF4D;
	static constexpr unsigned short VBK   = 0xFF4F;
	static constexpr unsigned short HDMA1 = 0xFF51;
	static constexpr unsigned short HDMA2 = 0xFF52;
	static constexpr unsigned short HDMA3 = 0xFF53;
	static constexpr unsigned short HDMA4 = 0xFF54;
	static constexpr unsigned short HDMA5 = 0xFF55;
	static constexpr unsigned short RP    = 0xFF56;
	static constexpr unsigned short BCPS  = 0xFF68;
	static constexpr unsigned short BCPD  = 0xFF69;
	static constexpr unsigned short OCPS  = 0xFF6A;
	static constexpr unsigned short OCPD  = 0xFF6B;
	static constexpr unsigned short SVBK  = 0xFF70;
	static constexpr unsigned short IE    = 0xFFFF;
}

static constexpr auto WindowTitle = "Gambo";
inline constexpr auto DMGScreenWidth = 160;
inline constexpr auto DMGScreenHeight = 144;
static auto PixelScale = 2;
static constexpr auto TabSizeInSpaces = 4;

#define SAFE_DELETE(ptr) if (ptr) { delete ptr; ptr = nullptr; }
#define SAFE_DELETE_ARRAY(ptr) if (ptr) { delete[] ptr; ptr = nullptr; }

#define X 1
static const std::vector<bool> fontSheet =
{
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, X, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, 0, X, X, X, 0,   0, X, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, X, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   X, X, X, X, X, X, X, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, X, X, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, X, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, X, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, X, X, X, X, X, X,   0, X, X, X, X, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   X, X, X, X, X, X, X, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, X, X, X, X,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, X, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, 0, 0, 0, 0, X, X, 0,   0, 0, X, X, X, 0, 0, X,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, X, X, X, 0,   0, X, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	
	0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,
	0, X, X, 0, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, 0, 0, X, X, 0,
	0, X, X, X, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	
	0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, 0, 0, 0,   0, X, X, X, X, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   X, X, X, 0, X, X, X, 0,   0, X, X, X, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, X, X, 0, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, X, X, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   X, X, X, X, X, X, X, 0,   0, X, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, X, X, 0, X, 0, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   0, X, X, 0, X, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, X, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   X, X, 0, X, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, X, X, 0, X, X, X, 0,   0, X, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, X, X, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, 0, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, 0, 0, 0,   0, X, X, X, X, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	
	0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, X, X, X, X, 0,   0, X, 0, 0, 0, 0, 0, 0,   0, X, X, X, X, 0, 0, 0,   0, 0, 0, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, X, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, X, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, 0, 0, 0,   0, X, X, X, 0, X, X, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   X, X, X, X, X, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   X, X, X, 0, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, 0, 0, 0,   0, 0, X, X, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, X, X, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, 0, 0, 0, X, X, X,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, X, X, X, X, 0,   0, 0, 0, 0, 0, 0, X, 0,   0, X, X, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   X, X, X, X, X, X, X, 0,
	
	X, X, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, X, X, 0, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, X, 0, X, X, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, X, 0, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, X, X, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, 0, X, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, X, X, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   X, X, X, X, 0, 0, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   X, X, 0, X, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   X, 0, 0, X, X, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, X, X, X, X, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, 0, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, 0, X, X, 0, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, X, X, 0, 0, 0,   0, 0, 0, 0, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, X, X, X, 0, 0,   0, 0, X, X, X, X, X, 0,   0, X, X, 0, 0, 0, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, 0, 0, X, X, X, 0,   0, 0, X, X, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, 0, X, X, 0, 0,   0, X, X, 0, 0, X, X, 0,   0, 0, 0, 0, 0, X, X, 0,   0, X, X, X, X, X, X, 0,   0, 0, 0, 0, X, X, X, 0,   0, 0, 0, X, X, 0, 0, 0,   0, X, X, X, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, X, X, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, X, X, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, X, X, X, X, X, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};
#undef X

static const SDL_Color
GREY(192, 192, 192), DARK_GREY(128, 128, 128), VERY_DARK_GREY(64, 64, 64),
RED(255, 0, 0), DARK_RED(128, 0, 0), VERY_DARK_RED(64, 0, 0),
YELLOW(255, 255, 0), DARK_YELLOW(128, 128, 0), VERY_DARK_YELLOW(64, 64, 0),
GREEN(0, 255, 0), DARK_GREEN(0, 128, 0), VERY_DARK_GREEN(0, 64, 0),
CYAN(0, 255, 255), DARK_CYAN(0, 128, 128), VERY_DARK_CYAN(0, 64, 64),
BLUE(0, 0, 255), DARK_BLUE(0, 0, 128), VERY_DARK_BLUE(0, 0, 64),
MAGENTA(255, 0, 255), DARK_MAGENTA(128, 0, 128), VERY_DARK_MAGENTA(64, 0, 64),
WHITE(255, 255, 255), BLACK(0, 0, 0), BLANK(0, 0, 0, 0);

static inline std::string hex(uint32_t n, uint8_t d)
{
	std::string s(d, '0');
	for (int i = d - 1; i >= 0; i--, n >>= 4)
		s[i] = "0123456789ABCDEF"[n & 0xF];
	return s;
};

enum ColorIndex
{
	White,
	LightGray,
	DarkGray,
	Black,
	Transparent // for use in sprites
};

enum InterruptFlags
{
	VBlank	= 0b00000001,
	LCDStat	= 0b00000010,
	Timer	= 0b00000100,
	Serial	= 0b00001000,
	Joypad	= 0b00010000,
};

static SDL_Color GameBoyColors[5]
{
	{ 228, 228, 228, 228 },
	{ 192, 192, 192, 255 },
	{ 96, 96, 96, 255 },
	{ 0, 0, 0, 255 },
	{ 0, 0, 0, 0 }, // transparent for use in sprites
};

static const auto BytesPerPixel = 4;
static const auto DesiredFPS = 60;
static auto FrameStart = 0.f;
static auto FrameTime = 0.f;
