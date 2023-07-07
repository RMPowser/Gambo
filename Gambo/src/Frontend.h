#pragma once
#include <atomic>
#include "SDL.h"
#include "GamboCore.h"

class Frontend
{
public:
	Frontend();
	~Frontend();
	
	void Run();

private:
	void BeginFrame();
	void UpdateUI();
	void EndFrame();
	void HandleKeyboardShortcuts();

	std::unique_ptr<GamboCore> gambo;
	SDL_Texture* gamboScreen = nullptr;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	bool done = false;
};