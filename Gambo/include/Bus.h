#pragma once

#include <cstdint>
#include <array>
#include "CPU.h"
#include "PPU.h"

class Bus
{
public:
	// 64KB total system memory. memory is mapped:
	// 0000-3FFF | 16 KiB ROM bank 00			  | From cartridge, usually a fixed bank
	// 4000-7FFF | 16 KiB ROM Bank 01~NN		  | From cartridge, switchable bank via mapper(if any)
	// 8000-9FFF | 8 KiB Video RAM(VRAM)		  | In CGB mode, switchable bank 0 / 1
	// A000-BFFF | 8 KiB External RAM			  | From cartridge, switchable bank if any
	// C000-CFFF | 4 KiB Work RAM(WRAM)			  | 
	// D000-DFFF | 4 KiB Work RAM(WRAM)			  | In CGB mode, switchable bank 1~7
	// E000-FDFF | Mirror of C000~DDFF(ECHO RAM)  | Nintendo says use of this area is prohibited.
	// FE00-FE9F | Sprite attribute table(OAM)	  | 
	// FEA0-FEFF | Not Usable					  | Nintendo says use of this area is prohibited
	// FF00-FF7F | I / O Registers				  | 
	// FF80-FFFE | High RAM(HRAM)				  | 
	// FFFF-FFFF | Interrupt Enable register (IE) |
	std::array<uint8_t, 0x10000> ram;
	CPU cpu;
	PPU ppu;


	Bus() 
		: cpu(this)
		, ppu(this)
	{
		// zero out ram to prevent any shenanigans
		ram.fill(0x00);
	};

	uint8_t Read(uint16_t addr) const
	{
		if (addr >= 0x0000 && addr <= 0xFFFF)
		{
			return ram[addr];
		}

		return 0x00;
	}

	void Write(uint16_t addr, uint8_t data)
	{
		if (addr >= 0x0000 && addr <= 0xFFFF)
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
		}
	}
};