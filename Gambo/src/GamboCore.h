#pragma once
#include "GamboDefine.h"
#include "Bus.h"
#include <atomic>

class Frontend;

class GamboCore
{
public:
	GamboCore(Frontend* fe);
	~GamboCore();

	void Run();

	SDL_Texture* GetScreen();
	float GetScreenWidth();
	float GetScreenHeight();

	std::atomic<bool> done = false;

private:
	Bus gb;

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, SDL_Color col = WHITE, u32 scale = 1);
	void DrawCpu(SDL_Color* target, int targetWidth, int targetHeight, int x, int y);
	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	
	Frontend* frontend;
	SDL_Texture* screen = nullptr;
	float screenWidth = DMGScreenWidth;
	float screenHeight = DMGScreenHeight;
	int screenScale = PixelScale; 
	bool running = false;
};