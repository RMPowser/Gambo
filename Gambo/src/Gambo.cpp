#include "Bus.h"
#include <fstream>
#include <random>

class Gambo
{
public:
	SDL_Window* window = nullptr;
	Bus gb;

	bool running = false;

	std::map<uint16_t, std::string> mapAsm;

	Gambo()
	{
		SDL_assert(SDL_Init(SDL_INIT_EVERYTHING) == 0);

		window = SDL_CreateWindow(WindowTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640 * PixelScale, 540 * PixelScale, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
		SDL_assert(window);

		SDL_assert(SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED));

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
	}

	~Gambo()
	{
		if (window != nullptr)
		{
			SDL_DestroyWindow(window);
		}

		SDL_Quit();
	}

	void Run()
	{
		while (true)
		{
			static int cycles;
			bool step = false;

			static SDL_Renderer* renderer = nullptr;
			renderer = SDL_GetRenderer(window);
			SDL_assert(renderer);

			static SDL_Texture* dmgScreen = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, DMGScreenWidth, DMGScreenHeight);
			SDL_assert(dmgScreen);

			int windowWidth, windowHeight;
			SDL_GetWindowSize(window, &windowWidth, &windowHeight);
			static SDL_Texture* windowTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, windowWidth, windowHeight);
			SDL_assert(windowTexture);

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

			static SDL_Color* target = nullptr;
			static int rowByteLength = 0;

			if (running || (!running && step))
			{
				cycles = gb.cpu.Clock();

				SDL_LockTexture(dmgScreen, NULL, (void**) &target, &rowByteLength);
				{
					for (size_t i = 0; i < cycles; i++)
					{
						gb.ppu.Clock(target);
					}

					static std::random_device rd;
					static std::mt19937 gen(rd());
					static std::uniform_int_distribution<> dist(0, 255);
					for (size_t row = 0; row < DMGScreenWidth; row++)
					{
						for (size_t col = 0; col < DMGScreenHeight; col++)
						{
							int value = dist(gen);
							target[row + (col * DMGScreenWidth)].r = value;
							target[row + (col * DMGScreenWidth)].g = value;
							target[row + (col * DMGScreenWidth)].b = value;
							target[row + (col * DMGScreenWidth)].a = 255;
						}
					}
				}
				SDL_UnlockTexture(dmgScreen);
			}

			u64 targetSize = 0;
			int targetWidth;
			int targetHeight;
			SDL_QueryTexture(windowTexture, NULL, NULL, &targetWidth, &targetHeight);

			SDL_LockTexture(windowTexture, NULL, (void**)&target, &rowByteLength);
			{
				SDL_memset(target, 0x00, targetWidth * targetHeight * 4);
				// Draw Ram Page 0x00
				//DrawRam(2, 2, 0x0000, 16, 16);
				//DrawRam(2, 182, 0x0100, 16, 16);
				DrawCpu(target, targetWidth, targetHeight, 448, 2);
				DrawCode(target, targetWidth, targetHeight, 448, 112, 22);

				DrawString(target, targetWidth, targetHeight, 10, 500, "SPACE = Step Instruction    R = RESET    P = PLAY", WHITE);
			}
			SDL_UnlockTexture(windowTexture);
			

			SDL_SetRenderDrawColor(renderer, 32, 32, 32, 255);
			SDL_RenderClear(renderer);
			SDL_RenderCopy(renderer, windowTexture, NULL, NULL);
			static SDL_Rect destRect = { 0, 0, DMGScreenWidth * PixelScale * 2, DMGScreenHeight * PixelScale * 2 };
			SDL_RenderCopy(renderer, dmgScreen, NULL, &destRect);
			SDL_RenderPresent(renderer);
		}
	}

	void DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, SDL_Color col = WHITE, u32 scale = 1)
	{
		static u64 targetSize;
		static u32 trueScale;
		targetSize = (u64)targetWidth* (u64)targetHeight;
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

	void DrawRam(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, uint16_t nAddr, int nRows, int nColumns)
	{
		//int nRamX = x, nRamY = y;
		//for (int row = 0; row < nRows; row++)
		//{
		//	std::string sOffset = "$" + hex(nAddr, 4) + ":";
		//	for (int col = 0; col < nColumns; col++)
		//	{
		//		sOffset += " " + hex(gb.Read(nAddr), 2);
		//		nAddr += 1;
		//	}
		//	DrawString(nRamX, nRamY, sOffset);
		//	nRamY += 10;
		//}
	}

	void DrawCpu(SDL_Color* target, int targetWidth, int targetHeight, int x, int y)
	{
		DrawString(target, targetWidth, targetHeight, x, y, "FLAGS:");
		DrawString(target, targetWidth, targetHeight, x +  56, y, "Z", gb.cpu.F & CPU::fZ ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x +  72, y, "N", gb.cpu.F & CPU::fN ? GREEN : RED);
		DrawString(target, targetWidth, targetHeight, x +  88, y, "H", gb.cpu.F & CPU::fH ? GREEN : RED);
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

private:
};

int main(int argc, char* argv[])
{
	std::unique_ptr<Gambo> emu = std::make_unique<Gambo>();
	emu->Run();

	return 0;
}