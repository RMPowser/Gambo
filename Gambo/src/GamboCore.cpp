#include "GamboCore.h"
#include "CPU.h"
#include "PPU.h"
#include "RAM.h"
#include "Input.h"
#include "Cartridge.h"
#include "BootRomDMG.h"
#include "VramViewer.h"

#include <fstream>
#include <random>
#include <format>
#include <iostream>

GamboCore::GamboCore()
	: ram(new RAM(this))
	, cpu(new CPU(this))
	, ppu(new PPU(this))
	, input(new Input(this))
	, boot(new BootRomDMG())
	, cart(new Cartridge())
	, vram(new VramViewer(ram))
{
	cart->Reset();
	Reset();
}

GamboCore::~GamboCore()
{
	SAFE_DELETE(cpu);
	SAFE_DELETE(ppu);
	SAFE_DELETE(ram);
	SAFE_DELETE(input);
	SAFE_DELETE(cart);
	SAFE_DELETE(boot);
}

void GamboCore::Run()
{
	if (running)
	{
		bool vblank = false;
		int totalCycles = 0;
		while (!vblank)
		{
			input->Check();
			int cycles = cpu->RunFor(1);
			vblank = ppu->Tick(cycles);

			totalCycles += cycles;
			if (totalCycles > 702240)
				vblank = true;

			//if (cpu->GetPC() == 0x00A4)
			//{
			//	running = false;
			//	break;
			//}
		}
		
		disassemble = true;
	}
	else if (step)
	{
		do
		{
			int cycles = cpu->RunFor(1);
			ppu->Tick(cycles);
		} while (!cpu->IsCurrentInstructionFinished());

		step = false;
		disassemble = true;
	}
	else if (stepFrame)
	{
		bool vblank = false;
		int totalCycles = 0;
		while (!vblank)
		{
			int cycles = cpu->RunFor(1);
			vblank = ppu->Tick(cycles);

			totalCycles += cycles;
			if (totalCycles > 702240)
				vblank = true;

			//if (cpu->GetPC() == 0x00A4)
			//{
			//	running = false;
			//	break;
			//}
		}

		stepFrame = false;
		disassemble = true;
	}
}

const void* GamboCore::GetScreen() const
{
	return (void*)ppu->GetScreen().data();
}

VramViewer& GamboCore::GetVramViewer()
{
	return *vram;
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

	g.LCDC = ram->Get(HWAddr::LCDC);
	g.STAT = ram->Get(HWAddr::STAT);
	g.LY = ram->Get(HWAddr::LY);
	g.IE = ram->Get(HWAddr::IE);
	g.IF = ram->Get(HWAddr::IF);

	if (disassemble)
		g.mapAsm = Disassemble(g.PC, 10);

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

bool GamboCore::GetStepFrame()
{
	return stepFrame;
}

void GamboCore::SetStepFrame(bool b)
{
	stepFrame = b;
}

const Cartridge& GamboCore::GetCartridge() const
{
	return *cart;
}

void GamboCore::InsertCartridge(std::filesystem::path filePath)
{
	Reset();

	cart->Load(filePath);

	// copy the first 2 rom banks of the cartridge data into ram
	for (u16 i = 0; i < 32KiB; i++)
		ram->Set(i, cart->Read(i));
}

void GamboCore::SetUseBootRom(bool b)
{
	useBootRom = b;

	if (!running)
		Reset();
}

bool GamboCore::IsUseBootRom()
{
	return useBootRom;
}

u8 GamboCore::Read(u16 addr)
{
	if (IsBootRomAddress(addr))
	{
		return boot->Read((u8)addr);
	}
	else if (IsCartridgeAddress(addr))
	{
		if (cart->IsLoaded())
			return cart->Read(addr);
		else
			return IsUseBootRom() ? 0xFF : ram->Read(addr);
	}
	else
	{
		return ram->Read(addr);
	}
}

void GamboCore::Write(u16 addr, u8 data)
{
	if (IsBootRomAddress(addr))
	{
		return;
	}
	else if (IsCartridgeAddress(addr))
	{
		if (cart->IsLoaded())
			cart->Write(addr, data);
		else
			return;
	}
	else
	{
		ram->Write(addr, data);
	}
}

void GamboCore::Reset()
{
	done = false;
	running = false;
	step = false;
	cpu->Reset();
	ppu->Reset();
	ram->Reset();
	boot->Reset();

	// resetting the cartridge is akin to removing a game from a physical gameboy
	//cart->Reset();
}

std::map<u16, std::string> GamboCore::Disassemble(u16 startAddr, int numInstr) const
{
	return cpu->Disassemble(startAddr, numInstr);
}

bool GamboCore::IsBootRomAddress(u16 addr)
{
	return
		!(ram->Get(HWAddr::BOOT) & 1) &&	// the boot rom sets this bit when it's finished
		(0x0000 <= addr && addr <= 0x00FF);
}

bool GamboCore::IsCartridgeAddress(u16 addr)
{
	return
		(0x0000 <= addr && addr <= 0x7FFF) ||	// rom
		(0xA000 <= addr && addr <= 0xBFFF) && cart->GetRamSize() > 0;		// ram
}