#include "GamboCore.h"
#include "Frontend.h"
#include <fstream>
#include <random>
#include <format>
#include <chrono>
#include <iostream>

GamboCore::GamboCore()
{
}

GamboCore::~GamboCore()
{
}

void GamboCore::Run()
{
	using namespace std::chrono;
	using clock = high_resolution_clock;
	using framerate = duration<int, std::ratio<1, DesiredFPS>>;
	auto timePoint = clock::now() + framerate{1};

	//while (!done)
	{
		//SDL_Event e;
		//while (SDL_PollEvent(&e))
		//{
		//	switch (e.type)
		//	{
		//		case SDL_KEYDOWN:
		//			if (e.key.keysym.sym == SDLK_p)
		//			{
		//				running = !running;
		//			}
		//			else if (e.key.keysym.sym == SDLK_r)
		//			{
		//				gb.cpu.Reset();
		//			}
		//			else if (e.key.keysym.sym == SDLK_SPACE)
		//			{
		//				step = true;
		//			}
		//			break;
		//
		//		case SDL_WINDOWEVENT:
		//			switch (e.window.event)
		//			{
		//				case SDL_WINDOWEVENT_CLOSE:
		//					return;
		//				default:
		//					break;
		//			}
		//			break;
		//
		//		default:
		//			break;
		//	}
		//}

		static bool disassemble = true;
		if (running)
		{
			do
			{
				gb.cpu.Clock();
				//if (gb.cpu.PC == 0xC000)
				//{
				//	running = false;
				//	goto BREAK;
				//}
				gb.ppu.Clock();
			} while (!gb.ppu.FrameComplete());

			disassemble = true;
		}
		else if (step)
		{
			do
			{
				gb.cpu.Clock();
			BREAK:
				gb.ppu.Clock();
			} while (!gb.cpu.InstructionComplete());
			step = false;
			disassemble = true;
		}

		if (disassemble)
		{
			gb.Disassemble(gb.cpu.PC, 10);
			disassemble = false;
		}

		std::this_thread::sleep_until(timePoint - 1ms);
		while (clock::now() <= timePoint)
		{
		}
		timePoint += framerate{1};
	}
}

void* GamboCore::GetScreen() const
{
	return gb.ppu.screen;
}

float GamboCore::GetScreenWidth() const
{
	return screenWidth * screenScale;
}

float GamboCore::GetScreenHeight() const
{
	return screenHeight * screenScale;
}

GamboState GamboCore::GetState() const
{
	GamboState g;
	g.flags.Z = gb.cpu.F & CPU::fZ;
	g.flags.N = gb.cpu.F & CPU::fN;
	g.flags.H = gb.cpu.F & CPU::fH;
	g.flags.C = gb.cpu.F & CPU::fC;
	g.flags.IME = gb.cpu.IME;

	g.registers.A = gb.cpu.A;
	g.registers.F = gb.cpu.F;
	g.registers.B = gb.cpu.B;
	g.registers.C = gb.cpu.C;
	g.registers.D = gb.cpu.D;
	g.registers.E = gb.cpu.E;
	g.registers.H = gb.cpu.H;
	g.registers.L = gb.cpu.L;
	
	g.PC = gb.cpu.PC;
	g.SP = gb.cpu.SP;

	g.LCDC = gb.ram[HWAddr::LCDC];
	g.STAT = gb.ram[HWAddr::STAT];
	g.LY = gb.ram[HWAddr::LY];
	g.IE = gb.ram[HWAddr::IE];
	g.IF = gb.ram[HWAddr::IF];

	g.mapAsm = gb.mapAsm;
	return g;
}

void GamboCore::InsertCartridge(std::wstring filePath)
{
	running = false;

	gb.Reset();
	gb.InsertCartridge(filePath);
}

void GamboCore::SetUseBootRom(bool b)
{
	gb.cpu.useBootRom = b;
}

bool GamboCore::GetUseBootRom()
{
	return gb.cpu.useBootRom;
}

void GamboCore::DrawString(SDL_Color* target, u32 targetWidth, u32 targetHeight, s32 x, s32 y, const std::string& sText, ImVec4 col, u32 scale)
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
					//if (fontSheet[(i + offsetX * 8) + ((j + offsetY * 8) * 128)])
						for (u32 is = 0; is < trueScale; is++)
							for (u32 js = 0; js < trueScale; js++)
							{
								u64 index = ((x * PixelScale) + screenX + (i * trueScale) + is) + (((y * PixelScale) + screenY + (j * trueScale) + js) * targetWidth);
								if (index < targetSize)
								{
									//target[index] = col;
								}
							}

			screenX += 8 * trueScale;
		}
	}
}

void GamboCore::DrawCode(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
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

void GamboCore::DrawStackPointer(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
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

void GamboCore::DrawRamWrites(SDL_Color* target, int targetWidth, int targetHeight, int x, int y, int nLines)
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
