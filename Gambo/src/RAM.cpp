#include "RAM.h"
#include "GamboCore.h"
#include "PPU.h"
#include <random>

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
	// these addresses are read only
	if ((0x0000 <= addr && addr <= 0x7FFF) ||
		(0xFEA0 <= addr && addr <= 0xFEFF))
	{
		return;
	}

	// the bottom half of this register is read only, and bits 6 and 7 are unused
	if (addr == HWAddr::P1)
	{
		auto& P1 = ram[addr];
		
		u8 lowBits = P1 & 0b11001111;
		P1 = data;
		P1 |= lowBits;
		return;
	}

	if (addr == HWAddr::LCDC)
	{
		u8 curr = ram[addr];

		if (GetBits(data, (u8)LCDCBits::LCDEnable, 0b1) && !GetBits(curr, (u8)LCDCBits::LCDEnable, 0b1))
			core->ppu->Enable();
		else if (!GetBits(data, (u8)LCDCBits::LCDEnable, 0b1) && GetBits(curr, (u8)LCDCBits::LCDEnable, 0b1))
			core->ppu->Disable();
	}

	ram[addr] = data;
	
	if (addr == HWAddr::DMA)
	{
		// do oam dma transfer
		u16 startAddr = data << 8;
		for (u16 currAddr = startAddr; currAddr < startAddr + 160; currAddr++)
		{
			u8 dataToCopy = Read(currAddr);
			ram[HWAddr::OAM + (currAddr - startAddr)] = dataToCopy;
		}
	}

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

const u8& RAM::Get(u16 addr)
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

	// fill WRAM with random garbage
	std::random_device rd;
	for (size_t i = 0xC000; i < 0xE000; i++)
		ram[i] = rd() % 0x100;
	
	// fill IO/control registers with 0xFF
	for (size_t i = 0xFF00; i < 0x10000; i++)
		ram[i] = 0xFF;
	
	if (core->IsUseBootRom())
	{
		ram[HWAddr::P1]		= 0x0F;
		ram[HWAddr::LCDC]	= 0x00;
		ram[HWAddr::STAT]	= 0x80;
		ram[HWAddr::SCY]	= 0x00;
		ram[HWAddr::SCX]	= 0x00;
		ram[HWAddr::BOOT]	= 0xFE;
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
		ram[HWAddr::OBP0]	= 0xFF;
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

		core->ppu->Enable();
	}
}

#pragma warning(pop)