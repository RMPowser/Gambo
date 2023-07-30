#pragma once
#include "GamboDefine.h"

class CPU;
class PPU;
class RAM;
class Cartridge;
class BootRom;
class VramViewer;

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
	friend class RAM;

public:
	GamboCore();
	~GamboCore();

	void Run();
	
	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Reset();

	const void* GetScreen() const;
	VramViewer& GetVramViewer();
	float GetScreenWidth() const;
	float GetScreenHeight() const;
	GamboState GetState() const;

	bool GetDone();
	void SetDone(bool b);
	bool GetRunning();
	void SetRunning(bool b);
	bool GetStep();
	void SetStep(bool b);
	bool GetStepFrame();
	void SetStepFrame(bool b);

	const Cartridge& GetCartridge() const;
	void InsertCartridge(std::filesystem::path filePath);

	void SetUseBootRom(bool b);
	bool IsUseBootRom();


private:
	std::map<u16, std::string> Disassemble(u16 startAddr, int numInstr) const;

	bool IsBootRomAddress(u16 addr);
	bool IsCartridgeAddress(u16 addr);

	std::atomic<bool> done;
	std::atomic<bool> running;
	std::atomic<bool> step;
	std::atomic<bool> stepFrame;

	CPU* cpu;
	PPU* ppu;
	RAM* ram;
	BootRom* boot;
	Cartridge* cart;
	VramViewer* vram;
	
	float screenWidth = GamboScreenWidth;
	float screenHeight = GamboScreenHeight;
	int screenScale = PixelScale; 
	bool disassemble = true;
	bool useBootRom;
};