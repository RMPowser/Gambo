#pragma once
#include "GamboDefine.h"

class CPU;
class PPU;
class Cartridge;

struct GamboState
{
	struct
	{
		bool Z, N, H, C, IME;
	} flags;

	struct
	{
		u8 A, F, B, C, D, E, H, L;
	} registers;

	u16 SP;
	u16 PC;

	u8 LCDC;
	u8 STAT;
	u8 LY;
	u8 IE;
	u8 IF;

	std::map<uint16_t, std::string> mapAsm;
};

class GamboCore
{
	friend class CPU;
	friend class PPU;
public:
	GamboCore();
	~GamboCore();

	void Run();
	
	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Reset();

	const void* GetScreen() const;
	float GetScreenWidth() const;
	float GetScreenHeight() const;
	GamboState GetState() const;

	bool GetDone();
	void SetDone(bool b);
	bool GetRunning();
	void SetRunning(bool b);
	bool GetStep();
	void SetStep(bool b);

	const Cartridge& GetCartridge() const;
	void InsertCartridge(std::filesystem::path filePath);

	void SetUseBootRom(bool b);
	bool IsUseBootRom();


private:
	std::map<u16, std::string> Disassemble(u16 startAddr, int numInstr) const;

	bool IsBootRomAddress(u16 addr);
	bool IsCartridgeAddress(u16 addr);

	void ResetRam();

	std::atomic<bool> done;
	std::atomic<bool> running;
	std::atomic<bool> step;

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
	std::vector<u8> ram;
	u16 lastWrite;
	u16 lastRead;
	CPU* cpu;
	PPU* ppu;
	Cartridge* cart;
	
	float screenWidth = GamboScreenWidth;
	float screenHeight = GamboScreenHeight;
	int screenScale = PixelScale; 
	bool disassemble = true;
	bool useBootRom;
};