#pragma once
#include "GamboDefine.h"
#include "Cartridge.h"
#include "Bus.h"
#include <atomic>

struct GamboState
{
	struct _flags
	{
		bool Z, N, H, C, IME;
	} flags;

	struct _reg
	{
		u8 A, F, B, C, D, E, H, L;
	} registers;

	u16 SP;
	u16 PC;

	u8 LCDC;
	u8 STAT;
	u8 LY;
	u8 IE;
	u8 IF;

	std::map<uint16_t, std::string> mapAsm;
};

class GamboCore
{
public:
	GamboCore();
	~GamboCore();

	void Run();

	void* GetScreen() const;
	float GetScreenWidth() const;
	float GetScreenHeight() const;
	GamboState GetState() const;

	void InsertCartridge(std::wstring filePath);

	void SetUseBootRom(bool b);
	bool GetUseBootRom();

	std::atomic<bool> done = false;
	std::atomic<bool> running = false;

private:
	Bus gb;

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, ImVec4 col = WHITE, u32 scale = 1);
	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	
	float screenWidth = GamboScreenWidth;
	float screenHeight = GamboScreenHeight;
	int screenScale = PixelScale; 
};