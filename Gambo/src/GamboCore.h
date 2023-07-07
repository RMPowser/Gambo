#pragma once
#include "GamboDefine.h"
#include "Bus.h"
#include <atomic>

class Frontend;

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
};

class GamboCore
{
public:
	GamboCore(Frontend* fe);
	~GamboCore();

	void Run();

	SDL_Texture* GetScreen() const;
	float GetScreenWidth() const;
	float GetScreenHeight() const;
	GamboState GetState() const;

	std::atomic<bool> done = false;

private:
	Bus gb;

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, ImVec4 col = WHITE, u32 scale = 1);
	void DrawCpu(SDL_Color* target, int targetWidth, int targetHeight, int x, int y);
	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	
	Frontend* frontend;
	std::atomic<SDL_Texture*> screen = nullptr;
	float screenWidth = DMGScreenWidth;
	float screenHeight = DMGScreenHeight;
	int screenScale = PixelScale; 
	bool running = false;
};