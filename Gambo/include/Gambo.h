#include "GamboDefine.h"
#include "Bus.h"


class Gambo
{
public:
	Gambo();
	~Gambo();

	void Run();

	Gambo(const Gambo& other) = delete;
	Gambo& operator=(const Gambo&) = delete;

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* dmgScreen = nullptr;
	SDL_Texture* windowTexture = nullptr;
	Bus gb;
	bool running = false;

private:
	void Render();

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, SDL_Color col = WHITE, u32 scale = 1);
	void DrawCpu(SDL_Color* target, int targetWidth, int targetHeight, int x, int y);
	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
	void DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines);
};