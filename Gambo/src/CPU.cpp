#include "CPU.h"
#include "Bus.h"

CPU::CPU(Bus* b)
	: bus(b)
{
}

CPU::~CPU()
{
}

u8 CPU::Read(u16 addr)
{
	if ((bus->ppu.mode == PPU::Mode::OAMScan && (0xFE00 <= addr && addr <= 0xFE9F)) // accessing oam during oam scan
		|| (bus->ppu.mode == PPU::Mode::Draw && ((0xFE00 <= addr && addr <= 0xFE9F) || (0x8000 <= addr && addr <= 0x9FFF)))) // accessing oam or vram during drawing
	{
		return 0xFF;
	}
	else
	{
		return bus->Read(addr);
	}
}

void CPU::Write(u16 addr, u8 data)
{
    bus->Write(addr, data);
	if (addr == HWAddr::DMA)
	{
		bus->ppu.DoDMATransfer = true;
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



			static u8& IF = bus->ram[HWAddr::IF];
			static u8& IE = bus->ram[HWAddr::IE];

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
	static u8& IF = bus->ram[HWAddr::IF];

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
	static int DIVCounter = 0;

	// DIV register always counts up every 256 clock cycles
	if (DIVCounter == 256)
	{
		DIVCounter = 0;
		bus->ram[HWAddr::DIV]++;
	}
	else
	{
		DIVCounter++;
	}



	// TIMA iterates at a specified frequency and only if it's enabled.
	static int TIMACounter = 0;

	static u8& TAC = bus->ram[HWAddr::TAC];
	static bool TIMAEnabled;
	TIMAEnabled = TAC & 0b100;
	if (TIMAEnabled)
	{
		static u8 TIMAFreqSelect;
		TIMAFreqSelect = TAC & 0b011;

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
		if (TIMACounter == TIMAFreq)
		{
			TIMACounter = 0;
			static u8& TIMA = bus->ram[HWAddr::TIMA];
			if (TIMA == 0xFF)
			{
				// TIMA resets to TMA register value
				TIMA = bus->ram[HWAddr::TMA];
				// request interrupt
				bus->ram[HWAddr::IF] |= InterruptFlags::Timer;
			}
			else
			{
				TIMA++;
			}
		}
		else
		{
			TIMACounter++;
		}
	}
}

bool CPU::InterruptPending()
{
	return (bus->ram[HWAddr::IE] & bus->ram[HWAddr::IF]) != 0;
}

bool CPU::InstructionComplete()
{
	return cycles == 0;
}

void CPU::Reset()
{
	// this puts the cpu into back into a state as if it had just finished a legit boot sequence
	A = 0x01;
	SetFlag(fZ, 1);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 1);
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
	bus->ram[HWAddr::P1]	=	 0xCF;
	bus->ram[HWAddr::SB]	=	 0x00;
	bus->ram[HWAddr::SC]	=	 0x7E;
	bus->ram[HWAddr::DIV]	=	 0xAB;
	bus->ram[HWAddr::TIMA]	=	 0x00;
	bus->ram[HWAddr::TMA]	=	 0x00;
	bus->ram[HWAddr::TAC]	=	 0xF8;
	bus->ram[HWAddr::IF]	=	 0xE1;
	bus->ram[HWAddr::NR10]	=	 0x80;
	bus->ram[HWAddr::NR11]	=	 0xBF;
	bus->ram[HWAddr::NR12]	=	 0xF3;
	bus->ram[HWAddr::NR13]	=	 0xFF;
	bus->ram[HWAddr::NR14]	=	 0xBF;
	bus->ram[HWAddr::NR21]	=	 0x3F;
	bus->ram[HWAddr::NR22]	=	 0x00;
	bus->ram[HWAddr::NR23]	=	 0xFF;
	bus->ram[HWAddr::NR24]	=	 0xBF;
	bus->ram[HWAddr::NR30]	=	 0x7F;
	bus->ram[HWAddr::NR31]	=	 0xFF;
	bus->ram[HWAddr::NR32]	=	 0x9F;
	bus->ram[HWAddr::NR33]	=	 0xFF;
	bus->ram[HWAddr::NR34]	=	 0xBF;
	bus->ram[HWAddr::NR41]	=	 0xFF;
	bus->ram[HWAddr::NR42]	=	 0x00;
	bus->ram[HWAddr::NR43]	=	 0x00;
	bus->ram[HWAddr::NR44]	=	 0xBF;
	bus->ram[HWAddr::NR50]	=	 0x77;
	bus->ram[HWAddr::NR51]	=	 0xF3;
	bus->ram[HWAddr::NR52]	=	 0xF1;
	bus->ram[HWAddr::LCDC]	=	 0x91;
	bus->ram[HWAddr::STAT]	=	 0x85;
	bus->ram[HWAddr::SCY]	=	 0x00;
	bus->ram[HWAddr::SCX]	=	 0x00;
	bus->ram[HWAddr::LY]	=	 0x00;
	bus->ram[HWAddr::LYC]	=	 0x00;
	bus->ram[HWAddr::DMA]	=	 0xFF;
	bus->ram[HWAddr::BGP]	=	 0xFC;
	bus->ram[HWAddr::OBP0]	=	 0x00;
	bus->ram[HWAddr::OBP1]	=	 0x00;
	bus->ram[HWAddr::WY]	=	 0x00;
	bus->ram[HWAddr::WX]	=	 0x00;
	bus->ram[HWAddr::KEY1]	=	 0xFF;
	bus->ram[HWAddr::VBK]	=	 0xFF;
	bus->ram[HWAddr::HDMA1]	=	 0xFF;
	bus->ram[HWAddr::HDMA2]	=	 0xFF;
	bus->ram[HWAddr::HDMA3]	=	 0xFF;
	bus->ram[HWAddr::HDMA4]	=	 0xFF;
	bus->ram[HWAddr::HDMA5]	=	 0xFF;
	bus->ram[HWAddr::RP]	=	 0xFF;
	bus->ram[HWAddr::BCPS]	=	 0xFF;
	bus->ram[HWAddr::BCPD]	=	 0xFF;
	bus->ram[HWAddr::OCPS]	=	 0xFF;
	bus->ram[HWAddr::OCPD]	=	 0xFF;
	bus->ram[HWAddr::SVBK]	=	 0xFF;
	bus->ram[HWAddr::IE]	=	 0x00;
}

void CPU::SetFlag(Flags f, bool v)
{
	if (v)
		F |= f;
	else
		F &= ~f;
}

bool CPU::GetFlag(Flags f)
{
	return ((F & f) > 0)
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
	if (!GetFlag(fN)) {  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
		if (GetFlag(fC) || A > 0x99) 
		{ 
			A += 0x60;
			SetFlag(fC, 1); 
		}
		if (GetFlag(fH) || (A & 0x0F) > 0x09) 
		{ A += 0x6; }
	}
	else {  // after a subtraction, only adjust if (half-)carry occurred
		if (GetFlag(fC)) { A -= 0x60; }
		if (GetFlag(fH)) { A -= 0x6; }
	}

	// these flags are always updated
	SetFlag(fZ, A == 0); // the usual z flag
	SetFlag(fH, 0); // h flag is always cleared
	return 0;
}

u8 CPU::CPL()
{
	A = ~A;

	SetFlag(fN, 1);
	SetFlag(fH, 1);
	return 0;
}

u8 CPU::SCF()
{
	SetFlag(fC, 1);

	SetFlag(fN, 0);
	SetFlag(fH, 0);
	return 0;
}

u8 CPU::CCF()
{
	SetFlag(fC, !GetFlag(fC));

	SetFlag(fN, 0);
	SetFlag(fH, 0);
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
	IME = true; 

	return 0;
}
#pragma endregion
#pragma region Jump Instructions
u8 CPU::JR_s8()
{
	static s8 data;
	data = Read(PC++);
	PC += data;
	
	return 0;
}

u8 CPU::JR_NZ_s8()
{
	static s8 data;
	data = Read(PC++);
	if (!GetFlag(fZ))
	{
		PC += data;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::JR_Z_s8()
{
	static s8 data;
	data = Read(PC++);
	if (GetFlag(fZ))
	{
		PC += data;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::JR_NC_s8()
{
	static s8 data;
	data = Read(PC++);
	if (!GetFlag(fC))
	{
		PC += data;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::JR_C_s8()
{
	static s8 data;
	data = Read(PC++);
	if (GetFlag(fC))
	{
		PC += data;
		return 4;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RET_NZ()
{
	if (!GetFlag(fZ))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = (high << 8) | low;
	if (!GetFlag(fZ))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	PC = high << 8 | low;
	return 0;
}

u8 CPU::CALL_NZ_a16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = high << 8 | low;
	if (!GetFlag(fZ))
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
	if (GetFlag(fZ))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = (high << 8) | low;
	if (GetFlag(fZ))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = high << 8 | low;
	if (GetFlag(fZ))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = high << 8 | low;
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
	if (!GetFlag(fC))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = (high << 8) | low;
	if (!GetFlag(fC))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = high << 8 | low;
	if (!GetFlag(fC))
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
	if (GetFlag(fC))
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
	u16 low = Read(SP--);
	u16 high = Read(SP--);

	PC = high << 8 | low;
	IME = true; 

	return 0;
}

u8 CPU::JP_C_a16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = (high << 8) | low;
	if (GetFlag(fC))
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	u16 addr = high << 8 | low;
	if (GetFlag(fZ))
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
	Write(HL, A);
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
	A = Read(C);
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

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

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

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (B & 0xF) < (prev & 0xF));
	
	return 0;
}

u8 CPU::DEC_B()
{
	B--;

	SetFlag(fZ, B == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (B & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_C()
{
	static int prev;
	prev = C;
	C++;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (C & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_C()
{
	C--;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (C & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_D()
{
	static int prev;
	prev = D;
	D++;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (D & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_D()
{
	D--;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (D & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_E()
{
	static int prev;
	prev = E;
	E++;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (E & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_E()
{
	E--;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (E & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_H()
{
	static int prev;
	prev = H;
	H++;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (H & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_H()
{
	H--;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (H & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_L()
{
	static int prev;
	prev = L;
	L++;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (L & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_L()
{
	L--;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (L & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_aHL()
{
	static int prev;
	prev = Read(HL);
	Write(HL, prev + 1);

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (Read(HL) & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_aHL()
{
	static int data;
	data = Read(HL);
	Write(HL, --data);

	SetFlag(fZ, data == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (data & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_A()
{
	static int prev;
	prev = A;
	A++;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_A()
{
	A--;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) == 0xF);

	return 0;
}

u8 CPU::ADD_A_B()
{
	int sum = A + B;
	int noCarrySum = A ^ B;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += B;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_C()
{
	int sum = A + C;
	int noCarrySum = A ^ C;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += C;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_D()
{
	int sum = A + D;
	int noCarrySum = A ^ D;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += D;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_E()
{
	int sum = A + E;
	int noCarrySum = A ^ E;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += E;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_H()
{
	int sum = A + H;
	int noCarrySum = A ^ H;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += H;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_L()
{
	int sum = A + L;
	int noCarrySum = A ^ L;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += L;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_aHL()
{
	int data = Read(HL);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADD_A_A()
{
	int sum = A + A;
	int noCarrySum = A ^ A;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += A;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_B()
{
	int data = B + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_C()
{
	int data = C + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_D()
{
	int data = D + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_E()
{
	int data = E + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_H()
{
	int data = H + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_L()
{
	int data = L + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_aHL()
{
	int data = Read(HL) + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_A()
{
	int data = A + GetFlag(fC);
	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_B()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (B & 0xF));
	SetFlag(fC, A < B);

	A -= B;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_C()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (C & 0xF));
	SetFlag(fC, A < C);

	A -= C;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_D()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (D & 0xF));
	SetFlag(fC, A < D);

	A -= D;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_E()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (E & 0xF));
	SetFlag(fC, A < E);

	A -= E;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_H()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (H & 0xF));
	SetFlag(fC, A < H);

	A -= H;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_L()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (L & 0xF));
	SetFlag(fC, A < L);

	A -= L;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_aHL()
{
	static u8 data;
	data = Read(HL);

	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF));
	SetFlag(fC, A < data);

	A -= data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_A()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (A & 0xF));
	SetFlag(fC, A < A);

	A -= A;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_B()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (B & 0xF) + GetFlag(fC));
	SetFlag(fC, A < B + GetFlag(fC));

	A -= B - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_C()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (C & 0xF) + GetFlag(fC));
	SetFlag(fC, A < C + GetFlag(fC));

	A -= C - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_D()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (D & 0xF) + GetFlag(fC));
	SetFlag(fC, A < D + GetFlag(fC));

	A -= D - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_E()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (E & 0xF) + GetFlag(fC));
	SetFlag(fC, A < E + GetFlag(fC));

	A -= E - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_H()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (H & 0xF) + GetFlag(fC));
	SetFlag(fC, A < H + GetFlag(fC));

	A -= H - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_L()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (L & 0xF) + GetFlag(fC));
	SetFlag(fC, A < L + GetFlag(fC));

	A -= L - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_aHL()
{
	static u8 data;
	data = Read(HL);

	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF) + GetFlag(fC));
	SetFlag(fC, A < data + GetFlag(fC));

	A -= data - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_A()
{
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (A & 0xF) + GetFlag(fC));
	SetFlag(fC, A < A + GetFlag(fC));

	A -= A - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::AND_A_B()
{
	A &= B;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_C()
{
	A &= C;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_D()
{
	A &= D;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_E()
{
	A &= E;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_H()
{
	A &= H;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_L()
{
	A &= L;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_aHL()
{
	A &= Read(HL);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::AND_A_A()
{
	A &= A;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_B()
{
	A ^= B;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_C()
{
	A ^= C;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_D()
{
	A ^= D;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_E()
{
	A ^= E;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_H()
{
	A ^= H;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_L()
{
	A ^= L;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_aHL()
{
	A ^= Read(HL);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_A()
{
	A ^= A;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_B()
{
	A |= B;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_C()
{
	A |= C;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_D()
{
	A |= D;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_E()
{
	A |= E;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_H()
{
	A |= H;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_L()
{
	A |= L;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_aHL()
{
	A |= Read(HL);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_A()
{
	A |= A;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::CP_A_B()
{
	static u16 result;
	result = A - B;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (B & 0xF));
	SetFlag(fC, A < B);

	return 0;
}

u8 CPU::CP_A_C()
{
	static u16 result;
	result = A - C;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (C & 0xF));
	SetFlag(fC, A < C);

	return 0;
}

u8 CPU::CP_A_D()
{
	static u16 result;
	result = A - D;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (D & 0xF));
	SetFlag(fC, A < D);

	return 0;
}

u8 CPU::CP_A_E()
{
	static u16 result;
	result = A - E;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (E & 0xF));
	SetFlag(fC, A < E);

	return 0;
}

u8 CPU::CP_A_H()
{
	static u16 result;
	result = A - H;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (H & 0xF));
	SetFlag(fC, A < H);

	return 0;
}

u8 CPU::CP_A_L()
{
	static u16 result;
	result = A - L;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (L & 0xF));
	SetFlag(fC, A < L);

	return 0;
}

u8 CPU::CP_A_aHL()
{
	static u16 result;
	static u8 data;
	data = Read(HL);
	result = A - data;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF));
	SetFlag(fC, A < data);

	return 0;
}

u8 CPU::CP_A_A()
{
	static s16 result;
	result = A - A;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (A & 0xF));
	SetFlag(fC, A < A);

	return 0;
}

u8 CPU::ADD_A_d8()
{
	static u8 data;
	data = Read(PC++);

	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::ADC_A_d8()
{
	int data = Read(PC++) + GetFlag(fC);

	int sum = A + data;
	int noCarrySum = A ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

	A += data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SUB_A_d8()
{
	static u8 data;
	data = Read(PC++);

	SetFlag(fN, 1);
	SetFlag(fC, A < data);
	SetFlag(fH, (A & 0xF) < (data & 0xF));

	A -= data;
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::SBC_A_d8()
{
	static u8 data;
	data = Read(PC++);
	
	SetFlag(fN, 1);
	SetFlag(fC, A < data + GetFlag(fC));
	SetFlag(fH, (A & 0xF) < (data & 0xF) + GetFlag(fC));

	A -= data - GetFlag(fC);
	SetFlag(fZ, A == 0);
	return 0;
}

u8 CPU::AND_A_d8()
{
	A &= Read(PC++);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 1);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::XOR_A_d8()
{
	A ^= Read(PC++);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::OR_A_d8()
{
	A |= Read(PC++);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::CP_A_d8()
{
	static u16 result;
	static u8 data;
	data = Read(PC++);
	result = A - data;

	SetFlag(fZ, result == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF));
	SetFlag(fC, A < data);

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

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x100);
	SetFlag(fC, carryBits & 0x10000);

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

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x100);
	SetFlag(fC, carryBits & 0x10000);

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

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x1000);
	SetFlag(fC, carryBits & 0x10000);

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

	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x1000);
	SetFlag(fC, carryBits & 0x10000);

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
	int data = Read(PC++);
	int sum = SP + data;
	int noCarrySum = SP ^ data;
	int carryBits = sum ^ noCarrySum; // this sets all bits that carried

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, carryBits & 0x10);
	SetFlag(fC, carryBits & 0x100);

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

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RRCA()
{
	static u8 bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | (bit0 << 7);

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RLA()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RRA()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | ((u8)GetFlag(fC) << 7);

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RLC_B()
{
	static bool bit7;
	bit7 = B & 0b10000000;

	B = (B << 1) | (u8)bit7;
	
	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);
	
	return 0;
}

u8 CPU::RLC_C()
{
	static bool bit7;
	bit7 = C & 0b10000000;

	C = (C << 1) | (u8)bit7;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RLC_D()
{
	static bool bit7;
	bit7 = D & 0b10000000;

	D = (D << 1) | (u8)bit7;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RLC_E()
{
	static bool bit7;
	bit7 = E & 0b10000000;

	E = (E << 1) | (u8)bit7;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RLC_H()
{
	static bool bit7;
	bit7 = H & 0b10000000;

	H = (H << 1) | (u8)bit7;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RLC_L()
{
	static bool bit7;
	bit7 = L & 0b10000000;

	L = (L << 1) | (u8)bit7;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RLC_aHL()
{
	static bool bit7;
	static u8 data;
	data = Read(HL);
	bit7 = data & 0b10000000;

	Write(HL, (data << 1) | (u8)bit7);

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RLC_A()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)bit7;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RRC_B()
{
	static u8 bit0;
	bit0 = B & 0b00000001;

	B = (B >> 1) | (bit0 << 7);
	
	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_C()
{
	static u8 bit0;
	bit0 = C & 0b00000001;

	C = (C >> 1) | (bit0 << 7);

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_D()
{
	static u8 bit0;
	bit0 = D & 0b00000001;

	D = (D >> 1) | (bit0 << 7);

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_E()
{
	static u8 bit0;
	bit0 = E & 0b00000001;

	E = (E >> 1) | (bit0 << 7);

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_H()
{
	static u8 bit0;
	bit0 = H & 0b00000001;

	H = (H >> 1) | (bit0 << 7);

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_L()
{
	static u8 bit0;
	bit0 = L & 0b00000001;

	L = (L >> 1) | (bit0 << 7);

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_aHL()
{
	static u8 bit0;
	static u8 data;
	data = Read(HL);
	bit0 = data & 0b00000001;

	Write(HL, (data >> 1) | (bit0 << 7));

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RRC_A()
{
	static u8 bit0;
	bit0 = L & 0b00000001;

	L = (L >> 1) | (bit0 << 7);

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RL_B()
{
	static bool bit7;
	bit7 = B & 0b10000000;

	B = (B << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_C()
{
	static bool bit7;
	bit7 = C & 0b10000000;

	C = (C << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_D()
{
	static bool bit7;
	bit7 = D & 0b10000000;

	D = (D << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_E()
{
	static bool bit7;
	bit7 = E & 0b10000000;

	E = (E << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_H()
{
	static bool bit7;
	bit7 = H & 0b10000000;

	H = (H << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_L()
{
	static bool bit7;
	bit7 = L & 0b10000000;

	L = (L << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_aHL()
{
	static bool bit7;
	static u8 data;
	data = Read(HL);
	bit7 = data & 0b10000000;

	Write(HL, (data << 1) | (u8)GetFlag(fC));

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RL_A()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A = (A << 1) | (u8)GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::RR_B()
{
	static bool bit0;
	bit0 = B & 0b00000001;

	B = (B >> 1) | (u8)GetFlag(fC);

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_C()
{
	static bool bit0;
	bit0 = C & 0b00000001;

	C = (C >> 1) | (u8)GetFlag(fC) << 7;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_D()
{
	static bool bit0;
	bit0 = D & 0b00000001;

	D = (D >> 1) | (u8)GetFlag(fC) << 7;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_E()
{
	static bool bit0;
	bit0 = E & 0b00000001;

	E = (E >> 1) | (u8)GetFlag(fC) << 7;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_H()
{
	static bool bit0;
	bit0 = H & 0b00000001;

	H = (H >> 1) | (u8)GetFlag(fC) << 7;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_L()
{
	static bool bit0;
	bit0 = L & 0b00000001;

	L = (L >> 1) | (u8)GetFlag(fC) << 7;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_aHL()
{
	static bool bit0;
	static u8 data;
	data = Read(HL);
	bit0 = data & 0b00000001;

	Write(HL, (data >> 1) | (u8)GetFlag(fC) << 7);

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::RR_A()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	A = (A >> 1) | (u8)GetFlag(fC) << 7;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SLA_B()
{
	static bool bit7;
	bit7 = B & 0b10000000;

	B <<= 1;

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_C()
{
	static bool bit7;
	bit7 = C & 0b10000000;

	C <<= 1;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_D()
{
	static bool bit7;
	bit7 = D & 0b10000000;

	D <<= 1;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_E()
{
	static bool bit7;
	bit7 = E & 0b10000000;

	E <<= 1;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_H()
{
	static bool bit7;
	bit7 = H & 0b10000000;

	H <<= 1;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_L()
{
	static bool bit7;
	bit7 = L & 0b10000000;

	L <<= 1;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_aHL()
{
	static bool bit7;
	static u8 data;
	data = Read(HL);
	bit7 = data & 0b10000000;

	Write(HL, data << 1);

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SLA_A()
{
	static bool bit7;
	bit7 = A & 0b10000000;

	A <<= 1;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	return 0;
}

u8 CPU::SRA_B()
{
	static bool bit0;
	static bool bit7;
	bit0 = B & 0b00000001;
	bit7 = B & 0b10000000;

	B = (B >> 1) | (bit7 << 7);

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_C()
{
	static bool bit0;
	static bool bit7;
	bit0 = C & 0b00000001;
	bit7 = C & 0b10000000;

	C = (C >> 1) | (bit7 << 7);

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_D()
{
	static bool bit0;
	static bool bit7;
	bit0 = D & 0b00000001;
	bit7 = D & 0b10000000;

	D = (D >> 1) | (bit7 << 7);

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_E()
{
	static bool bit0;
	static bool bit7;
	bit0 = E & 0b00000001;
	bit7 = E & 0b10000000;

	E = (E >> 1) | (bit7 << 7);

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_H()
{
	static bool bit0;
	static bool bit7;
	bit0 = H & 0b00000001;
	bit7 = H & 0b10000000;

	H = (H >> 1) | (bit7 << 7);

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_L()
{
	static bool bit0;
	static bool bit7;
	bit0 = L & 0b00000001;
	bit7 = L & 0b10000000;

	L = (L >> 1) | (bit7 << 7);

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_aHL()
{
	static u8 data;
	static bool bit0;
	static bool bit7;
	bit0 = B & 0b00000001;
	bit7 = B & 0b10000000;
	data = Read(HL);

	Write(HL, (data >> 1) | (bit7 << 7));

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRA_A()
{
	static bool bit0;
	static bool bit7;
	bit0 = A & 0b00000001;
	bit7 = A & 0b10000000;

	A = (A >> 1) | (bit7 << 7);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SWAP_B()
{
	static u8 low;
	static u8 high;
	low = B & 0b00001111;
	high = B >> 4;

	B = (low << 4) | high;

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SWAP_C()
{
	static u8 low;
	static u8 high;
	low = C & 0b00001111;
	high = C >> 4;

	C = (low << 4) | high;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SWAP_D()
{
	static u8 low;
	static u8 high;
	low = D & 0b00001111;
	high = D >> 4;

	D = (low << 4) | high;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SWAP_E()
{
	static u8 low;
	static u8 high;
	low = E & 0b00001111;
	high = E >> 4;

	E = (low << 4) | high;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SWAP_H()
{
	static u8 low;
	static u8 high;
	low = H & 0b00001111;
	high = H >> 4;

	H = (low << 4) | high;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SWAP_L()
{
	static u8 low;
	static u8 high;
	low = L & 0b00001111;
	high = L >> 4;

	L = (low << 4) | high;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

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

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SWAP_A()
{
	static u8 low;
	static u8 high;
	low = A & 0b00001111;
	high = A >> 4;

	A = (low << 4) | high;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, 0);

	return 0;
}

u8 CPU::SRL_B()
{
	static bool bit0;
	bit0 = B & 0b00000001;

	B >>= 1;

	SetFlag(fZ, B == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_C()
{
	static bool bit0;
	bit0 = C & 0b00000001;

	C >>= 1;

	SetFlag(fZ, C == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_D()
{
	static bool bit0;
	bit0 = D & 0b00000001;

	D >>= 1;

	SetFlag(fZ, D == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_E()
{
	static bool bit0;
	bit0 = E & 0b00000001;

	E >>= 1;

	SetFlag(fZ, E == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_H()
{
	static bool bit0;
	bit0 = H & 0b00000001;

	H >>= 1;

	SetFlag(fZ, H == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_L()
{
	static bool bit0;
	bit0 = L & 0b00000001;

	L >>= 1;

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_aHL()
{
	static u8 data;
	static bool bit0;
	data = Read(HL);
	bit0 = data & 0b00000001;

	Write(HL, data >> 1);

	SetFlag(fZ, Read(HL) == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SRL_A()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	A >>= 1;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::BIT_0_B()
{
	SetFlag(fZ, B & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_C()
{
	SetFlag(fZ, C & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_D()
{
	SetFlag(fZ, D & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_E()
{
	SetFlag(fZ, E & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_H()
{
	SetFlag(fZ, H & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_L()
{
	SetFlag(fZ, L & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_0_A()
{
	SetFlag(fZ, A & (1 << 0));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_B()
{
	SetFlag(fZ, B & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_C()
{
	SetFlag(fZ, C & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_D()
{
	SetFlag(fZ, D & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_E()
{
	SetFlag(fZ, E & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_H()
{
	SetFlag(fZ, H & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_L()
{
	SetFlag(fZ, L & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_1_A()
{
	SetFlag(fZ, A & (1 << 1));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_B()
{
	SetFlag(fZ, B & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_C()
{
	SetFlag(fZ, C & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_D()
{
	SetFlag(fZ, D & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_E()
{
	SetFlag(fZ, E & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_H()
{
	SetFlag(fZ, H & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_L()
{
	SetFlag(fZ, L & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_2_A()
{
	SetFlag(fZ, A & (1 << 2));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_B()
{
	SetFlag(fZ, B & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_C()
{
	SetFlag(fZ, C & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_D()
{
	SetFlag(fZ, D & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_E()
{
	SetFlag(fZ, E & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_H()
{
	SetFlag(fZ, H & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_L()
{
	SetFlag(fZ, L & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_3_A()
{
	SetFlag(fZ, A & (1 << 3));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_B()
{
	SetFlag(fZ, B & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_C()
{
	SetFlag(fZ, C & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_D()
{
	SetFlag(fZ, D & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_E()
{
	SetFlag(fZ, E & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_H()
{
	SetFlag(fZ, H & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_L()
{
	SetFlag(fZ, L & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_4_A()
{
	SetFlag(fZ, A & (1 << 4));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_B()
{
	SetFlag(fZ, B & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_C()
{
	SetFlag(fZ, C & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_D()
{
	SetFlag(fZ, D & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_E()
{
	SetFlag(fZ, E & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_H()
{
	SetFlag(fZ, H & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_L()
{
	SetFlag(fZ, L & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_5_A()
{
	SetFlag(fZ, A & (1 << 5));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_B()
{
	SetFlag(fZ, B & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_C()
{
	SetFlag(fZ, C & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_D()
{
	SetFlag(fZ, D & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_E()
{
	SetFlag(fZ, E & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_H()
{
	SetFlag(fZ, H & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_L()
{
	SetFlag(fZ, L & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_6_A()
{
	SetFlag(fZ, A & (1 << 6));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_B()
{
	SetFlag(fZ, B & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_C()
{
	SetFlag(fZ, C & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_D()
{
	SetFlag(fZ, D & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_E()
{
	SetFlag(fZ, E & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_H()
{
	SetFlag(fZ, H & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_L()
{
	SetFlag(fZ, L & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_aHL()
{
	SetFlag(fZ, Read(HL) & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

	return 0;
}

u8 CPU::BIT_7_A()
{
	SetFlag(fZ, A & (1 << 7));
	SetFlag(fN, 0);
	SetFlag(fH, 1);

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
	C &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_4_D()
{
	D &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_4_E()
{
	E &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_4_H()
{
	H &= ~(1 << 3);
	return 0;
}

u8 CPU::RES_4_L()
{
	L &= ~(1 << 3);
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