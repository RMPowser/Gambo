#pragma once
#include "SDL.h"
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <atomic>
#include <map>
#include <filesystem>

typedef uint64_t u64;
typedef  int64_t s64;
typedef uint32_t u32;
typedef  int32_t s32;
typedef uint16_t u16;
typedef  int16_t s16;
typedef uint8_t u8;
typedef  int8_t s8;

#pragma warning(push)
#pragma warning(disable: 4455)
constexpr unsigned long long operator""KiB(unsigned long long const x)
{
	return 1024L * x;
}

constexpr unsigned long long operator""MiB(unsigned long long const x)
{
	return 1024L * 1024L * x;
}
#pragma warning(pop)

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
	static constexpr unsigned short BOOT  = 0xFF50;
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

inline constexpr auto GamboScreenWidth = 160;
inline constexpr auto GamboScreenHeight = 144;
inline constexpr auto GamboScreenSize = GamboScreenWidth * GamboScreenHeight;
inline constexpr auto GamboAspectRatio = (float)GamboScreenWidth / (float)GamboScreenHeight;
inline constexpr auto BytesPerPixel = 4;
static auto PixelScale = 5;
inline constexpr auto PixelScaleMax = 8;
static constexpr auto TabSizeInSpaces = 4;

#define SAFE_DELETE(ptr) if (ptr) { delete ptr; ptr = nullptr; }
#define SAFE_DELETE_ARRAY(ptr) if (ptr) { delete[] ptr; ptr = nullptr; }

static const ImVec4
	GREY				{ 0.75, 0.75, 0.75, 1 },
	DARK_GREY			{ 0.5, 0.5, 0.5, 1 },
	VERY_DARK_GREY		{ 0.25, 0.25, 0.25, 1 },
	RED					{ 1, 0, 0, 1 },
	DARK_RED			{ 0.5, 0, 0, 1 },
	VERY_DARK_RED		{ 0.25, 0, 0, 1 },
	YELLOW				{ 1, 1, 0, 1 },
	DARK_YELLOW			{ 0.5, 0.5, 0, 1 },
	VERY_DARK_YELLOW	{ 0.25, 0.25, 0, 1 },
	GREEN				{ 0, 1, 0, 1 },
	DARK_GREEN			{ 0, 0.5, 0, 1 },
	VERY_DARK_GREEN		{ 0, 0.25, 0, 1 },
	CYAN				{ 0, 1, 1, 1 },
	DARK_CYAN			{ 0, 0.5, 0.5, 1 },
	VERY_DARK_CYAN		{ 0, 0.25, 0.25, 1 },
	BLUE				{ 0, 0, 1, 1 },
	DARK_BLUE			{ 0, 0, 0.5, 1 },
	VERY_DARK_BLUE		{ 0, 0, 0.25, 1 },
	MAGENTA				{ 1, 0, 1, 1 },
	DARK_MAGENTA		{ 0.5, 0, 0.5, 1 },
	VERY_DARK_MAGENTA	{ 0.25, 0, 0.25, 1 },
	WHITE				{ 1, 1, 1, 1 },
	BLACK				{ 0, 0, 0, 1 },
	BLANK				{ 0, 0, 0, 0 };

static inline std::string hex(uint32_t n, uint8_t d)
{
	std::string s(d, '0');
	for (int i = d - 1; i >= 0; i--, n >>= 4)
		s[i] = "0123456789ABCDEF"[n & 0xF];
	return s;
};


enum InterruptFlags
{
	None	= 0b00000000,
	VBlank	= 0b00000001,
	LCDStat	= 0b00000010,
	Timer	= 0b00000100,
	Serial	= 0b00001000,
	Joypad	= 0b00010000,
};

enum ColorIndex
{
	White,
	LightGray,
	DarkGray,
	Black,
	Transparent // for use in sprites
};

static SDL_Color GameBoyColors[5]
{
	{ 200, 200, 15, 255 },
	{ 139, 172, 15, 255 },
	{ 48, 98, 48, 255 },
	{ 15, 56, 15, 255 },
	{ 0, 0, 0, 0 }, // transparent for use in sprites
};

static const auto DesiredFPS = 60;

constexpr u8 GetBits(u8 reg, u8 bitIndex, u8 bitMask)
{
	return (reg & (bitMask << bitIndex)) >> bitIndex;
}

constexpr void SetBit(u8& reg, u8 bitIndex, bool value)
{
	value == true 
		? reg |= 1 << bitIndex 
		: reg &= ~(1 << bitIndex);
}