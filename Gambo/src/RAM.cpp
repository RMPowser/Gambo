#include "RAM.h"
#include "GamboCore.h"
#include "PPU.h"
#include <random>

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

#pragma warning(push)
#pragma warning(disable: 26451)

RAM::RAM(GamboCore* c)
	: core(c)
{
}

RAM::~RAM()
{
}

const u8 RAM::Read(u16 addr) const
{
	return ram[addr];
}

void RAM::Write(u16 addr, u8 data)
{
	// TODO: remove this after implementing input
	if (addr == HWAddr::P1)
	{
		return;
	}

	// these addresses are read only
	if ((0x0000 <= addr && addr <= 0x7FFF) ||
		(0xFEA0 <= addr && addr <= 0xFEFF))
	{
		return;
	}

	if (addr == HWAddr::LCDC)
	{
		u8 curr = ram[addr];

		// if the window will be enabled from being disabled, reset its line
		if (!GetBits(curr, (u8)LCDCBits::WindowEnable, 0b1) && GetBits(data, (u8)LCDCBits::WindowEnable, 0b1))
			core->ppu->ResetWindowLine();

		if (GetBits(data, (u8)LCDCBits::LCDEnable, 0b1) && !GetBits(curr, (u8)LCDCBits::LCDEnable, 0b1))
			core->ppu->Enable();
		else if (!GetBits(data, (u8)LCDCBits::LCDEnable, 0b1) && GetBits(curr, (u8)LCDCBits::LCDEnable, 0b1))
			core->ppu->Disable();
	}

	ram[addr] = data;

	// this is the implementation for echo ram
	if (addr >= 0xE000 && addr <= 0xFDFF)
		ram[addr - 0x2000] = data;
	else if (addr >= 0xC000 && addr <= 0xDDFF)
		ram[addr + 0x2000] = data;

	// top 3 bits in IF are always read as 1
	if (addr == HWAddr::IF)
		ram[HWAddr::IF] |= 0b11100000;

	// writing anything to the DIV register resets it to 0
	if (addr == HWAddr::DIV)
		ram[addr] = 0;
}

u8& RAM::Get(u16 addr)
{
	return ram[addr];
}

void RAM::Set(u16 addr, u8 data)
{
	ram[addr] = data;
}

void RAM::Reset()
{
	ram.fill(0x00);

	// fill vram with 0xFF
	for (size_t i = 0x8000; i < 0xC000; i++)
		ram[i] = 0xFF;

	// fill WRAM with random bullshit
	std::random_device rd;
	for (size_t i = 0xC000; i < 0xE000; i++)
		ram[i] = rd() % 0x100;
	
	// fill IO/control registers with 0xFF
	for (size_t i = 0xFF00; i < 0x10000; i++)
		ram[i] = 0xFF;
	
	if (core->IsUseBootRom())
	{
		ram[HWAddr::LCDC]	= 0x00;
		ram[HWAddr::STAT]	= 0x00;
		ram[HWAddr::BOOT]	= 0xFE;
		ram[HWAddr::P1]		= 0x0F;
	}
	else
	{
		ram[HWAddr::P1]		= 0xCF;
		ram[HWAddr::SB]		= 0x00;
		ram[HWAddr::SC]		= 0x7E;
		ram[HWAddr::DIV]	= 0xAC;
		ram[HWAddr::TIMA]	= 0x00;
		ram[HWAddr::TMA]	= 0x00;
		ram[HWAddr::TAC]	= 0xF8;
		ram[HWAddr::IF]		= 0xE1;
		ram[HWAddr::NR10]	= 0x80;
		ram[HWAddr::NR11]	= 0xBF;
		ram[HWAddr::NR12]	= 0xF3;
		ram[HWAddr::NR13]	= 0xFF;
		ram[HWAddr::NR14]	= 0xBF;
		ram[HWAddr::NR21]	= 0x3F;
		ram[HWAddr::NR22]	= 0x00;
		ram[HWAddr::NR23]	= 0xFF;
		ram[HWAddr::NR24]	= 0xBF;
		ram[HWAddr::NR30]	= 0x7F;
		ram[HWAddr::NR31]	= 0xFF;
		ram[HWAddr::NR32]	= 0x9F;
		ram[HWAddr::NR33]	= 0xFF;
		ram[HWAddr::NR34]	= 0xBF;
		ram[HWAddr::NR41]	= 0xFF;
		ram[HWAddr::NR42]	= 0x00;
		ram[HWAddr::NR43]	= 0x00;
		ram[HWAddr::NR44]	= 0xBF;
		ram[HWAddr::NR50]	= 0x77;
		ram[HWAddr::NR51]	= 0xF3;
		ram[HWAddr::NR52]	= 0xF1;
		ram[HWAddr::LCDC]	= 0x91;
		ram[HWAddr::STAT]	= 0x80;
		ram[HWAddr::SCY]	= 0x00;
		ram[HWAddr::SCX]	= 0x00;
		ram[HWAddr::LY]		= 0x00;
		ram[HWAddr::LYC]	= 0x00;
		ram[HWAddr::DMA]	= 0xFF;
		ram[HWAddr::BGP]	= 0xFC;
		ram[HWAddr::OBP0]	= 0x00;
		ram[HWAddr::OBP1]	= 0x00;
		ram[HWAddr::WY]		= 0x00;
		ram[HWAddr::WX]		= 0x00;
		ram[HWAddr::KEY1]	= 0xFF;
		ram[HWAddr::VBK]	= 0xFF;
		ram[HWAddr::BOOT]	= 0xFF;
		ram[HWAddr::HDMA1]	= 0xFF;
		ram[HWAddr::HDMA2]	= 0xFF;
		ram[HWAddr::HDMA3]	= 0xFF;
		ram[HWAddr::HDMA4]	= 0xFF;
		ram[HWAddr::HDMA5]	= 0xFF;
		ram[HWAddr::RP]		= 0xFF;
		ram[HWAddr::BCPS]	= 0xFF;
		ram[HWAddr::BCPD]	= 0xFF;
		ram[HWAddr::OCPS]	= 0xFF;
		ram[HWAddr::OCPD]	= 0xFF;
		ram[HWAddr::SVBK]	= 0xFF;
		ram[HWAddr::IE]		= 0x00;
	}
}

#pragma warning(pop)