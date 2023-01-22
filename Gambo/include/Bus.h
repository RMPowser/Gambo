#pragma once

#include <array>
#include <format>
#include "CPU.h"
#include "PPU.h"
#include "GamboDefine.h"

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
	std::array<u8, 0x10000> ram;
	std::map<uint16_t, std::string> mapAsm;
	u16 lastWrite = 0;
	u16 lastRead = 0;
	CPU cpu;
	PPU ppu;


	Bus() 
		: cpu(this)
		, ppu(this)
	{
		// zero out ram to prevent any shenanigans
		ram.fill(0x00);
	};

	u8 Read(uint16_t addr)
	{
		if (addr >= 0x0000 && addr <= 0xFFFF)
		{
			lastRead = addr;
			return ram[addr];
		}

		return 0x00;
	}

	void Write(u16 addr, u8 data)
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

			lastWrite = addr;
		}
	}


	void Disassemble(u16 startAddr, u16 endAddr)
	{
		u32 addr = startAddr;
		u8 value = 0x00, lo = 0x00, hi = 0x00;
		u16 lineAddr = 0;

		mapAsm.clear();

		while (addr <= (u32)endAddr)
		{
			// skip vram
			if (0x8800 <= addr && addr <= 0x9FFF)
			{
				addr++;
				continue;
			}

			lineAddr = addr;

			// prefix line with instruction addr
			std::string s = "$" + hex(addr, 4) + ": ";

			// read instruction and get readable name
			u8 opcode = ram[addr++];
			if (opcode == 0xCB)
			{
				// its a 16bit opcode so read another byte
				opcode = ram[addr++];

				s += cpu.instructions16bit[opcode].mnemonic;
			}
			else
			{
				auto& instruction = cpu.instructions8bit[opcode];
				if (instruction.bytes == 2)
				{
					u8 data = ram[addr++];
					s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 2)));
				}
				else if (instruction.bytes == 3)
				{
					u16 lo = ram[addr++];
					u16 hi = ram[addr++];
					u16 data = (hi << 8) | lo;
					s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 4)));
				}
				else
				{
					s += cpu.instructions8bit[opcode].mnemonic;
				}
			}


			mapAsm[lineAddr] = s;
		}
	}
};