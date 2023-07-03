#include "GamboDefine.h"
#include "Bus.h"

class Frontend;

class GamboCore
{
public:
	GamboCore(const std::shared_ptr<Frontend> fe);
	~GamboCore();

	void Run();

	GamboCore(const GamboCore& other) = delete;
	GamboCore& operator=(const GamboCore&) = delete;

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* dmgScreen = nullptr;
	SDL_Texture* windowTexture = nullptr;
	Bus gb;
	bool running = false;

private:
	std::shared_ptr<Frontend> frontend;
	void Render();

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, SDL_Color col = WHITE, u32 scale = 1);
	void DrawCpu(SDL_Color* target, int targetWidth, int targetHeight, int x, int y);
	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
};