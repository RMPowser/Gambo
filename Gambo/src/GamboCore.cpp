#include "GamboCore.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

#include <fstream>
#include <random>
#include <format>
#include <chrono>
#include <iostream>

GamboCore::GamboCore()
	: cpu(new CPU(this))
	, ppu(new PPU(this))
	, cart(nullptr)
{
}

GamboCore::~GamboCore()
{
	SAFE_DELETE(cpu);
	SAFE_DELETE(ppu);
	SAFE_DELETE(cart);
}

void GamboCore::Run()
{
	using namespace std::chrono;
	using clock = high_resolution_clock;
	using framerate = duration<int, std::ratio<1, DesiredFPS>>;
	auto timePoint = clock::now() + framerate{1};

	
	disassemble = true;
	if (running)
	{
		do
		{
			cpu->Clock();
			//if (cpu->PC == 0xC000)
			//{
			//	running = false;
			//	goto BREAK;
			//}
			ppu->Clock();
		} while (!ppu->FrameComplete());

		disassemble = true;
	}
	else if (step)
	{
		do
		{
			cpu->Clock();
		BREAK:
			ppu->Clock();
		} while (!cpu->InstructionComplete());
		step = false;
		disassemble = true;
	}

	if (disassemble)
	{
		Disassemble(cpu->PC, 10);
		disassemble = false;
	}

	std::this_thread::sleep_until(timePoint - 1ms);
	while (clock::now() <= timePoint)
	{
		// do nothing.
	}
	timePoint += framerate{1};
}

void* GamboCore::GetScreen() const
{
	return ppu->screen;
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
	g.flags.Z = cpu->F & (u8)CPUFlags::Z;
	g.flags.N = cpu->F & (u8)CPUFlags::N;
	g.flags.H = cpu->F & (u8)CPUFlags::H;
	g.flags.C = cpu->F & (u8)CPUFlags::C;
	g.flags.IME = cpu->IME;

	g.registers.A = cpu->A;
	g.registers.F = cpu->F;
	g.registers.B = cpu->B;
	g.registers.C = cpu->C;
	g.registers.D = cpu->D;
	g.registers.E = cpu->E;
	g.registers.H = cpu->H;
	g.registers.L = cpu->L;
	
	g.PC = cpu->PC;
	g.SP = cpu->SP;

	g.LCDC = ram[HWAddr::LCDC];
	g.STAT = ram[HWAddr::STAT];
	g.LY = ram[HWAddr::LY];
	g.IE = ram[HWAddr::IE];
	g.IF = ram[HWAddr::IF];

	g.mapAsm = mapAsm;
	return g;
}

const Cartridge& GamboCore::GetCartridge() const
{
	return *cart;
}

void GamboCore::InsertCartridge(std::filesystem::path filePath)
{
	running = false;

	Reset();

	SAFE_DELETE(cart);
	cart = new Cartridge(filePath);

	// copy the first 2 rom banks of the cartridge data into ram
	for (u16 i = 0; i < 32KiB; i++)
	{
		ram[i] = cart->Read(i);
	}
}

void GamboCore::SetUseBootRom(bool b)
{
	cpu->useBootRom = b;
}

bool GamboCore::GetUseBootRom()
{
	return cpu->useBootRom;
}

u8 GamboCore::Read(u16 addr)
{
	lastRead = addr;

	// boot rom
	if (IsBootRomAddress(addr))
	{
		return cpu->bootRom[addr];
	}
	// cartridge
	else if (IsCartridgeAddress(addr))
	{
		if (cart != nullptr)
		{
			return cart->Read(addr);
		}
		else
		{
			return cpu->useBootRom ? 0xFF : ram[addr];
		}
	}
	// normal ram
	else
	{
		return ram[addr];
	}
}

void GamboCore::Write(u16 addr, u8 data)
{
	// TODO: remove this after implementing input
	if (addr == HWAddr::P1)
	{
		return;
	}

	lastWrite = addr;

	// cartridge
	if (IsCartridgeAddress(addr) && cart != nullptr)
	{
		cart->Write(addr, data);
	}
	// normal ram
	else
	{
		ram[addr] = data;

		// this is the implementation for echo ram
		if (addr >= 0xE000 && addr <= 0xFDFF)
		{
			ram[addr - 0x2000] = data;
		}
		else if (addr >= 0xC000 && addr <= 0xDDFF)
		{
			ram[addr + 0x2000] = data;
		}

		// top 3 bits in IF are always read as 1
		if (addr == HWAddr::IF)
		{
			ram[HWAddr::IF] |= 0b11100000;
		}

		// writing anything to the DIV register resets it to 0
		if (addr == HWAddr::DIV)
		{
			ram[addr] = 0;
		}
	}
}

void GamboCore::Reset()
{
	ram = std::vector<u8>(64KiB, 0x00);
	mapAsm.clear();
	lastWrite = 0;
	lastRead = 0;
	cpu->Reset();
	ppu->Reset();
	SAFE_DELETE(cart);
}

void GamboCore::Disassemble(u16 startAddr, int numInstr)
{
	u32 addr = startAddr;
	u8 value = 0x00, lo = 0x00, hi = 0x00;
	u16 lineAddr = 0;

	mapAsm.clear();

	while (mapAsm.size() < numInstr)
	{

		//if ((0x4000 <= addr && addr <= 0xBFFF) || // skip vram
		//	(0x0104 <= addr && addr <= 0x014F) || // skip cartridge header
		//	(0xFE00 <= addr && addr <= 0xFE7F) || // skip OAM and IO
		//	(addr == 0xD800)) // skip this address in particular because if i dont, it breaks disassembly
		//{
		//	addr++;
		//	continue;
		//}

		lineAddr = addr;

		// prefix line with instruction addr
		std::string s = "$" + hex(addr, 4) + ": ";

		// read instruction and get readable name
		u8 opcode = Read(addr++);
		if (opcode == 0xCB)
		{
			// its a 16bit opcode so read another byte
			opcode = Read(addr++);

			s += cpu->instructions16bit[opcode].mnemonic;
		}
		else
		{
			auto& instruction = cpu->instructions8bit[opcode];
			switch (instruction.bytes)
			{
				case 0:
				case 1:
				{
					s += cpu->instructions8bit[opcode].mnemonic;
					break;
				}
				case 2:
				{
					u8 data = Read(addr++);
					std::string firstTwoChar(instruction.mnemonic.begin(), instruction.mnemonic.begin() + 2);
					if (firstTwoChar == "JR")
					{
						s16 sdata = (s8)data;
						sdata += addr;
						s += std::vformat(instruction.mnemonic, std::make_format_args(hex(sdata, 4)));
						break;
					}
					s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 2)));
					break;
				}
				case 3:
				{
					u16 lo = Read(addr++);
					u16 hi = Read(addr++);
					u16 data = (hi << 8) | lo;
					s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 4)));
					break;
				}
				default:
					throw("opcode has more than 3 bytes");
			}
		}

		mapAsm[lineAddr] = s;
	}
}

bool GamboCore::IsBootRomAddress(u16 addr)
{
	return
		!(ram[HWAddr::BOOT] & 1) &&
		(0x0000 <= addr && addr <= 0x00FF);
}

bool GamboCore::IsCartridgeAddress(u16 addr)
{
	if (cart == nullptr)
	{
		return false;
	}
	else
	{
		return
			!IsBootRomAddress(addr) &&
			(0x0000 <= addr && addr <= 0x7FFF && addr < cart->GetRomSize()) ||	// rom
			(0xA000 <= addr && addr <= 0xBFFF && addr < cart->GetRamSize());	// ram
	}
}
