#pragma once

#include <array>
#include <format>
#include "CPU.h"
#include "PPU.h"
#include "GamboDefine.h"
#include "Cartridge.h"

class Bus
{
public:
	Bus();
	~Bus();

	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Reset();

	void InsertCartridge(std::wstring filePath);

	void Disassemble(u16 startAddr, int numInstr);
	
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
	std::vector<u8> ram = std::vector<u8>(64KiB, 0x00);
	std::map<uint16_t, std::string> mapAsm;
	u16 lastWrite = 0;
	u16 lastRead = 0;
	CPU cpu;
	PPU ppu;
	Cartridge* cart;

private:
	bool IsBootRomAddress(u16 addr);
	bool IsCartridgeAddress(u16 addr);
};