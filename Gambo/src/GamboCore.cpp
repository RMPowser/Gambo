#include "GamboCore.h"
#include "CPU.h"
#include "PPU.h"
#include "Cartridge.h"

#include <fstream>
#include <random>
#include <format>
#include <chrono>
#include <iostream>

const std::array<u8, 256> bootRom = // this is a regular DMG boot rom. not DMG0.
{
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
	0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
	0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
	0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
	0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
	0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xE2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
	0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3C, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50,
};

GamboCore::GamboCore()
	: cpu(new CPU(this))
	, ppu(new PPU(this))
	, cart(nullptr)
	, useBootRom(false)
	, screenWidth(GamboScreenWidth)
	, screenHeight(GamboScreenHeight)
	, screenScale(PixelScale)
	, disassemble(true)
	, lastRead(0)
	, lastWrite(0)
	, done(false)
	, running(false)
	, step(false)
{
	ram = std::vector<u8>(64KiB, 0x00);
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

	if (running || step)
	{
		bool vblank = false;
		int totalCycles = 0;
		while (!vblank)
		{
			int cycles = cpu->RunFor(1);
			//if (cpu->PC == 0xC000)
			//{
			//	running = false;
			//	goto BREAK;
			//}
			vblank = ppu->Tick(cycles);

			totalCycles += cycles;
			if (totalCycles > 702240)
				vblank = true;
		}

		step = false;
		disassemble = true;
	}
	else if (step)
	{
		int cycles = cpu->RunFor(1);
		BREAK:
		ppu->Tick(cycles);
		
		step = false;
		disassemble = true;
	}

	std::this_thread::sleep_until(timePoint - 1ms);
	while (clock::now() <= timePoint)
	{
		// do nothing.
	}
	timePoint += framerate{1};
}

const void* GamboCore::GetScreen() const
{
	return (void*)ppu->GetScreen().data();
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
	g.flags.Z = cpu->GetFlag(CPUFlags::Z);
	g.flags.N = cpu->GetFlag(CPUFlags::N);
	g.flags.H = cpu->GetFlag(CPUFlags::H);
	g.flags.C = cpu->GetFlag(CPUFlags::C);
	g.flags.IME = cpu->GetIME();

	g.registers.A = cpu->GetA();
	g.registers.F = cpu->GetF();
	g.registers.B = cpu->GetB();
	g.registers.C = cpu->GetC();
	g.registers.D = cpu->GetD();
	g.registers.E = cpu->GetE();
	g.registers.H = cpu->GetH();
	g.registers.L = cpu->GetL();
	
	g.PC = cpu->GetPC();
	g.SP = cpu->GetSP();

	g.LCDC = ram[HWAddr::LCDC];
	g.STAT = ram[HWAddr::STAT];
	g.LY = ram[HWAddr::LY];
	g.IE = ram[HWAddr::IE];
	g.IF = ram[HWAddr::IF];

	if (disassemble)
	{
		g.mapAsm = Disassemble(g.PC, 10);
	}

	return g;
}

bool GamboCore::GetDone()
{
	return done;
}

void GamboCore::SetDone(bool b)
{
	done = b;
}

bool GamboCore::GetRunning()
{
	return running;
}

void GamboCore::SetRunning(bool b)
{
	running = b;
}

bool GamboCore::GetStep()
{
	return step;
}

void GamboCore::SetStep(bool b)
{
	step = b;
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
	useBootRom = b;
}

bool GamboCore::IsUseBootRom()
{
	return useBootRom;
}

u8 GamboCore::Read(u16 addr)
{
	lastRead = addr;

	// boot rom
	if (IsBootRomAddress(addr))
	{
		return bootRom[addr];
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
			return useBootRom ? 0xFF : ram[addr];
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
		if (addr == HWAddr::LCDC)
		{
			u8 curr = ram[addr];

			if (!GetBits(curr, (u8)LCDCBits::WindowEnable, 0b1) && GetBits(data, (u8)LCDCBits::WindowEnable, 0b1))
				ppu->ResetWindowLine();

			if (GetBits(data, (u8)LCDCBits::LCDEnable, 0b1))
				ppu->Enable();
			else
				ppu->Disable();
		}

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
	lastWrite = 0;
	lastRead = 0;
	cpu->Reset();
	ppu->Reset();
	ResetRam();
	SAFE_DELETE(cart);
}

std::map<u16, std::string> GamboCore::Disassemble(u16 startAddr, int numInstr) const
{
	return cpu->Disassemble(startAddr, numInstr);
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

void GamboCore::ResetRam()
{
	if (IsUseBootRom())
	{
		ram[HWAddr::BOOT] = 0xFE;
		ram[HWAddr::P1] = 0x0F;
	}
	else
	{
		for (size_t i = 0xFF00; i < 0x10000; i++)
			ram[i] = 0xFF;

		ram[HWAddr::P1] = 0xCF;
		ram[HWAddr::SB] = 0x00;
		ram[HWAddr::SC] = 0x7E;
		ram[HWAddr::DIV] = 0xAC;
		ram[HWAddr::TIMA] = 0x00;
		ram[HWAddr::TMA] = 0x00;
		ram[HWAddr::TAC] = 0xF8;
		ram[HWAddr::IF] = 0xE1;
		ram[HWAddr::NR10] = 0x80;
		ram[HWAddr::NR11] = 0xBF;
		ram[HWAddr::NR12] = 0xF3;
		ram[HWAddr::NR13] = 0xFF;
		ram[HWAddr::NR14] = 0xBF;
		ram[HWAddr::NR21] = 0x3F;
		ram[HWAddr::NR22] = 0x00;
		ram[HWAddr::NR23] = 0xFF;
		ram[HWAddr::NR24] = 0xBF;
		ram[HWAddr::NR30] = 0x7F;
		ram[HWAddr::NR31] = 0xFF;
		ram[HWAddr::NR32] = 0x9F;
		ram[HWAddr::NR33] = 0xFF;
		ram[HWAddr::NR34] = 0xBF;
		ram[HWAddr::NR41] = 0xFF;
		ram[HWAddr::NR42] = 0x00;
		ram[HWAddr::NR43] = 0x00;
		ram[HWAddr::NR44] = 0xBF;
		ram[HWAddr::NR50] = 0x77;
		ram[HWAddr::NR51] = 0xF3;
		ram[HWAddr::NR52] = 0xF1;
		ram[HWAddr::LCDC] = 0x91;
		ram[HWAddr::STAT] = 0x80;
		ram[HWAddr::SCY] = 0x00;
		ram[HWAddr::SCX] = 0x00;
		ram[HWAddr::LY] = 0x00;
		ram[HWAddr::LYC] = 0x00;
		ram[HWAddr::DMA] = 0xFF;
		ram[HWAddr::BGP] = 0xFC;
		ram[HWAddr::OBP0] = 0x00;
		ram[HWAddr::OBP1] = 0x00;
		ram[HWAddr::WY] = 0x00;
		ram[HWAddr::WX] = 0x00;
		ram[HWAddr::KEY1] = 0xFF;
		ram[HWAddr::VBK] = 0xFF;
		ram[HWAddr::BOOT] = 0xFF;
		ram[HWAddr::HDMA1] = 0xFF;
		ram[HWAddr::HDMA2] = 0xFF;
		ram[HWAddr::HDMA3] = 0xFF;
		ram[HWAddr::HDMA4] = 0xFF;
		ram[HWAddr::HDMA5] = 0xFF;
		ram[HWAddr::RP] = 0xFF;
		ram[HWAddr::BCPS] = 0xFF;
		ram[HWAddr::BCPD] = 0xFF;
		ram[HWAddr::OCPS] = 0xFF;
		ram[HWAddr::OCPD] = 0xFF;
		ram[HWAddr::SVBK] = 0xFF;
		ram[HWAddr::IE] = 0x00;
	}
}
