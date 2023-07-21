#pragma once
#include "GamboDefine.h"

class GamboCore;
class PPU;

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
class RAM
{
	bool operator==(const RAM& other) const = delete;
public:
	RAM(GamboCore* c);
	~RAM();

	const u8 Read(u16 addr) const;
	void Write(u16 addr, u8 data);

	// Used only when i need to ignore memory rules
	u8& Get(u16 addr);

	void Reset();

private:
	GamboCore* core;
	std::array<u8, 64KiB> ram;
	u16 lastRead;
	u16 lastWrite;
};