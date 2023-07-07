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
	
	SDL_Window* GetWindow();
	SDL_Renderer* GetRenderer();


private:
	void BeginFrame();
	void UpdateUI();
	void EndFrame();
	void HandleKeyboardShortcuts();

	std::unique_ptr<GamboCore> gambo;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	bool done = false;
};