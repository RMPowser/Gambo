#pragma once
#include <atomic>
#include "SDL.h"
#include "GamboCore.h"
#include "FileDialogs.h"
#include <filesystem>

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
	void OpenGameFromFile(std::filesystem::path filePath = FileDialogs::OpenFile(L"Game Boy Rom\0*.gb"));

	std::unique_ptr<GamboCore> gambo;
	SDL_Texture* gamboScreen = nullptr;
	SDL_Texture* gamboVramView = nullptr;
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;

	bool done = false;
	bool integerScale = true;
	bool maintainAspectRatio = true;

	// helpers
	void DrawGamboWindow();
	void DrawDebugInfoWindow();
	void DrawVramViewer();
	void SetGamboRunning();
	void SetGamboStep();
};