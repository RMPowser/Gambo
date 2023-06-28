#include "Bus.h"
#include <fstream>
#include <random>
#include <format>
#include <chrono>
#include <iostream>

template <class Clock, class Duration>
void sleep_until(std::chrono::time_point<Clock, Duration> tp)
{
	using namespace std::chrono;
	
}


class Gambo
{
public:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* dmgScreen = nullptr;
	SDL_Texture* windowTexture = nullptr;
	Bus gb;
	bool running = false;

	Gambo()
	{
		SDL_assert_release(SDL_Init(SDL_INIT_EVERYTHING) == 0);

		window = SDL_CreateWindow(WindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * PixelScale, 350 * PixelScale, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
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

		std::ifstream input("C:\\Users\\Ryan\\source\\repos\\gb-test-roms\\instr_timing\\instr_timing.gb", std::ios::binary);

		// copies all data into buffer
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
		uint16_t offset = 0x0000;

		// copy the buffer into ram
		for (auto& byte : buffer)
		{
			gb.ram[offset++] = byte;
		}

		gb.Disassemble(0x0000, 0xFFFF);

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
		using namespace std::chrono;
		using clock = high_resolution_clock;
		using framerate = duration<int, std::ratio<1, 60>>;
		auto timePoint = clock::now() + framerate{1};
		while (true)
		{
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

			static bool disassemble;
			disassemble = false;
			if (running)
			{
				do
				{
					gb.cpu.Clock();
					//if (gb.cpu.PC == 0xC448)
					//{
					//	running = false;
					//	goto BREAK;
					//}
					gb.ppu.Clock(dmgScreen);
				} while (!gb.ppu.FrameComplete());

				disassemble = true;
			}
			else if (step)
			{
				do
				{
					gb.cpu.Clock();
				BREAK:
					gb.ppu.Clock(dmgScreen);
				} while (!gb.cpu.InstructionComplete());
				step = false;
				disassemble = true;
			}

#if !defined(NDEBUG)
			if (disassemble)
			{
				gb.Disassemble(0x0000, 0xFFFF);
			}
#endif

			Render();

			std::this_thread::sleep_until(timePoint - 5ms);
			while (clock::now() <= timePoint) 
			{
			}
			timePoint += framerate{1};
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
			DrawCpu(target, targetWidth, targetHeight, 448, 2);
			DrawCode(target, targetWidth, targetHeight, 448, 82, 10);
			DrawStackPointer(target, targetWidth, targetHeight, 448, 202, 10);
			DrawRamWrites(target, targetWidth, targetHeight, 548, 202, 10);

			DrawString(target, targetWidth, targetHeight, 10, 332, "SPACE = Step Instruction    R = RESET    P = PLAY");
		}
		SDL_UnlockTexture(windowTexture);


		SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
		//SDL_RenderClear(renderer);
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
		DrawString(target, targetWidth, targetHeight, x + 64, y, "N", gb.cpu.F & CPU::fN ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x + 72, y, "H", gb.cpu.F & CPU::fH ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x + 80, y, "C", gb.cpu.F & CPU::fC ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x + 100, y, "IME", gb.cpu.IME ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x, y + 10, "AF: $" + hex(gb.cpu.AF, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 20, "BC: $" + hex(gb.cpu.BC, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 30, "DE: $" + hex(gb.cpu.DE, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 40, "HL: $" + hex(gb.cpu.HL, 4));
		DrawString(target, targetWidth, targetHeight, x, y + 50, "SP: $" + hex(gb.cpu.SP, 4), GREEN);
		DrawString(target, targetWidth, targetHeight, x, y + 60, "PC: $" + hex(gb.cpu.PC, 4), CYAN);
		DrawString(target, targetWidth, targetHeight, x + 100, y + 10, "LCDC: $" + hex(gb.ram[HWAddr::LCDC], 2));
		DrawString(target, targetWidth, targetHeight, x + 100, y + 20, "STAT: $" + hex(gb.ram[HWAddr::STAT], 2));
		DrawString(target, targetWidth, targetHeight, x + 100, y + 30, "LY:   $" + hex(gb.ram[HWAddr::LY], 2));
		DrawString(target, targetWidth, targetHeight, x + 100, y + 50, "IE:   $" + hex(gb.ram[HWAddr::IE], 2));
		DrawString(target, targetWidth, targetHeight, x + 100, y + 60, "IF:   $" + hex(gb.ram[HWAddr::IF], 2));
	}

	void DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
	{
		auto it_a = gb.mapAsm.find(gb.cpu.PC);
		SDL_assert(it_a != gb.mapAsm.end());

		int nLineY = (nLines >> 1) * 10 + y;
		if (it_a != gb.mapAsm.end())
		{
			DrawString(target, targetWidth, targetHeight, x, nLineY, (*it_a).second, CYAN);
			while (nLineY < (nLines * 10) + y)
			{
				nLineY += 10;
				if (++it_a != gb.mapAsm.end())
				{
					DrawString(target, targetWidth, targetHeight, x, nLineY, (*it_a).second);
				}
			}
		}

		it_a = gb.mapAsm.find(gb.cpu.PC);
		nLineY = (nLines >> 1) * 10 + y;
		if (it_a != gb.mapAsm.end())
		{
			while (nLineY > y)
			{
				nLineY -= 10;
				if (--it_a != gb.mapAsm.end())
				{
					DrawString(target, targetWidth, targetHeight, x, nLineY, (*it_a).second);
				}
			}
		}
	}

	void DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
	{
		u16 addr = gb.cpu.SP;
		int nLineY = (nLines >> 1) * 10 + y;
		auto data0 = hex(addr, 4);
		auto data1 = hex(gb.ram[addr], 2);
		auto s = std::format("${}: {}", data0, data1);
		DrawString(target, targetWidth, targetHeight, x, nLineY, s, GREEN);
		while (nLineY < (nLines * 10) + y)
		{
			nLineY += 10;
			addr += 1;
			data0 = hex(addr, 4);
			data1 = hex(gb.ram[addr], 2);
			s = std::format("${}: {}", data0, data1);
			DrawString(target, targetWidth, targetHeight, x, nLineY, s);
		}

		addr = gb.cpu.SP;
		nLineY = (nLines >> 1) * 10 + y;
		while (nLineY > y)
		{
			nLineY -= 10;
			addr -= 1;
			data0 = hex(addr, 4);
			data1 = hex(gb.ram[addr], 2);
			s = std::format("${}: {}", data0, data1);
			DrawString(target, targetWidth, targetHeight, x, nLineY, s);
		}
	}

	void DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
	{
		u16 addr = gb.lastWrite;
		int nLineY = (nLines >> 1) * 10 + y;
		DrawString(target, targetWidth, targetHeight, x, nLineY, std::format("${}: {}", hex(addr, 4), hex(gb.ram[addr], 2)), RED);
		while (nLineY < (nLines * 10) + y)
		{
			nLineY += 10;
			addr++;
			DrawString(target, targetWidth, targetHeight, x, nLineY, std::format("${}: {}", hex(addr, 4), hex(gb.ram[addr], 2)));
		}

		addr = gb.lastWrite;
		nLineY = (nLines >> 1) * 10 + y;
		while (nLineY > y)
		{
			nLineY -= 10;
			addr--;
			DrawString(target, targetWidth, targetHeight, x, nLineY, std::format("${}: {}", hex(addr, 4), hex(gb.ram[addr], 2)));
		}
	}
};

int main(int argc, char* argv[])
{
	auto emu = std::make_unique<Gambo>();
	emu->Run();

	return 0;
}