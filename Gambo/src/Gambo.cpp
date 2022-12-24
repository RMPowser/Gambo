#include "Bus.h"
#include <fstream>
#include <random>
#include <format>
#include <chrono>
#include <iostream>


class Gambo
{
public:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* dmgScreen = nullptr;
	SDL_Texture* windowTexture = nullptr;
	Bus gb;
	std::map<uint16_t, std::string> mapAsm;
	bool running = false;

	Gambo()
	{
		SDL_assert_release(SDL_Init(SDL_INIT_EVERYTHING) == 0);

		window = SDL_CreateWindow(WindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * PixelScale, 540 * PixelScale, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
		SDL_assert_release(window);

		SDL_assert_release(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));

		renderer = SDL_GetRenderer(window);
		SDL_assert_release(renderer);

		dmgScreen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, DMGScreenWidth, DMGScreenHeight);
		SDL_assert_release(dmgScreen);

		int windowWidth, windowHeight;
		SDL_GetWindowSize(window, &windowWidth, &windowHeight);
		windowTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
		SDL_assert_release(windowTexture);

		std::ifstream input("C:\\Users\\Ryan\\source\\repos\\Gambo\\Gambo\\test roms\\cpu_instrs.gb", std::ios::binary);

		// copies all data into buffer
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
		uint16_t offset = 0x0000;

		// copy the buffer into ram
		for (auto& byte : buffer)
		{
			gb.ram[offset++] = byte;
		}

		mapAsm = gb.cpu.Disassemble(0x0000, 0xFFFF);

		gb.cpu.Reset();

		Render();
	}

	~Gambo()
	{
		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
		}

		SDL_Quit();
	}

	Gambo(const Gambo& other) = delete;
	Gambo& operator=(const Gambo&) = delete;

	void Run()
	{
		using clock = std::chrono::high_resolution_clock;
		using frames = std::chrono::duration<double, std::ratio<1, 60>>;

		LARGE_INTEGER nextFrame;

		while (true)
		{
			QueryPerformanceCounter(&nextFrame);
			nextFrame.QuadPart += (PerformanceFrequency.QuadPart / DesiredFPS);

			static bool step = false;

			SDL_Event e;
			while (SDL_PollEvent(&e))
			{
				switch (e.type)
				{
					case SDL_KEYDOWN:
						if (e.key.keysym.sym == SDLK_p)
						{
							running = !running;
						}
						else if (e.key.keysym.sym == SDLK_r)
						{
							gb.cpu.Reset();
						}
						else if (e.key.keysym.sym == SDLK_SPACE)
						{
							step = true;
						}
						break;

					case SDL_WINDOWEVENT:
						switch (e.window.event)
						{
							case SDL_WINDOWEVENT_CLOSE:
								return;
							default:
								break;
						}
						break;

					default:
						break;
				}
			}


			if (running)
			{
				do
				{
					gb.cpu.Clock();
					gb.ppu.Clock(dmgScreen);
				} while (!gb.ppu.FrameComplete());
			}
			else if (step)
			{
				do
				{
					gb.cpu.Clock();
					gb.ppu.Clock(dmgScreen);
				} while (!gb.cpu.InstructionComplete());
				step = false;
			}


			Render();


			decltype(nextFrame) i = {};
			QueryPerformanceCounter(&i);
			while (i.QuadPart < nextFrame.QuadPart)
			{
				QueryPerformanceCounter(&i);
			}
		}
	}

private:
	void Render()
	{
		static SDL_Color* target = nullptr;
		static int rowByteLength = 0;

		int targetWidth;
		int targetHeight;
		SDL_QueryTexture(windowTexture, NULL, NULL, &targetWidth, &targetHeight);

		SDL_LockTexture(windowTexture, NULL, (void**)&target, &rowByteLength);
		{
			memset(target, 32, targetWidth * targetHeight * 4);
			// Draw Ram Page 0x00
			//DrawRam(2, 2, 0x0000, 16, 16);
			//DrawRam(2, 182, 0x0100, 16, 16);
			DrawCpu(target, targetWidth, targetHeight, 448, 2);
			DrawCode(target, targetWidth, targetHeight, 448, 112, 22);

			DrawString(target, targetWidth, targetHeight, 10, 500, "SPACE = Step Instruction    R = RESET    P = PLAY");
			DrawString(target, targetWidth, targetHeight, 10, 510, std::format("FPS: {}", "???"));
		}
		SDL_UnlockTexture(windowTexture);


		SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, windowTexture, NULL, NULL);
		static SDL_Rect destRect = { 0, 0, DMGScreenWidth * PixelScale * 2, DMGScreenHeight * PixelScale * 2 };
		SDL_RenderCopy(renderer, dmgScreen, NULL, &destRect);
		SDL_RenderPresent(renderer);
	}

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, SDL_Color col = WHITE, u32 scale = 1)
	{
		static u64 targetSize;
		static u32 trueScale;
		targetSize = (u64)targetWidth * (u64)targetHeight;
		trueScale = scale * PixelScale;

		s32 screenX = 0;
		s32 screenY = 0;

		for (auto c : sText)
		{
			if (c == '\n')
			{
				screenX = 0;
				screenY += 8 * trueScale;
			}
			else if (c == '\t')
			{
				screenX += 8 * TabSizeInSpaces * trueScale;
			}
			else
			{
				s32 offsetX = (c - 32) % 16;
				s32 offsetY = (c - 32) / 16;


				for (u32 i = 0; i < 8; i++)
					for (u32 j = 0; j < 8; j++)
						if (fontSheet[(i + offsetX * 8) + ((j + offsetY * 8) * 128)])
							for (u32 is = 0; is < trueScale; is++)
								for (u32 js = 0; js < trueScale; js++)
								{
									u64 index = ((x * PixelScale) + screenX + (i * trueScale) + is) + (((y * PixelScale) + screenY + (j * trueScale) + js) * targetWidth);
									if (index < targetSize)
									{
										target[index] = col;
									}
								}

				screenX += 8 * trueScale;
			}
		}
	}

	void DrawCpu(SDL_Color* target, int targetWidth, int targetHeight, int x, int y)
	{
		DrawString(target, targetWidth, targetHeight, x, y, "FLAGS:");
		DrawString(target, targetWidth, targetHeight, x + 56, y, "Z", gb.cpu.F & CPU::fZ ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x + 72, y, "N", gb.cpu.F & CPU::fN ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x + 88, y, "H", gb.cpu.F & CPU::fH ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x + 104, y, "C", gb.cpu.F & CPU::fC ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x, y + 10, "PC: $" + hex(gb.cpu.PC, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 20, "AF: $" + hex(gb.cpu.AF, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 30, "BC: $" + hex(gb.cpu.BC, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 40, "DE: $" + hex(gb.cpu.DE, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 50, "HL: $" + hex(gb.cpu.HL, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 60, "SP: $" + hex(gb.cpu.SP, 4));
	}

	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
	{
		auto it_a = mapAsm.find(gb.cpu.PC);
		SDL_assert(it_a != mapAsm.end());
		SDL_assert(it_a != mapAsm.end());

		int nLineY = (nLines >> 1) * 10 + y;
		if (it_a != mapAsm.end())
		{
			DrawString(target, targetWidth, targetHeight, x, nLineY, (*it_a).second, CYAN);
			while (nLineY < (nLines * 10) + y)
			{
				nLineY += 10;
				if (++it_a != mapAsm.end())
				{
					DrawString(target, targetWidth, targetHeight, x, nLineY, (*it_a).second);
				}
			}
		}

		it_a = mapAsm.find(gb.cpu.PC);
		nLineY = (nLines >> 1) * 10 + y;
		if (it_a != mapAsm.end())
		{
			while (nLineY > y)
			{
				nLineY -= 10;
				if (--it_a != mapAsm.end())
				{
					DrawString(target, targetWidth, targetHeight, x, nLineY, (*it_a).second);
				}
			}
		}
	}
};

int main(int argc, char* argv[])
{
	QueryPerformanceFrequency(&PerformanceFrequency);
	std::unique_ptr<Gambo> emu = std::make_unique<Gambo>();
	emu->Run();

	return 0;
}