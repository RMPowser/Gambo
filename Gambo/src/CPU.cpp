#include "CPU.h"
#include "GamboCore.h"
#include "PPU.h"

CPU::CPU(GamboCore* c)
	: core(c)
{
	Reset();
}

CPU::~CPU()
{
}

u8 CPU::Read(u16 addr)
{
	if ((core->ppu->mode == PPUMode::OAMScan && (0xFE00 <= addr && addr <= 0xFE9F)) // accessing oam during oam scan
		|| (core->ppu->mode == PPUMode::Draw && ((0xFE00 <= addr && addr <= 0xFE9F) || (0x8000 <= addr && addr <= 0x9FFF)))) // accessing oam or vram during drawing
	{
		return 0x00;
	}
	else
	{
		return core->Read(addr);
	}
}

void CPU::Write(u16 addr, u8 data)
{
	if ((core->ppu->mode == PPUMode::OAMScan && (0xFE00 <= addr && addr <= 0xFE9F)) // accessing oam during oam scan
		|| (core->ppu->mode == PPUMode::Draw && ((0xFE00 <= addr && addr <= 0xFE9F) || (0x8000 <= addr && addr <= 0x9FFF)))) // accessing oam or vram during drawing
	{
		return;
	}

    core->Write(addr, data);
	if (addr == HWAddr::DMA)
	{
		core->ppu->DoDMATransfer = true;
	}
}

void CPU::Clock()
{
	if (cycles == 0)
	{
		cycles = instructions8bit[0].cycles;

		if (isHalted && InterruptPending())
		{
			isHalted = false;
		}
		
		if (!isHalted)
		{
			// read and execute code normally
			if (IMEOneInstructionDelay)
			{
				IME = true;
				IMEOneInstructionDelay = false;
			}

			opcode = Read(PC++);
			if (opcode == 0xCB)
			{
				// its a 16bit opcode
				opcode = Read(PC++);

				// lookup the initial number of clock cycles this instruction takes
				cycles = instructions16bit[opcode].cycles;

				// execute the instruction and see if we require additional clock cycles
				cycles += (this->*instructions16bit[opcode].Execute)();
			}
			else
			{
				// lookup the initial number of cycles this instruction takes
				cycles = instructions8bit[opcode].cycles;

				// execute the instruction and see if we require additional clock cycles
				cycles += (this->*instructions8bit[opcode].Execute)();
			}



			static u8& IF = core->ram[HWAddr::IF];
			static u8& IE = core->ram[HWAddr::IE];

			// handle interrupts in priority order
			if (IF & InterruptFlags::VBlank && IE & InterruptFlags::VBlank)
			{
				HandleInterrupt(InterruptFlags::VBlank);
			}
			else if (IF & InterruptFlags::LCDStat && IE & InterruptFlags::LCDStat)
			{
				HandleInterrupt(InterruptFlags::LCDStat);
			}
			else if (IF & InterruptFlags::Timer && IE & InterruptFlags::Timer)
			{
				HandleInterrupt(InterruptFlags::Timer);
			}
			else if (IF & InterruptFlags::Serial && IE & InterruptFlags::Serial)
			{
				HandleInterrupt(InterruptFlags::Serial);
			}
			else if (IF & InterruptFlags::Joypad && IE & InterruptFlags::Joypad)
			{
				HandleInterrupt(InterruptFlags::Joypad);
			}
		}
	}

	cycles--;

	UpdateTimers();
}

void CPU::HandleInterrupt(InterruptFlags f)
{
	static u8& IF = core->ram[HWAddr::IF];

	// give control to the interrupt if IME is enabled
	if (IME)
	{
		IME = false;

		// acknowledge the interrupt
		IF &= ~(f);

		cycles = 50;

		Push(PC);

		switch (f)
		{
			case VBlank:
				PC = 0x0040;
				break;

			case LCDStat:
				PC = 0x0048;
				break;

			case Timer:
				PC = 0x0050;
				break;

			case Serial:
				PC = 0x0058;
				break;

			case Joypad:
				PC = 0x0060;
				break;

			default:
				throw;
		}
	}
}

void CPU::UpdateTimers()
{
	// DIV register always counts up every 256 clock cycles
	static int DIVCounter = 0;
	static u8& DIV = core->ram[HWAddr::DIV];

	DIVCounter++;
	DIVCounter %= 256;

	if (DIVCounter == 0)
		DIV++;



	// TIMA iterates at a specified frequency and only if it's enabled.
	static int TIMACounter = 0;

	static u8& TAC = core->ram[HWAddr::TAC];
	static bool TIMAEnabled;
	TIMAEnabled = TAC & 0b100;
	if (TIMAEnabled)
	{
		static u8 TIMAFreqSelect = 0;
		if (TIMAFreqSelect != (TAC & 0b011))
		{
			TIMAFreqSelect = TAC & 0b011;
			TIMACounter = 0;
		}

		static int TIMAFreq;
		switch (TIMAFreqSelect)
		{
			case 0b00:
				TIMAFreq = 1024;
				break;

			case 0b01:
				TIMAFreq = 16;
				break;

			case 0b10:
				TIMAFreq = 64;
				break;

			case 0b11:
				TIMAFreq = 256;
				break;
		}

		// iterate TIMA at specified frequency
		TIMACounter++;
		TIMACounter %= TIMAFreq;
		if (TIMACounter == 0)
		{
			static u8& TIMA = core->ram[HWAddr::TIMA];
			TIMA++;
			if (TIMA == 0x00)
			{
				// TIMA resets to TMA register value
				TIMA = core->ram[HWAddr::TMA];
				// request interrupt
				core->ram[HWAddr::IF] |= InterruptFlags::Timer;
			}
		}
	}
}

bool CPU::InterruptPending()
{
	return (core->ram[HWAddr::IE] & core->ram[HWAddr::IF]) != 0;
}

void CPU::ADC(const u8 data)
{
	int carry = GetFlag(CPUFlags::C);

	int sum = A + data + carry;

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, ((A & 0xF) + (data & 0xF) + carry) > 0xF);
	SetFlag(CPUFlags::C, sum > 0xFF);

	A = (u8)sum;
	SetFlag(CPUFlags::Z, A == 0);
}

void CPU::SBC(const u8 data)
{
	int carry = GetFlag(CPUFlags::C);

	int sum = A - data - carry;

	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, ((A & 0xF) - (data & 0xF) - carry) < 0);
	SetFlag(CPUFlags::C, sum < 0);

	A = (u8)sum;
	SetFlag(CPUFlags::Z, A == 0);
}

void CPU::BIT(u8& reg, int bit)
{
	SetFlag(CPUFlags::Z, ((reg >> bit) & 0x01) == 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::N, 0);
}

bool CPU::InstructionComplete()
{
	return cycles == 0;
}

void CPU::Reset()
{
	if (!useBootRom)
	{
		// this puts the cpu back into a state as if it had just finished a legit boot sequence
		A = 0x01;
		SetFlag(CPUFlags::Z, 1);
		SetFlag(CPUFlags::N, 0);
		SetFlag(CPUFlags::H, 1);
		SetFlag(CPUFlags::C, 1);
		B = 0x00;
		C = 0x13;
		D = 0x00;
		E = 0xD8;
		H = 0x01;
		L = 0x4D;
		PC = 0x0100;
		SP = 0xFFFE;
		IME = false;
		stopMode = false;
		isHalted = false;

		// this is what the hardware registers look like at PC = 0x0100
		for (size_t i = 0xFF00; i < 0x10000; i++)
			core->ram[i] = 0xFF;
		
		core->ram[HWAddr::P1] = 0xCF;
		core->ram[HWAddr::SB] = 0x00;
		core->ram[HWAddr::SC] = 0x7E;
		core->ram[HWAddr::DIV] = 0xAC;
		core->ram[HWAddr::TIMA] = 0x00;
		core->ram[HWAddr::TMA] = 0x00;
		core->ram[HWAddr::TAC] = 0xF8;
		core->ram[HWAddr::IF] = 0xE1;
		core->ram[HWAddr::NR10] = 0x80;
		core->ram[HWAddr::NR11] = 0xBF;
		core->ram[HWAddr::NR12] = 0xF3;
		core->ram[HWAddr::NR13] = 0xFF;
		core->ram[HWAddr::NR14] = 0xBF;
		core->ram[HWAddr::NR21] = 0x3F;
		core->ram[HWAddr::NR22] = 0x00;
		core->ram[HWAddr::NR23] = 0xFF;
		core->ram[HWAddr::NR24] = 0xBF;
		core->ram[HWAddr::NR30] = 0x7F;
		core->ram[HWAddr::NR31] = 0xFF;
		core->ram[HWAddr::NR32] = 0x9F;
		core->ram[HWAddr::NR33] = 0xFF;
		core->ram[HWAddr::NR34] = 0xBF;
		core->ram[HWAddr::NR41] = 0xFF;
		core->ram[HWAddr::NR42] = 0x00;
		core->ram[HWAddr::NR43] = 0x00;
		core->ram[HWAddr::NR44] = 0xBF;
		core->ram[HWAddr::NR50] = 0x77;
		core->ram[HWAddr::NR51] = 0xF3;
		core->ram[HWAddr::NR52] = 0xF1;
		core->ram[HWAddr::LCDC] = 0x91;
		core->ram[HWAddr::STAT] = 0x80;
		core->ram[HWAddr::SCY] = 0x00;
		core->ram[HWAddr::SCX] = 0x00;
		core->ram[HWAddr::LY] = 0x00;
		core->ram[HWAddr::LYC] = 0x00;
		core->ram[HWAddr::DMA] = 0xFF;
		core->ram[HWAddr::BGP] = 0xFC;
		core->ram[HWAddr::OBP0] = 0x00;
		core->ram[HWAddr::OBP1] = 0x00;
		core->ram[HWAddr::WY] = 0x00;
		core->ram[HWAddr::WX] = 0x00;
		core->ram[HWAddr::KEY1] = 0xFF;
		core->ram[HWAddr::VBK] = 0xFF;
		core->ram[HWAddr::BOOT] = 0xFF;
		core->ram[HWAddr::HDMA1] = 0xFF;
		core->ram[HWAddr::HDMA2] = 0xFF;
		core->ram[HWAddr::HDMA3] = 0xFF;
		core->ram[HWAddr::HDMA4] = 0xFF;
		core->ram[HWAddr::HDMA5] = 0xFF;
		core->ram[HWAddr::RP] = 0xFF;
		core->ram[HWAddr::BCPS] = 0xFF;
		core->ram[HWAddr::BCPD] = 0xFF;
		core->ram[HWAddr::OCPS] = 0xFF;
		core->ram[HWAddr::OCPD] = 0xFF;
		core->ram[HWAddr::SVBK] = 0xFF;
		core->ram[HWAddr::IE] = 0x00;
	}
	else
	{
		A = 0x00;
		SetFlag(CPUFlags::Z, 0);
		SetFlag(CPUFlags::N, 0);
		SetFlag(CPUFlags::H, 0);
		SetFlag(CPUFlags::C, 0);
		B = 0x00;
		C = 0x00;
		D = 0x00;
		E = 0x00;
		H = 0x00;
		L = 0x00;
		PC = 0x0000;
		SP = 0xFFFE;
		IME = false;
		stopMode = false;
		isHalted = false;
		core->ram[HWAddr::BOOT] = 0xFE;
		core->ram[HWAddr::P1] = 0x0F;
	}
}

void CPU::SetFlag(CPUFlags f, bool v)
{
	if (v)
		F |= (u8)f;
	else
		F &= ~(u8)f;
}

bool CPU::GetFlag(CPUFlags f)
{
	return ((F & (u8)f) > 0)
		? true
		: false;
}

void CPU::Push(const std::same_as<u16> auto data)
{
	static u8 high;
	static u8 low;

	high = data >> 8;
	low = data & 0xFF;

	Write(--SP, high);
	Write(--SP, low);
}

void CPU::Pop(std::same_as<u16> auto& reg)
{
	static u16 high;
	static u16 low;
	low = Read(SP++);
	high = Read(SP++);

	reg = (high << 8) | low;
}

#pragma region CPU Control Instructions
// catch all non existing instructions
u8 CPU::XXX()
{
	throw;
}

u8 CPU::NOP()
{
	return 0;
}

u8 CPU::STOP()
{
	stopMode = true;
	return 0;
}

u8 CPU::DAA()
{
	if (!GetFlag(CPUFlags::N)) {  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
		if (GetFlag(CPUFlags::C) || A > 0x99) 
		{ 
			A += 0x60;
			SetFlag(CPUFlags::C, 1); 
		}
		if (GetFlag(CPUFlags::H) || (A & 0x0F) > 0x09) 
		{ A += 0x6; }
	}
	else {  // after a subtraction, only adjust if (half-)carry occurred
		if (GetFlag(CPUFlags::C)) { A -= 0x60; }
		if (GetFlag(CPUFlags::H)) { A -= 0x6; }
	}

	// these flags are always updated
	SetFlag(CPUFlags::Z, A == 0); // the usual z flag
	SetFlag(CPUFlags::H, 0); // h flag is always cleared
	return 0;
}

u8 CPU::CPL()
{
	A = ~A;

	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, 1);
	return 0;
}

u8 CPU::SCF()
{
	SetFlag(CPUFlags::C, 1);

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	return 0;
}

u8 CPU::CCF()
{
	SetFlag(CPUFlags::C, !GetFlag(CPUFlags::C));

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	return 0;
}

u8 CPU::HALT()
{
	isHalted = true;

	return 0;
}

u8 CPU::DI()
{
	IME = false;

	return 0;
}

u8 CPU::EI()
{
	IMEOneInstructionDelay = true;

	return 0;
}
#pragma endregion
#pragma region Jump Instructions
u8 CPU::JR_s8()
{
	PC = (s8)Read(PC) + PC + 1;
	
	return 0;
}

u8 CPU::JR_NZ_s8()
{
	if (!GetFlag(CPUFlags::Z))
	{
		PC = (s8)Read(PC) + PC + 1;
		return 4;
	}
	else
	{
		PC++;
		return 0;
	}
}

u8 CPU::JR_Z_s8()
{
	if (GetFlag(CPUFlags::Z))
	{
		PC = (s8)Read(PC) + PC + 1;
		return 4;
	}
	else
	{
		PC++;
		return 0;
	}
}

u8 CPU::JR_NC_s8()
{
	if (!GetFlag(CPUFlags::C))
	{
		PC = (s8)Read(PC) + PC + 1;
		return 4;
	}
	else
	{
		PC++;
		return 0;
	}
}

u8 CPU::JR_C_s8()
{
	if (GetFlag(CPUFlags::C))
	{
		PC = (s8)Read(PC) + PC + 1;
		return 4;
	}
	else
	{
		PC++;
		return 0;
	}
}

u8 CPU::RET_NZ()
{
	if (!GetFlag(CPUFlags::Z))
	{
		Pop(PC);
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::JP_NZ_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (!GetFlag(CPUFlags::Z))
	{
		PC = addr;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::JP_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	low = Read(PC++);
	high = Read(PC++);

	PC = (high << 8) | low;
	return 0;
}

u8 CPU::CALL_NZ_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (!GetFlag(CPUFlags::Z))
	{
		Push(PC);
		PC = addr;
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RST_0()
{
	Push(PC);

	PC = 0x0000;
	return 0;
}

u8 CPU::RET_Z()
{
	if (GetFlag(CPUFlags::Z))
	{
		Pop(PC);
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RET()
{
	Pop(PC);
	return 0;
}

u8 CPU::JP_Z_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (GetFlag(CPUFlags::Z))
	{
		PC = addr;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::CALL_Z_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (GetFlag(CPUFlags::Z))
	{
		Push(PC);
		PC = addr;
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::CALL_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	Push(PC);
	PC = addr;

	return 0;
}

u8 CPU::RST_1()
{
	Push(PC);
	PC = 0x0008;
	return 0;
}

u8 CPU::RET_NC()
{
	if (!GetFlag(CPUFlags::C))
	{
		Pop(PC);
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::JP_NC_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (!GetFlag(CPUFlags::C))
	{
		PC = addr;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::CALL_NC_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (!GetFlag(CPUFlags::C))
	{
		Push(PC);
		PC = addr;
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RST_2()
{
	Push(PC);
	PC = 0x0010;
	return 0;
}

u8 CPU::RET_C()
{
	if (GetFlag(CPUFlags::C))
	{
		Pop(PC);
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RETI()
{
	EI();
	RET();

	return 0;
}

u8 CPU::JP_C_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (GetFlag(CPUFlags::C))
	{
		PC = addr;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::CALL_C_a16()
{
	static u16 low = 0;
	static u16 high = 0;
	static u16 addr = 0;
	low = Read(PC++);
	high = Read(PC++);

	addr = (high << 8) | low;
	if (GetFlag(CPUFlags::C))
	{
		Push(PC);
		PC = addr;
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RST_3()
{
	Push(PC);
	PC = 0x0018;
	return 0;
}

u8 CPU::RST_4()
{
	Push(PC);
	PC = 0x0020;
	return 0;
}

u8 CPU::JP_HL()
{
	PC = HL;
	return 0;
}

u8 CPU::RST_5()
{
	Push(PC);
	PC = 0x0028;
	return 0;
}

u8 CPU::RST_6()
{
	Push(PC);
	PC = 0x0030;
	return 0;
}

u8 CPU::RST_7()
{
	Push(PC);
	PC = 0x0038;
	return 0;
}
#pragma endregion
#pragma region 8bit Load Instructions
u8 CPU::LD_aBC_A()
{
	Write(BC, A);
	return 0;
}

u8 CPU::LD_B_d8()
{
	B = Read(PC++);
	return 0;
}

u8 CPU::LD_A_aBC()
{
	A = Read(BC);
	return 0;
}

u8 CPU::LD_C_d8()
{
	C = Read(PC++);
	return 0;
}

u8 CPU::LD_aDE_A()
{
	Write(DE, A);
	return 0;
}

u8 CPU::LD_D_d8()
{
	D = Read(PC++);
	return 0;
}

u8 CPU::LD_A_aDE()
{
	A = Read(DE);
	return 0;
}

u8 CPU::LD_E_d8()
{
	E = Read(PC++);
	return 0;
}

u8 CPU::LD_aHLinc_A()
{
	Write(HL++, A);
	return 0;
}

u8 CPU::LD_H_d8()
{
	H = Read(PC++);
	return 0;
}

u8 CPU::LD_A_aHLinc()
{
	A = Read(HL++);
	return 0;
}

u8 CPU::LD_L_d8()
{
	L = Read(PC++);
	return 0;
}

u8 CPU::LD_aHLdec_A()
{
	Write(HL--, A);
	return 0;
}

u8 CPU::LD_aHL_d8()
{
	static u8 data;
	data = Read(PC++);
	Write(HL, data);
	return 0;
}

u8 CPU::LD_A_aHLdec()
{
	A = Read(HL--);
	return 0;
}

u8 CPU::LD_A_d8()
{
	A = Read(PC++);
	return 0;
}

u8 CPU::LD_B_B()
{
	B = B;
	return 0;
}

u8 CPU::LD_B_C()
{
	B = C;
	return 0;
}

u8 CPU::LD_B_D()
{
	B = D;
	return 0;
}

u8 CPU::LD_B_E()
{
	B = E;
	return 0;
}

u8 CPU::LD_B_H()
{
	B = H;
	return 0;
}

u8 CPU::LD_B_L()
{
	B = L;
	return 0;
}

u8 CPU::LD_B_aHL()
{
	B = Read(HL);
	return 0;
}

u8 CPU::LD_B_A()
{
	B = A;
	return 0;
}

u8 CPU::LD_C_B()
{
	C = B;
	return 0;
}

u8 CPU::LD_C_C()
{
	C = C;
	return 0;
}

u8 CPU::LD_C_D()
{
	C = D;
	return 0;
}

u8 CPU::LD_C_E()
{
	C = E;
	return 0;
}

u8 CPU::LD_C_H()
{
	C = H;
	return 0;
}

u8 CPU::LD_C_L()
{
	C = L;
	return 0;
}

u8 CPU::LD_C_aHL()
{
	C = Read(HL);
	return 0;
}

u8 CPU::LD_C_A()
{
	C = A;
	return 0;
}

u8 CPU::LD_D_B()
{
	D = B;
	return 0;
}

u8 CPU::LD_D_C()
{
	D = C;
	return 0;
}

u8 CPU::LD_D_D()
{
	D = D;
	return 0;
}

u8 CPU::LD_D_E()
{
	D = E;
	return 0;
}

u8 CPU::LD_D_H()
{
	D = H;
	return 0;
}

u8 CPU::LD_D_L()
{
	D = L;
	return 0;
}

u8 CPU::LD_D_aHL()
{
	D = Read(HL);
	return 0;
}

u8 CPU::LD_D_A()
{
	D = A;
	return 0;
}

u8 CPU::LD_E_B()
{
	E = B;
	return 0;
}

u8 CPU::LD_E_C()
{
	E = C;
	return 0;
}

u8 CPU::LD_E_D()
{
	E = D;
	return 0;
}

u8 CPU::LD_E_E()
{
	E = E;
	return 0;
}

u8 CPU::LD_E_H()
{
	E = H;
	return 0;
}

u8 CPU::LD_E_L()
{
	E = L;
	return 0;
}

u8 CPU::LD_E_aHL()
{
	E = Read(HL);
	return 0;
}

u8 CPU::LD_E_A()
{
	E = A;
	return 0;
}

u8 CPU::LD_H_B()
{
	H = B;
	return 0;
}

u8 CPU::LD_H_C()
{
	H = C;
	return 0;
}

u8 CPU::LD_H_D()
{
	H = D;
	return 0;
}

u8 CPU::LD_H_E()
{
	H = E;
	return 0;
}

u8 CPU::LD_H_H()
{
	H = H;
	return 0;
}

u8 CPU::LD_H_L()
{
	H = L;
	return 0;
}

u8 CPU::LD_H_aHL()
{
	H = Read(HL);
	return 0;
}

u8 CPU::LD_H_A()
{
	H = A;
	return 0;
}

u8 CPU::LD_L_B()
{
	L = B;
	return 0;
}

u8 CPU::LD_L_C()
{
	L = C;
	return 0;
}

u8 CPU::LD_L_D()
{
	L = D;
	return 0;
}

u8 CPU::LD_L_E()
{
	L = E;
	return 0;
}

u8 CPU::LD_L_H()
{
	L = H;
	return 0;
}

u8 CPU::LD_L_L()
{
	L = L;
	return 0;
}

u8 CPU::LD_L_aHL()
{
	L = Read(HL);
	return 0;
}

u8 CPU::LD_L_A()
{
	L = A;
	return 0;
}

u8 CPU::LD_aHL_B()
{
	Write(HL, B);
	return 0;
}

u8 CPU::LD_aHL_C()
{
	Write(HL, C);
	return 0;
}

u8 CPU::LD_aHL_D()
{
	Write(HL, D);
	return 0;
}

u8 CPU::LD_aHL_E()
{
	Write(HL, E);
	return 0;
}

u8 CPU::LD_aHL_H()
{
	Write(HL, H);
	return 0;
}

u8 CPU::LD_aHL_L()
{
	Write(HL, L);
	return 0;
}

u8 CPU::LD_aHL_A()
{
	Write(HL, A);
	return 0;
}

u8 CPU::LD_A_B()
{
	A = B;
	return 0;
}

u8 CPU::LD_A_C()
{
	A = C;
	return 0;
}

u8 CPU::LD_A_D()
{
	A = D;
	return 0;
}

u8 CPU::LD_A_E()
{
	A = E;
	return 0;
}

u8 CPU::LD_A_H()
{
	A = H;
	return 0;
}

u8 CPU::LD_A_L()
{
	A = L;
	return 0;
}

u8 CPU::LD_A_aHL()
{
	A = Read(HL);
	return 0;
}

u8 CPU::LD_A_A()
{
	A = A;
	return 0;
}

u8 CPU::LD_aa8_A()
{
	u16 addr = Read(PC++) | 0xFF00;
	Write(addr, A);
	return 0;
}

u8 CPU::LD_aC_A()
{
	u16 addr = C | 0xFF00;
	Write(addr, A);
	return 0;
}

u8 CPU::LD_aa16_A()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
	Write(addr, A);
	return 0;
}

u8 CPU::LD_A_aa8()
{
	A = Read(Read(PC++) | 0xFF00);
	return 0;
}

u8 CPU::LD_A_aC()
{
	A = Read(u16(0xFF00 + C));
	return 0;
}

u8 CPU::LD_A_aa16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
	A = Read(addr);
	return 0;
}
#pragma endregion
#pragma region 16bit Load Instructions
u8 CPU::LD_BC_d16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 data = (high << 8) | low;
	BC = data;
	return 0;
}

u8 CPU::LD_a16_SP()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
	u8 spLow = u8(SP & 0xFF);
	u8 spHigh = u8(SP >> 8);
	Write(addr, spLow);
	Write(addr + 1, spHigh);
	return 0;
}

u8 CPU::LD_DE_d16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 data = (high << 8) | low;
	DE = data;
	return 0;
}

u8 CPU::LD_HL_d16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 data = (high << 8) | low;
	HL = data;
	return 0;
}

u8 CPU::LD_SP_d16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 data = (high << 8) | low;
	SP = data;
	return 0;
}

u8 CPU::POP_BC()
{
	Pop(BC);
	return 0;
}

u8 CPU::PUSH_BC()
{
	Push(BC);
	return 0;
}

u8 CPU::POP_DE()
{
	Pop(DE);
	return 0;
}

u8 CPU::PUSH_DE()
{
	Push(DE);
	return 0;
}

u8 CPU::POP_HL()
{
	Pop(HL);
	return 0;
}

u8 CPU::PUSH_HL()
{
	Push(HL);
	return 0;
}

u8 CPU::POP_AF()
{
	Pop(AF);
	F &= ~(0b00001111);
	return 0;
}

u8 CPU::PUSH_AF()
{
	Push(AF);
	return 0;
}

u8 CPU::LD_HL_SPinc_s8()
{
	static s8 data;
	static s16 result8;
	static int result16;

	data = Read(PC++);

	int sum = SP + data;
	int noCarrySum = SP ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	HL = sum;

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	return 0;
}

u8 CPU::LD_SP_HL()
{
	SP = HL;
	return 0;
}
#pragma endregion
#pragma region 8bit Arithmetic Instructions
u8 CPU::INC_B()
{
	static int prev;
	prev = B;
	B++;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (B & 0xF) < (prev & 0xF));
	
	return 0;
}

u8 CPU::DEC_B()
{
	B--;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (B & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_C()
{
	static int prev;
	prev = C;
	C++;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (C & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_C()
{
	C--;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (C & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_D()
{
	static int prev;
	prev = D;
	D++;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (D & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_D()
{
	D--;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (D & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_E()
{
	static int prev;
	prev = E;
	E++;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (E & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_E()
{
	E--;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (E & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_H()
{
	static int prev;
	prev = H;
	H++;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (H & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_H()
{
	H--;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (H & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_L()
{
	static int prev;
	prev = L;
	L++;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (L & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_L()
{
	L--;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (L & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_aHL()
{
	static int prev;
	prev = Read(HL);
	Write(HL, prev + 1);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (Read(HL) & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_aHL()
{
	static int data;
	data = Read(HL);
	Write(HL, --data);

	SetFlag(CPUFlags::Z, data == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (data & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_A()
{
	static int prev;
	prev = A;
	A++;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (A & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_A()
{
	A--;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) == 0xF);

	return 0;
}

u8 CPU::ADD_A_B()
{
	int sum = A + B;
	int noCarrySum = A ^ B;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += B;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_C()
{
	int sum = A + C;
	int noCarrySum = A ^ C;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += C;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_D()
{
	int sum = A + D;
	int noCarrySum = A ^ D;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += D;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_E()
{
	int sum = A + E;
	int noCarrySum = A ^ E;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += E;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_H()
{
	int sum = A + H;
	int noCarrySum = A ^ H;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += H;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_L()
{
	int sum = A + L;
	int noCarrySum = A ^ L;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += L;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_aHL()
{
	int data = Read(HL);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += data;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADD_A_A()
{
	int sum = A + A;
	int noCarrySum = A ^ A;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += A;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADC_A_B()
{
	ADC(B);
	return 0;
}

u8 CPU::ADC_A_C()
{
	ADC(C);
	return 0;
}

u8 CPU::ADC_A_D()
{
	ADC(D);
	return 0;
}

u8 CPU::ADC_A_E()
{
	ADC(E);
	return 0;
}

u8 CPU::ADC_A_H()
{
	ADC(H);
	return 0;
}

u8 CPU::ADC_A_L()
{
	ADC(L);
	return 0;
}

u8 CPU::ADC_A_aHL()
{
	ADC(Read(HL));
	return 0;
}

u8 CPU::ADC_A_A()
{
	ADC(A);
	return 0;
}

u8 CPU::SUB_A_B()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (B & 0xF));
	SetFlag(CPUFlags::C, A < B);

	A -= B;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_C()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (C & 0xF));
	SetFlag(CPUFlags::C, A < C);

	A -= C;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_D()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (D & 0xF));
	SetFlag(CPUFlags::C, A < D);

	A -= D;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_E()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (E & 0xF));
	SetFlag(CPUFlags::C, A < E);

	A -= E;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_H()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (H & 0xF));
	SetFlag(CPUFlags::C, A < H);

	A -= H;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_L()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (L & 0xF));
	SetFlag(CPUFlags::C, A < L);

	A -= L;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_aHL()
{
	static u8 data;
	data = Read(HL);

	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (data & 0xF));
	SetFlag(CPUFlags::C, A < data);

	A -= data;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SUB_A_A()
{
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (A & 0xF));
	SetFlag(CPUFlags::C, A < A);

	A -= A;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SBC_A_B()
{
	SBC(B);
	return 0;
}

u8 CPU::SBC_A_C()
{
	SBC(C);
	return 0;
}

u8 CPU::SBC_A_D()
{
	SBC(D);
	return 0;
}

u8 CPU::SBC_A_E()
{
	SBC(E);
	return 0;
}

u8 CPU::SBC_A_H()
{
	SBC(H);
	return 0;
}

u8 CPU::SBC_A_L()
{
	SBC(L);
	return 0;
}

u8 CPU::SBC_A_aHL()
{
	SBC(Read(HL));
	return 0;
}

u8 CPU::SBC_A_A()
{
	SBC(A);
	return 0;
}

u8 CPU::AND_A_B()
{
	A &= B;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_C()
{
	A &= C;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_D()
{
	A &= D;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_E()
{
	A &= E;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_H()
{
	A &= H;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_L()
{
	A &= L;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_aHL()
{
	A &= Read(HL);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::AND_A_A()
{
	A &= A;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_B()
{
	A ^= B;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_C()
{
	A ^= C;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_D()
{
	A ^= D;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_E()
{
	A ^= E;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_H()
{
	A ^= H;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_L()
{
	A ^= L;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_aHL()
{
	A ^= Read(HL);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_A()
{
	A ^= A;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_B()
{
	A |= B;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_C()
{
	A |= C;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_D()
{
	A |= D;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_E()
{
	A |= E;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_H()
{
	A |= H;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_L()
{
	A |= L;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_aHL()
{
	A |= Read(HL);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_A()
{
	A |= A;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::CP_A_B()
{
	static u16 result;
	result = A - B;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (B & 0xF));
	SetFlag(CPUFlags::C, A < B);

	return 0;
}

u8 CPU::CP_A_C()
{
	static u16 result;
	result = A - C;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (C & 0xF));
	SetFlag(CPUFlags::C, A < C);

	return 0;
}

u8 CPU::CP_A_D()
{
	static u16 result;
	result = A - D;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (D & 0xF));
	SetFlag(CPUFlags::C, A < D);

	return 0;
}

u8 CPU::CP_A_E()
{
	static u16 result;
	result = A - E;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (E & 0xF));
	SetFlag(CPUFlags::C, A < E);

	return 0;
}

u8 CPU::CP_A_H()
{
	static u16 result;
	result = A - H;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (H & 0xF));
	SetFlag(CPUFlags::C, A < H);

	return 0;
}

u8 CPU::CP_A_L()
{
	static u16 result;
	result = A - L;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (L & 0xF));
	SetFlag(CPUFlags::C, A < L);

	return 0;
}

u8 CPU::CP_A_aHL()
{
	static u16 result;
	static u8 data;
	data = Read(HL);
	result = A - data;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (data & 0xF));
	SetFlag(CPUFlags::C, A < data);

	return 0;
}

u8 CPU::CP_A_A()
{
	static s16 result;
	result = A - A;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (A & 0xF));
	SetFlag(CPUFlags::C, A < A);

	return 0;
}

u8 CPU::ADD_A_d8()
{
	static u8 data;
	data = Read(PC++);

	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	A += data;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::ADC_A_d8()
{
	ADC(Read(PC++));
	return 0;
}

u8 CPU::SUB_A_d8()
{
	static u8 data;
	data = Read(PC++);

	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::C, A < data);
	SetFlag(CPUFlags::H, (A & 0xF) < (data & 0xF));

	A -= data;
	SetFlag(CPUFlags::Z, A == 0);
	return 0;
}

u8 CPU::SBC_A_d8()
{
	SBC(Read(PC++));
	return 0;
}

u8 CPU::AND_A_d8()
{
	A &= Read(PC++);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 1);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::XOR_A_d8()
{
	A ^= Read(PC++);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::OR_A_d8()
{
	A |= Read(PC++);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::CP_A_d8()
{
	static u16 result;
	static u8 data;
	data = Read(PC++);
	result = A - data;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (data & 0xF));
	SetFlag(CPUFlags::C, A < data);

	return 0;
}
#pragma endregion
#pragma region 16bit Arithmetic Instructions
u8 CPU::INC_BC()
{
	BC++;
	return 0;
}

u8 CPU::ADD_HL_BC()
{
	int sum = HL + BC;
	int noCarrySum = HL ^ BC;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x1000);
	SetFlag(CPUFlags::C, carryBits & 0x10000);

	HL += BC;
	return 0;
}

u8 CPU::DEC_BC()
{
	BC--;
	return 0;
}

u8 CPU::INC_DE()
{
	DE++;
	return 0;
}

u8 CPU::ADD_HL_DE()
{
	int sum = HL + DE;
	int noCarrySum = HL ^ DE;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x1000);
	SetFlag(CPUFlags::C, carryBits & 0x10000);

	HL += DE;
	return 0;
}

u8 CPU::DEC_DE()
{
	DE--;
	return 0;
}

u8 CPU::INC_HL()
{
	HL++;
	return 0;
}

u8 CPU::ADD_HL_HL()
{
	int sum = HL + HL;
	int noCarrySum = HL ^ HL;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x1000);
	SetFlag(CPUFlags::C, carryBits & 0x10000);

	HL += HL;
	return 0;
}

u8 CPU::DEC_HL()
{
	HL--;
	return 0;
}

u8 CPU::INC_SP()
{
	SP++;
	return 0;
}

u8 CPU::ADD_HL_SP()
{
	int sum = HL + SP;
	int noCarrySum = HL ^ SP;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x1000);
	SetFlag(CPUFlags::C, carryBits & 0x10000);

	HL += SP;
	return 0;
}

u8 CPU::DEC_SP()
{
	SP--;
	return 0;
}

u8 CPU::ADD_SP_s8()
{
	s8 data = Read(PC++);
	int sum = SP + data;
	int noCarrySum = SP ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, carryBits & 0x10);
	SetFlag(CPUFlags::C, carryBits & 0x100);

	SP += data;
	return 0;
}
#pragma endregion
#pragma region 8bit bit instructions
u8 CPU::RLCA()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RRCA()
{
	static u8 bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RLA()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RRA()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | ((u8)GetFlag(CPUFlags::C) << 7);

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RLC_B()
{
	static bool bit7;
	bit7 = B & 0b10000000;

	B = (B << 1) | (u8)bit7;
	
	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);
	
	return 0;
}

u8 CPU::RLC_C()
{
	static bool bit7;
	bit7 = C & 0b10000000;

	C = (C << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_D()
{
	static bool bit7;
	bit7 = D & 0b10000000;

	D = (D << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_E()
{
	static bool bit7;
	bit7 = E & 0b10000000;

	E = (E << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_H()
{
	static bool bit7;
	bit7 = H & 0b10000000;

	H = (H << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_L()
{
	static bool bit7;
	bit7 = L & 0b10000000;

	L = (L << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_aHL()
{
	static bool bit7;
	static u8 data;
	data = Read(HL);
	bit7 = data & 0b10000000;

	Write(HL, (data << 1) | (u8)bit7);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_A()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RRC_B()
{
	static u8 bit0;
	bit0 = B & 0b00000001;

	B = (B >> 1) | (bit0 << 7);
	
	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_C()
{
	static u8 bit0;
	bit0 = C & 0b00000001;

	C = (C >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_D()
{
	static u8 bit0;
	bit0 = D & 0b00000001;

	D = (D >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_E()
{
	static u8 bit0;
	bit0 = E & 0b00000001;

	E = (E >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_H()
{
	static u8 bit0;
	bit0 = H & 0b00000001;

	H = (H >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_L()
{
	static u8 bit0;
	bit0 = L & 0b00000001;

	L = (L >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_aHL()
{
	static u8 bit0;
	static u8 data;
	data = Read(HL);
	bit0 = data & 0b00000001;

	Write(HL, (data >> 1) | (bit0 << 7));

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_A()
{
	static u8 bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RL_B()
{
	static bool bit7;
	bit7 = B & 0b10000000;

	B = (B << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_C()
{
	static bool bit7;
	bit7 = C & 0b10000000;

	C = (C << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_D()
{
	static bool bit7;
	bit7 = D & 0b10000000;

	D = (D << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_E()
{
	static bool bit7;
	bit7 = E & 0b10000000;

	E = (E << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_H()
{
	static bool bit7;
	bit7 = H & 0b10000000;

	H = (H << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_L()
{
	static bool bit7;
	bit7 = L & 0b10000000;

	L = (L << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_aHL()
{
	static bool bit7;
	static u8 data;
	data = Read(HL);
	bit7 = data & 0b10000000;

	Write(HL, (data << 1) | (u8)GetFlag(CPUFlags::C));

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_A()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RR_B()
{
	static bool bit0;
	bit0 = B & 0b00000001;

	B = (B >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_C()
{
	static bool bit0;
	bit0 = C & 0b00000001;

	C = (C >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_D()
{
	static bool bit0;
	bit0 = D & 0b00000001;

	D = (D >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_E()
{
	static bool bit0;
	bit0 = E & 0b00000001;

	E = (E >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_H()
{
	static bool bit0;
	bit0 = H & 0b00000001;

	H = (H >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_L()
{
	static bool bit0;
	bit0 = L & 0b00000001;

	L = (L >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_aHL()
{
	static bool bit0;
	static u8 data;
	data = Read(HL);
	bit0 = data & 0b00000001;

	Write(HL, (data >> 1) | (u8)GetFlag(CPUFlags::C) << 7);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_A()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SLA_B()
{
	static bool bit7;
	bit7 = B & 0b10000000;

	B <<= 1;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_C()
{
	static bool bit7;
	bit7 = C & 0b10000000;

	C <<= 1;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_D()
{
	static bool bit7;
	bit7 = D & 0b10000000;

	D <<= 1;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_E()
{
	static bool bit7;
	bit7 = E & 0b10000000;

	E <<= 1;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_H()
{
	static bool bit7;
	bit7 = H & 0b10000000;

	H <<= 1;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_L()
{
	static bool bit7;
	bit7 = L & 0b10000000;

	L <<= 1;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_aHL()
{
	static bool bit7;
	static u8 data;
	data = Read(HL);
	bit7 = data & 0b10000000;

	Write(HL, data << 1);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_A()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A <<= 1;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SRA_B()
{
	static bool bit0;
	static bool bit7;
	bit0 = B & 0b00000001;
	bit7 = B & 0b10000000;

	B = (B >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_C()
{
	static bool bit0;
	static bool bit7;
	bit0 = C & 0b00000001;
	bit7 = C & 0b10000000;

	C = (C >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_D()
{
	static bool bit0;
	static bool bit7;
	bit0 = D & 0b00000001;
	bit7 = D & 0b10000000;

	D = (D >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_E()
{
	static bool bit0;
	static bool bit7;
	bit0 = E & 0b00000001;
	bit7 = E & 0b10000000;

	E = (E >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_H()
{
	static bool bit0;
	static bool bit7;
	bit0 = H & 0b00000001;
	bit7 = H & 0b10000000;

	H = (H >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_L()
{
	static bool bit0;
	static bool bit7;
	bit0 = L & 0b00000001;
	bit7 = L & 0b10000000;

	L = (L >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_aHL()
{
	static u8 data;
	static bool bit0;
	static bool bit7;
	data = Read(HL);
	bit0 = data & 0b00000001;
	bit7 = (data & 0b10000000) >> 7;

	Write(HL, (data >> 1) | (bit7 << 7));

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_A()
{
	static bool bit0;
	static bool bit7;
	bit0 = A & 0b00000001;
	bit7 = A & 0b10000000;

	A = (A >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SWAP_B()
{
	static u8 low;
	static u8 high;
	low = B & 0b00001111;
	high = B >> 4;

	B = (low << 4) | high;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_C()
{
	static u8 low;
	static u8 high;
	low = C & 0b00001111;
	high = C >> 4;

	C = (low << 4) | high;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_D()
{
	static u8 low;
	static u8 high;
	low = D & 0b00001111;
	high = D >> 4;

	D = (low << 4) | high;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_E()
{
	static u8 low;
	static u8 high;
	low = E & 0b00001111;
	high = E >> 4;

	E = (low << 4) | high;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_H()
{
	static u8 low;
	static u8 high;
	low = H & 0b00001111;
	high = H >> 4;

	H = (low << 4) | high;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_L()
{
	static u8 low;
	static u8 high;
	low = L & 0b00001111;
	high = L >> 4;

	L = (low << 4) | high;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_aHL()
{
	static u8 data;
	data = Read(HL);

	static u8 low;
	static u8 high;
	low = data & 0b00001111;
	high = data >> 4;

	Write(HL, (low << 4) | high);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_A()
{
	static u8 low;
	static u8 high;
	low = A & 0b00001111;
	high = A >> 4;

	A = (low << 4) | high;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SRL_B()
{
	static bool bit0;
	bit0 = B & 0b00000001;

	B >>= 1;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_C()
{
	static bool bit0;
	bit0 = C & 0b00000001;

	C >>= 1;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_D()
{
	static bool bit0;
	bit0 = D & 0b00000001;

	D >>= 1;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_E()
{
	static bool bit0;
	bit0 = E & 0b00000001;

	E >>= 1;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_H()
{
	static bool bit0;
	bit0 = H & 0b00000001;

	H >>= 1;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_L()
{
	static bool bit0;
	bit0 = L & 0b00000001;

	L >>= 1;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_aHL()
{
	static u8 data;
	static bool bit0;
	data = Read(HL);
	bit0 = data & 0b00000001;

	Write(HL, data >> 1);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_A()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	A >>= 1;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::BIT_0_B()
{

	BIT(B, 0);

	return 0;
}

u8 CPU::BIT_0_C()
{
	BIT(C, 0);

	return 0;
}

u8 CPU::BIT_0_D()
{
	BIT(D, 0);

	return 0;
}

u8 CPU::BIT_0_E()
{
	BIT(E, 0);

	return 0;
}

u8 CPU::BIT_0_H()
{
	BIT(H, 0);

	return 0;
}

u8 CPU::BIT_0_L()
{
	BIT(L, 0);

	return 0;
}

u8 CPU::BIT_0_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 0);

	return 0;
}

u8 CPU::BIT_0_A()
{
	BIT(A, 0);

	return 0;
}

u8 CPU::BIT_1_B()
{
	BIT(B, 1);

	return 0;
}

u8 CPU::BIT_1_C()
{
	BIT(C, 1);

	return 0;
}

u8 CPU::BIT_1_D()
{
	BIT(D, 1);

	return 0;
}

u8 CPU::BIT_1_E()
{
	BIT(E, 1);

	return 0;
}

u8 CPU::BIT_1_H()
{
	BIT(H, 1);

	return 0;
}

u8 CPU::BIT_1_L()
{
	BIT(L, 1);

	return 0;
}

u8 CPU::BIT_1_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 1);

	return 0;
}

u8 CPU::BIT_1_A()
{
	BIT(A, 1);

	return 0;
}

u8 CPU::BIT_2_B()
{
	BIT(B, 2);

	return 0;
}

u8 CPU::BIT_2_C()
{
	BIT(C, 2);

	return 0;
}

u8 CPU::BIT_2_D()
{
	BIT(D, 2);

	return 0;
}

u8 CPU::BIT_2_E()
{
	BIT(E, 2);

	return 0;
}

u8 CPU::BIT_2_H()
{
	BIT(H, 2);

	return 0;
}

u8 CPU::BIT_2_L()
{
	BIT(L, 2);

	return 0;
}

u8 CPU::BIT_2_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 2);

	return 0;
}

u8 CPU::BIT_2_A()
{
	BIT(A, 2);

	return 0;
}

u8 CPU::BIT_3_B()
{
	BIT(B, 3);

	return 0;
}

u8 CPU::BIT_3_C()
{
	BIT(C, 3);

	return 0;
}

u8 CPU::BIT_3_D()
{
	BIT(D, 3);

	return 0;
}

u8 CPU::BIT_3_E()
{
	BIT(E, 3);

	return 0;
}

u8 CPU::BIT_3_H()
{
	BIT(H, 3);

	return 0;
}

u8 CPU::BIT_3_L()
{
	BIT(L, 3);

	return 0;
}

u8 CPU::BIT_3_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 3);

	return 0;
}

u8 CPU::BIT_3_A()
{
	BIT(A, 3);

	return 0;
}

u8 CPU::BIT_4_B()
{
	BIT(B, 4);

	return 0;
}

u8 CPU::BIT_4_C()
{
	BIT(C, 4);

	return 0;
}

u8 CPU::BIT_4_D()
{
	BIT(D, 4);

	return 0;
}

u8 CPU::BIT_4_E()
{
	BIT(E, 4);

	return 0;
}

u8 CPU::BIT_4_H()
{
	BIT(H, 4);

	return 0;
}

u8 CPU::BIT_4_L()
{
	BIT(L, 4);

	return 0;
}

u8 CPU::BIT_4_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 4);

	return 0;
}

u8 CPU::BIT_4_A()
{
	BIT(A, 4);

	return 0;
}

u8 CPU::BIT_5_B()
{
	BIT(B, 5);

	return 0;
}

u8 CPU::BIT_5_C()
{
	BIT(C, 5);

	return 0;
}

u8 CPU::BIT_5_D()
{
	BIT(D, 5);

	return 0;
}

u8 CPU::BIT_5_E()
{
	BIT(E, 5);

	return 0;
}

u8 CPU::BIT_5_H()
{
	BIT(H, 5);

	return 0;
}

u8 CPU::BIT_5_L()
{
	BIT(L, 5);

	return 0;
}

u8 CPU::BIT_5_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 5);

	return 0;
}

u8 CPU::BIT_5_A()
{
	BIT(A, 5);

	return 0;
}

u8 CPU::BIT_6_B()
{
	BIT(B, 6);

	return 0;
}

u8 CPU::BIT_6_C()
{
	BIT(C, 6);

	return 0;
}

u8 CPU::BIT_6_D()
{
	BIT(D, 6);

	return 0;
}

u8 CPU::BIT_6_E()
{
	BIT(E, 6);

	return 0;
}

u8 CPU::BIT_6_H()
{
	BIT(H, 6);

	return 0;
}

u8 CPU::BIT_6_L()
{
	BIT(L, 6);

	return 0;
}

u8 CPU::BIT_6_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 6);

	return 0;
}

u8 CPU::BIT_6_A()
{
	BIT(A, 6);

	return 0;
}

u8 CPU::BIT_7_B()
{
	BIT(B, 7);

	return 0;
}

u8 CPU::BIT_7_C()
{
	BIT(C, 7);

	return 0;
}

u8 CPU::BIT_7_D()
{
	BIT(D, 7);

	return 0;
}

u8 CPU::BIT_7_E()
{
	BIT(E, 7);

	return 0;
}

u8 CPU::BIT_7_H()
{
	BIT(H, 7);

	return 0;
}

u8 CPU::BIT_7_L()
{
	BIT(L, 7);

	return 0;
}

u8 CPU::BIT_7_aHL()
{
	static u8 data = 0;
	data = Read(HL);
	BIT(data, 7);

	return 0;
}

u8 CPU::BIT_7_A()
{
	BIT(A, 7);

	return 0;
}

u8 CPU::RES_0_B()
{
	B &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_0_C()
{
	C &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_0_D()
{
	D &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_0_E()
{
	E &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_0_H()
{
	H &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_0_L()
{
	L &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_0_aHL()
{
	Write(HL, Read(HL) & ~(1 << 0));
	return 0;
}

u8 CPU::RES_0_A()
{
	A &= ~(1 << 0);
	return 0;
}

u8 CPU::RES_1_B()
{
	B &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_1_C()
{
	C &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_1_D()
{
	D &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_1_E()
{
	E &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_1_H()
{
	H &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_1_L()
{
	L &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_1_aHL()
{
	Write(HL, Read(HL) & ~(1 << 1));
	return 0;
}

u8 CPU::RES_1_A()
{
	A &= ~(1 << 1);
	return 0;
}

u8 CPU::RES_2_B()
{
	B &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_2_C()
{
	C &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_2_D()
{
	D &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_2_E()
{
	E &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_2_H()
{
	H &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_2_L()
{
	L &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_2_aHL()
{
	Write(HL, Read(HL) & ~(1 << 2));
	return 0;
}

u8 CPU::RES_2_A()
{
	A &= ~(1 << 2);
	return 0;
}

u8 CPU::RES_3_B()
{
	B &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_3_C()
{
	C &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_3_D()
{
	D &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_3_E()
{
	E &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_3_H()
{
	H &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_3_L()
{
	L &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_3_aHL()
{
	Write(HL, Read(HL) & ~(1 << 3));
	return 0;
}

u8 CPU::RES_3_A()
{
	A &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_4_B()
{
	B &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_4_C()
{
	C &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_4_D()
{
	D &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_4_E()
{
	E &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_4_H()
{
	H &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_4_L()
{
	L &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_4_aHL()
{
	Write(HL, Read(HL) & ~(1 << 4));
	return 0;
}

u8 CPU::RES_4_A()
{
	A &= ~(1 << 4);
	return 0;
}

u8 CPU::RES_5_B()
{
	B &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_5_C()
{
	C &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_5_D()
{
	D &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_5_E()
{
	E &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_5_H()
{
	H &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_5_L()
{
	L &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_5_aHL()
{
	Write(HL, Read(HL) & ~(1 << 5));
	return 0;
}

u8 CPU::RES_5_A()
{
	A &= ~(1 << 5);
	return 0;
}

u8 CPU::RES_6_B()
{
	B &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_6_C()
{
	C &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_6_D()
{
	D &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_6_E()
{
	E &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_6_H()
{
	H &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_6_L()
{
	L &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_6_aHL()
{
	Write(HL, Read(HL) & ~(1 << 6));
	return 0;
}

u8 CPU::RES_6_A()
{
	A &= ~(1 << 6);
	return 0;
}

u8 CPU::RES_7_B()
{
	B &= ~(1 << 7);
	return 0;
}

u8 CPU::RES_7_C()
{
	C &= ~(1 << 7);
	return 0;
}

u8 CPU::RES_7_D()
{
	D &= ~(1 << 7);
	return 0;
}

u8 CPU::RES_7_E()
{
	E &= ~(1 << 7);
	return 0;
}

u8 CPU::RES_7_H()
{
	H &= ~(1 << 7);
	return 0;
}

u8 CPU::RES_7_L()
{
	L &= ~(1 << 7);
	return 0;
}

u8 CPU::RES_7_aHL()
{
	Write(HL, Read(HL) & ~(1 << 7));
	return 0;
}

u8 CPU::RES_7_A()
{
	A &= ~(1 << 7);
	return 0;
}

u8 CPU::SET_0_B()
{
	B |= (1 << 0);
	return 0;
}

u8 CPU::SET_0_C()
{
	C |= (1 << 0);
	return 0;
}

u8 CPU::SET_0_D()
{
	D |= (1 << 0);
	return 0;
}

u8 CPU::SET_0_E()
{
	E |= (1 << 0);
	return 0;
}

u8 CPU::SET_0_H()
{
	H |= (1 << 0);
	return 0;
}

u8 CPU::SET_0_L()
{
	L |= (1 << 0);
	return 0;
}

u8 CPU::SET_0_aHL()
{
	Write(HL, Read(HL) | (1 << 0));
	return 0;
}

u8 CPU::SET_0_A()
{
	A |= (1 << 0);
	return 0;
}

u8 CPU::SET_1_B()
{
	B |= (1 << 1);
	return 0;
}

u8 CPU::SET_1_C()
{
	C |= (1 << 1);
	return 0;
}

u8 CPU::SET_1_D()
{
	D |= (1 << 1);
	return 0;
}

u8 CPU::SET_1_E()
{
	E |= (1 << 1);
	return 0;
}

u8 CPU::SET_1_H()
{
	H |= (1 << 1);
	return 0;
}

u8 CPU::SET_1_L()
{
	L |= (1 << 1);
	return 0;
}

u8 CPU::SET_1_aHL()
{
	Write(HL, Read(HL) | (1 << 1));
	return 0;
}

u8 CPU::SET_1_A()
{
	A |= (1 << 1);
	return 0;
}

u8 CPU::SET_2_B()
{
	B |= (1 << 2);
	return 0;
}

u8 CPU::SET_2_C()
{
	C |= (1 << 2);
	return 0;
}

u8 CPU::SET_2_D()
{
	D |= (1 << 2);
	return 0;
}

u8 CPU::SET_2_E()
{
	E |= (1 << 2);
	return 0;
}

u8 CPU::SET_2_H()
{
	H |= (1 << 2);
	return 0;
}

u8 CPU::SET_2_L()
{
	L |= (1 << 2);
	return 0;
}

u8 CPU::SET_2_aHL()
{
	Write(HL, Read(HL) | (1 << 2));
	return 0;
}

u8 CPU::SET_2_A()
{
	A |= (1 << 2);
	return 0;
}

u8 CPU::SET_3_B()
{
	B |= (1 << 3);
	return 0;
}

u8 CPU::SET_3_C()
{
	C |= (1 << 3);
	return 0;
}

u8 CPU::SET_3_D()
{
	D |= (1 << 3);
	return 0;
}

u8 CPU::SET_3_E()
{
	E |= (1 << 3);
	return 0;
}

u8 CPU::SET_3_H()
{
	H |= (1 << 3);
	return 0;
}

u8 CPU::SET_3_L()
{
	L |= (1 << 3);
	return 0;
}

u8 CPU::SET_3_aHL()
{
	Write(HL, Read(HL) | (1 << 3));
	return 0;
}

u8 CPU::SET_3_A()
{
	A |= (1 << 3);
	return 0;
}

u8 CPU::SET_4_B()
{
	B |= (1 << 4);
	return 0;
}

u8 CPU::SET_4_C()
{
	C |= (1 << 4);
	return 0;
}

u8 CPU::SET_4_D()
{
	D |= (1 << 4);
	return 0;
}

u8 CPU::SET_4_E()
{
	E |= (1 << 4);
	return 0;
}

u8 CPU::SET_4_H()
{
	H |= (1 << 4);
	return 0;
}

u8 CPU::SET_4_L()
{
	L |= (1 << 4);
	return 0;
}

u8 CPU::SET_4_aHL()
{
	Write(HL, Read(HL) | (1 << 4));
	return 0;
}

u8 CPU::SET_4_A()
{
	A |= (1 << 4);
	return 0;
}

u8 CPU::SET_5_B()
{
	B |= (1 << 5);
	return 0;
}

u8 CPU::SET_5_C()
{
	C |= (1 << 5);
	return 0;
}

u8 CPU::SET_5_D()
{
	D |= (1 << 5);
	return 0;
}

u8 CPU::SET_5_E()
{
	E |= (1 << 5);
	return 0;
}

u8 CPU::SET_5_H()
{
	H |= (1 << 5);
	return 0;
}

u8 CPU::SET_5_L()
{
	L |= (1 << 5);
	return 0;
}

u8 CPU::SET_5_aHL()
{
	Write(HL, Read(HL) | (1 << 5));
	return 0;
}

u8 CPU::SET_5_A()
{
	A |= (1 << 5);
	return 0;
}

u8 CPU::SET_6_B()
{
	B |= (1 << 6);
	return 0;
}

u8 CPU::SET_6_C()
{
	C |= (1 << 6);
	return 0;
}

u8 CPU::SET_6_D()
{
	D |= (1 << 6);
	return 0;
}

u8 CPU::SET_6_E()
{
	E |= (1 << 6);
	return 0;
}

u8 CPU::SET_6_H()
{
	H |= (1 << 6);
	return 0;
}

u8 CPU::SET_6_L()
{
	L |= (1 << 6);
	return 0;
}

u8 CPU::SET_6_aHL()
{
	Write(HL, Read(HL) | (1 << 6));
	return 0;
}

u8 CPU::SET_6_A()
{
	A |= (1 << 6);
	return 0;
}

u8 CPU::SET_7_B()
{
	B |= (1 << 7);
	return 0;
}

u8 CPU::SET_7_C()
{
	C |= (1 << 7);
	return 0;
}

u8 CPU::SET_7_D()
{
	D |= (1 << 7);
	return 0;
}

u8 CPU::SET_7_E()
{
	E |= (1 << 7);
	return 0;
}

u8 CPU::SET_7_H()
{
	H |= (1 << 7);
	return 0;
}

u8 CPU::SET_7_L()
{
	L |= (1 << 7);
	return 0;
}

u8 CPU::SET_7_aHL()
{
	Write(HL, Read(HL) | (1 << 7));
	return 0;
}

u8 CPU::SET_7_A()
{
	A |= (1 << 7);
	return 0;
}
#pragma endregion