#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <SDL2/SDL.h>
#include "Bus.h"
#include <assert.h>
#include <iostream>

constexpr auto DMGScreenWidth	= 640;
constexpr auto DMGScreenHeight	= 480;
constexpr auto PixelWidth		= 2;
constexpr auto PixelHeight		= 2;
constexpr auto FullScreen		= false;
constexpr auto Vsync			= false;
constexpr auto Cohesion			= false;

std::string hex(uint32_t n, uint8_t d)
{
	std::string s(d, '0');
	for (int i = d - 1; i >= 0; i--, n >>= 4)
		s[i] = "0123456789ABCDEF"[n & 0xF];
	return s;
};

class Gambo : public olc::PixelGameEngine
{
public:
	Bus gb;
	bool running = false;

	std::map<uint16_t, std::string> mapAsm;

	Gambo()
	{
		sAppName = "Gambo";
	}

	bool OnUserCreate() override
	{

		std::ifstream input("C:\\Users\\Ryan\\source\\repos\\Gambo\\Gambo\\test roms\\cpu_instrs.gb", std::ios::binary);

		// copies all data into buffer
		std::vector<uint8_t> buffer(std::istreambuf_iterator<char>(input), {});
		uint16_t offset = 0x0000;

		for (auto& byte : buffer)
		{
			gb.ram[offset++] = byte;
		}

		mapAsm = gb.cpu.Disassemble(0x0000, 0xFFFF);

		gb.cpu.Reset();
		return true;
	}

	bool OnUserUpdate(float deltaTime) override
	{
		static olc::Sprite* drawTarget = GetDrawTarget();
		Clear(olc::BLACK);

		if (GetKey(olc::Key::P).bPressed)
		{
			running = !running;
		}

		static int cycles;
		if (running || (!running && GetKey(olc::Key::SPACE).bPressed))
		{
			cycles = gb.cpu.Clock();
			for (size_t i = 0; i < cycles; i++)
			{
				gb.ppu.Clock(drawTarget);
			}
		}

		if (GetKey(olc::Key::R).bPressed)
			gb.cpu.Reset();

		// Draw Ram Page 0x00		
		//DrawRam(2, 2, 0x0000, 16, 16);
		//DrawRam(2, 182, 0x0100, 16, 16);
		DrawCpu(448, 2);
		DrawCode(448, 112, 22);

		DrawString(10, 370, "SPACE = Step Instruction    R = RESET    P = PLAY");


		return true;
	}


	void DrawRam(int x, int y, uint16_t nAddr, int nRows, int nColumns)
	{
		int nRamX = x, nRamY = y;
		for (int row = 0; row < nRows; row++)
		{
			std::string sOffset = "$" + hex(nAddr, 4) + ":";
			for (int col = 0; col < nColumns; col++)
			{
				sOffset += " " + hex(gb.Read(nAddr), 2);
				nAddr += 1;
			}
			DrawString(nRamX, nRamY, sOffset);
			nRamY += 10;
		}
	}

	void DrawCpu(int x, int y)
	{
		DrawString(x, y, "FLAGS:", olc::WHITE);
		DrawString(x + 56, y, "Z", gb.cpu.F & CPU::fZ ? olc::GREEN : olc::RED);
		DrawString(x + 72, y, "N", gb.cpu.F & CPU::fN ? olc::GREEN : olc::RED);
		DrawString(x + 88, y, "H", gb.cpu.F & CPU::fH ? olc::GREEN : olc::RED);
		DrawString(x + 104, y, "C", gb.cpu.F & CPU::fC ? olc::GREEN : olc::RED);
		DrawString(x, y + 10, "PC: $" + hex(gb.cpu.PC, 4));
		DrawString(x, y + 20, "AF: $" + hex(gb.cpu.AF, 4));
		DrawString(x, y + 30, "BC: $" + hex(gb.cpu.BC, 4));
		DrawString(x, y + 40, "DE: $" + hex(gb.cpu.DE, 4));
		DrawString(x, y + 50, "HL: $" + hex(gb.cpu.HL, 4));
		DrawString(x, y + 60, "SP: $" + hex(gb.cpu.SP, 4));
	}

	void DrawCode(int x, int y, int nLines)
	{
		auto it_a = mapAsm.find(gb.cpu.PC);
		assert(it_a != mapAsm.end());

		int nLineY = (nLines >> 1) * 10 + y;
		if (it_a != mapAsm.end())
		{
			DrawString(x, nLineY, (*it_a).second, olc::CYAN);
			while (nLineY < (nLines * 10) + y)
			{
				nLineY += 10;
				if (++it_a != mapAsm.end())
				{
					DrawString(x, nLineY, (*it_a).second);
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
					DrawString(x, nLineY, (*it_a).second);
				}
			}
		}
	}

private:
};

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Quit();

	//Gambo* emu = new Gambo;
	//if (emu->Construct(DMGScreenWidth, DMGScreenHeight, PixelWidth, PixelHeight, FullScreen, Vsync, Cohesion))
	//{
	//	emu->Start();
	//}

	return 0;
}