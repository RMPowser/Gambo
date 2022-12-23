#include <format>
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
    return bus->Read(addr);
}

void CPU::Write(u16 addr, u8 data)
{
    bus->Write(addr, data);
	if (addr == HWAddr::DMA)
	{
		bus->ppu.DoDMATransfer = true;
	}
}

u8 CPU::Clock()
{
	static u8 cycles = 0;

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

	// return the number of cycles the instruction took.
	return cycles;
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

	// this is what the hardware registers look like at PC = 0x0100
	Write(HWAddr::P1,	 0xCF);
	Write(HWAddr::SB,	 0x00);
	Write(HWAddr::SC,	 0x7E);
	Write(HWAddr::DIV,	 0xAB);
	Write(HWAddr::TIMA,	 0x00);
	Write(HWAddr::TMA,	 0x00);
	Write(HWAddr::TAC,	 0xF8);
	Write(HWAddr::IF,	 0xE1);
	Write(HWAddr::NR10,	 0x80);
	Write(HWAddr::NR11,	 0xBF);
	Write(HWAddr::NR12,	 0xF3);
	Write(HWAddr::NR13,	 0xFF);
	Write(HWAddr::NR14,	 0xBF);
	Write(HWAddr::NR21,	 0x3F);
	Write(HWAddr::NR22,	 0x00);
	Write(HWAddr::NR23,	 0xFF);
	Write(HWAddr::NR24,	 0xBF);
	Write(HWAddr::NR30,	 0x7F);
	Write(HWAddr::NR31,	 0xFF);
	Write(HWAddr::NR32,	 0x9F);
	Write(HWAddr::NR33,	 0xFF);
	Write(HWAddr::NR34,	 0xBF);
	Write(HWAddr::NR41,	 0xFF);
	Write(HWAddr::NR42,	 0x00);
	Write(HWAddr::NR43,	 0x00);
	Write(HWAddr::NR44,	 0xBF);
	Write(HWAddr::NR50,	 0x77);
	Write(HWAddr::NR51,	 0xF3);
	Write(HWAddr::NR52,	 0xF1);
	Write(HWAddr::LCDC,	 0x91);
	Write(HWAddr::STAT,	 0x85);
	Write(HWAddr::SCY,	 0x00);
	Write(HWAddr::SCX,	 0x00);
	Write(HWAddr::LY,	 0x00);
	Write(HWAddr::LYC,	 0x00);
	Write(HWAddr::DMA,	 0xFF);
	Write(HWAddr::BGP,	 0xFC);
	Write(HWAddr::OBP0,	 0x00);
	Write(HWAddr::OBP1,	 0x00);
	Write(HWAddr::WY,	 0x00);
	Write(HWAddr::WX,	 0x00);
	Write(HWAddr::KEY1,	 0xFF);
	Write(HWAddr::VBK,	 0xFF);
	Write(HWAddr::HDMA1,	 0xFF);
	Write(HWAddr::HDMA2,	 0xFF);
	Write(HWAddr::HDMA3,	 0xFF);
	Write(HWAddr::HDMA4,	 0xFF);
	Write(HWAddr::HDMA5,	 0xFF);
	Write(HWAddr::RP,	 0xFF);
	Write(HWAddr::BCPS,	 0xFF);
	Write(HWAddr::BCPD,	 0xFF);
	Write(HWAddr::OCPS,	 0xFF);
	Write(HWAddr::OCPD,	 0xFF);
	Write(HWAddr::SVBK,	 0xFF);
	Write(HWAddr::IE,	 0x00);
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

void CPU::Push(u8 data)
{
	Write(--SP, data);
}

u8 CPU::Pop()
{
	return Read(SP++);
}

void CPU::PushPC()
{
	Push(PC & 0xFF);
	Push(PC >> 8);
}

u16 CPU::PopPC()
{
	static u16 high;
	static u16 low;
	high = Pop();
	low = Pop();

	u16 result = (high << 8) | low;
	return result;
}

std::map<u16, std::string> CPU::Disassemble(u16 startAddr, u16 endAddr)
{
	u32 addr = startAddr;
	u8 value = 0x00, lo = 0x00, hi = 0x00;
	std::map<u16, std::string> mapLines;
	u16 lineAddr = 0;

	auto hex = [](uint32_t n, uint8_t d)
	{
		std::string s(d, '0');
		for (int i = d - 1; i >= 0; i--, n >>= 4)
			s[i] = "0123456789ABCDEF"[n & 0xF];
		return s;
	};

	while (addr <= (u32)endAddr)
	{
		lineAddr = addr;

		// prefix line with instruction addr
		std::string s = "$" + hex(addr, 4) + ": ";

		// read instruction and get readable name
		u8 opcode = bus->Read(addr++);
		if (opcode == 0xCB)
		{
			// its a 16bit opcode so read another byte
			opcode = Read(addr++);

			s += instructions16bit[opcode].mnemonic;
		}
		else
		{
			auto& instruction = instructions8bit[opcode];
			if (instruction.bytes == 2)
			{
				u8 data = Read(addr++);
				s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 2)));
			}
			else if (instruction.bytes == 3)
			{
				u16 lo = Read(addr++);
				u16 hi = Read(addr++);
				u16 data = (hi << 8) | lo;
				s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 4)));
			}
			else
			{
				s += instructions8bit[opcode].mnemonic;
			}
		}


		mapLines[lineAddr] = s;
	}

	return mapLines;
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
		PC = PopPC();
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
		PushPC();
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
	PushPC();

	PC = 0x0000;
	return 0;
}

u8 CPU::RET_Z()
{
	if (GetFlag(fZ))
	{
		PC = PopPC();
		return 12;
	}
	else
	{
		return 0;
	}
}

u8 CPU::RET()
{
	PC = PopPC();
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
		PushPC();
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
	PushPC();
	PC = addr;

	return 0;
}

u8 CPU::RST_1()
{
	PushPC();
	PC = 0x0008;
	return 0;
}

u8 CPU::RET_NC()
{
	if (!GetFlag(fC))
	{
		PC = PopPC();
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
		PushPC();
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
	PushPC();
	PC = 0x0010;
	return 0;
}

u8 CPU::RET_C()
{
	if (GetFlag(fC))
	{
		PC = PopPC();
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
		PushPC();
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
	PushPC();
	PC = 0x0018;
	return 0;
}

u8 CPU::RST_4()
{
	PushPC();
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
	PushPC();
	PC = 0x0028;
	return 0;
}

u8 CPU::RST_6()
{
	PushPC();
	PC = 0x0030;
	return 0;
}

u8 CPU::RST_7()
{
	PushPC();
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
	A = Read(Read(PC++));
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
	u8 spLow = u8(SP | 0xFF);
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
	C = Pop();
	B = Pop();
	return 0;
}

u8 CPU::PUSH_BC()
{
	Push(B);
	Push(C);
	return 0;
}

u8 CPU::POP_DE()
{
	E = Pop();
	D = Pop();
	return 0;
}

u8 CPU::PUSH_DE()
{
	Push(D);
	Push(E);
	return 0;
}

u8 CPU::POP_HL()
{
	L = Pop();
	H = Pop();
	return 0;
}

u8 CPU::PUSH_HL()
{
	Push(H);
	Push(L);
	return 0;
}

u8 CPU::POP_AF()
{
	F = Pop();
	F &= ~(0b00001111);
	A = Pop();
	return 0;
}

u8 CPU::PUSH_AF()
{
	Push(A);
	Push(F);
	return 0;
}

u8 CPU::LD_HL_SPinc_s8()
{
	static s8 data;
	static s16 result8;
	static int result16;

	data = Read(PC++);
	result8 = (SP & 0xF) + (data & 0xF);
	result16 = SP + data;
	HL = SP + data;

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, result8 > 0xFF);
	SetFlag(fC, result16 > 0xFFFF);

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
	SetFlag(fH, (B & 0xF) != 0);

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
	SetFlag(fH, (C & 0xF) != 0);

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
	SetFlag(fH, (D & 0xF) != 0);

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
	SetFlag(fH, (E & 0xF) != 0);

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
	SetFlag(fH, (H & 0xF) != 0);

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
	SetFlag(fH, (L & 0xF) != 0);

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
	SetFlag(fH, (data & 0xF) != 0);

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
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0xF) != 0);

	return 0;
}

u8 CPU::ADD_A_B()
{
	static u16 result;
	result = A + B;
	A += B;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_C()
{
	static u16 result;
	result = A + C;
	A += C;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_D()
{
	static u16 result;
	result = A + D;
	A += D;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_E()
{
	static u16 result;
	result = A + E;
	A += E;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_H()
{
	static u16 result;
	result = A + H;
	A += H;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_L()
{
	static u16 result;
	result = A + L;
	A += L;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_aHL()
{
	static u16 result;
	result = A + Read(HL);
	A += Read(HL);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADD_A_A()
{
	static u16 result;
	result = A + A;
	A += A;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_B()
{
	static u16 result;
	result = A + B + GetFlag(fC);
	A += B + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_C()
{
	static u16 result;
	result = A + C + GetFlag(fC);
	A += C + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_D()
{
	static u16 result;
	result = A + D + GetFlag(fC);
	A += D + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_E()
{
	static u16 result;
	result = A + E + GetFlag(fC);
	A += E + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_H()
{
	static u16 result;
	result = A + H + GetFlag(fC);
	A += H + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_L()
{
	static u16 result;
	result = A + L + GetFlag(fC);
	A += L + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_aHL()
{
	static u16 result;
	result = A + Read(HL) + GetFlag(fC);
	A += Read(HL) + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::ADC_A_A()
{
	static u16 result;
	result = A + A + GetFlag(fC);
	A += A + GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::SUB_A_B()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (B & 0xF));
	SetFlag(fC, A < B);

	A -= B;
	return 0;
}

u8 CPU::SUB_A_C()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (C & 0xF));
	SetFlag(fC, A < C);

	A -= C;
	return 0;
}

u8 CPU::SUB_A_D()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (D & 0xF));
	SetFlag(fC, A < D);

	A -= D;
	return 0;
}

u8 CPU::SUB_A_E()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (E & 0xF));
	SetFlag(fC, A < E);

	A -= E;
	return 0;
}

u8 CPU::SUB_A_H()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (H & 0xF));
	SetFlag(fC, A < H);

	A -= H;
	return 0;
}

u8 CPU::SUB_A_L()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (L & 0xF));
	SetFlag(fC, A < L);

	A -= L;
	return 0;
}

u8 CPU::SUB_A_aHL()
{
	static u8 data;
	data = Read(HL);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF));
	SetFlag(fC, A < data);

	A -= data;
	return 0;
}

u8 CPU::SUB_A_A()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (A & 0xF));
	SetFlag(fC, A < A);

	A -= A;
	return 0;
}

u8 CPU::SBC_A_B()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (B & 0xF) + GetFlag(fC));
	SetFlag(fC, A < B + GetFlag(fC));

	A -= B - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_C()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (C & 0xF) + GetFlag(fC));
	SetFlag(fC, A < C + GetFlag(fC));

	A -= C - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_D()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (D & 0xF) + GetFlag(fC));
	SetFlag(fC, A < D + GetFlag(fC));

	A -= D - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_E()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (E & 0xF) + GetFlag(fC));
	SetFlag(fC, A < E + GetFlag(fC));

	A -= E - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_H()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (H & 0xF) + GetFlag(fC));
	SetFlag(fC, A < H + GetFlag(fC));

	A -= H - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_L()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (L & 0xF) + GetFlag(fC));
	SetFlag(fC, A < L + GetFlag(fC));

	A -= L - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_aHL()
{
	static u8 data;
	data = Read(HL);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF) + GetFlag(fC));
	SetFlag(fC, A < data + GetFlag(fC));

	A -= data - GetFlag(fC);
	return 0;
}

u8 CPU::SBC_A_A()
{
	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (A & 0xF) + GetFlag(fC));
	SetFlag(fC, A < A + GetFlag(fC));

	A -= A - GetFlag(fC);
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
	static u16 result;
	static u8 data;
	data = Read(PC++);
	result = A + data;
	A += data;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;

	return 0;
}

u8 CPU::ADC_A_d8()
{
	static u16 result;
	static u8 data;
	data = Read(PC++);
	result = A + data;
	A += data;

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, (A & 0b00010000));
	SetFlag(fC, (result & 0b0000000100000000));
	return 0;
}

u8 CPU::SUB_A_d8()
{
	static u8 data;
	data = Read(PC++);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF));
	SetFlag(fC, A < data);

	A -= data;
	return 0;
}

u8 CPU::SBC_A_d8()
{
	static u8 data;
	data = Read(PC++);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 1);
	SetFlag(fH, (A & 0xF) < (data & 0xF) + GetFlag(fC));
	SetFlag(fC, A < data + GetFlag(fC));

	A -= data - GetFlag(fC);
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
	SetFlag(fN, 0);
	SetFlag(fH, ((HL & 0xFF) + (BC & 0xFF)) > 0xFF);
	SetFlag(fC, ((int)HL + (int)BC) > 0xFFFF);
	
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
	SetFlag(fN, 0);
	SetFlag(fH, ((HL & 0xFF) + (DE & 0xFF)) > 0xFF);
	SetFlag(fC, ((int)HL + (int)DE) > 0xFFFF);

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
	SetFlag(fN, 0);
	SetFlag(fH, ((HL & 0xFF) + (HL & 0xFF)) > 0xFF);
	SetFlag(fC, ((int)HL + (int)HL) > 0xFFFF);

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
	SetFlag(fN, 0);
	SetFlag(fH, ((HL & 0xFF) + (SP & 0xFF)) > 0xFF);
	SetFlag(fC, ((int)HL + (int)SP) > 0xFFFF);

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
	static u8 data;
	data = Read(PC++);

	SetFlag(fN, 0);
	SetFlag(fH, ((SP & 0xFF) + (data & 0xFF)) > 0xFF);
	SetFlag(fC, ((int)SP + (int)data) > 0xFFFF);

	SP += data;
	return 0;
}
#pragma endregion
#pragma region 8bit bit instructions
u8 CPU::RLCA()
{
	static bool bit7;
	bit7 = A >> 7;

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	A = (A << 1) | (u8)bit7;
	return 0;
}

u8 CPU::RRCA()
{
	static u8 bit0;
	bit0 = A & 0b00000001;

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	A = (A >> 1) | (bit0 << 7);
	return 0;
}

u8 CPU::RLA()
{
	static bool bit7;
	bit7 = A >> 7;

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit7);

	A = (A << 1) | (u8)GetFlag(fC);
	return 0;
}

u8 CPU::RRA()
{
	static bool bit0;
	bit0 = A & 0b00000001;

	SetFlag(fZ, 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	A = (A >> 1) | (u8)GetFlag(fC);
	return 0;
}

u8 CPU::RLC_B()
{
	static bool bit7;
	bit7 = B >> 7;

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
	bit7 = C >> 7;

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
	bit7 = D >> 7;

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
	bit7 = E >> 7;

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
	bit7 = H >> 7;

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
	bit7 = L >> 7;

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
	bit7 = data >> 7;

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
	bit7 = A >> 7;

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

	SetFlag(fZ, L == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	L = (L >> 1) | (bit0 << 7);
	return 0;
}

u8 CPU::RL_B()
{
	static bool bit7;
	bit7 = B >> 7;

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
	bit7 = C >> 7;

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
	bit7 = D >> 7;

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
	bit7 = E >> 7;

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
	bit7 = H >> 7;

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
	bit7 = L >> 7;

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
	bit7 = data >> 7;

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
	bit7 = A >> 7;

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

	C = (C >> 1) | (u8)GetFlag(fC);

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

	D = (D >> 1) | (u8)GetFlag(fC);

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

	E = (E >> 1) | (u8)GetFlag(fC);

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

	H = (H >> 1) | (u8)GetFlag(fC);

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

	L = (L >> 1) | (u8)GetFlag(fC);

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

	Write(HL, (data >> 1) | (u8)GetFlag(fC));

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

	A = (A >> 1) | (u8)GetFlag(fC);

	SetFlag(fZ, A == 0);
	SetFlag(fN, 0);
	SetFlag(fH, 0);
	SetFlag(fC, bit0);

	return 0;
}

u8 CPU::SLA_B()
{
	static bool bit7;
	bit7 = B >> 7;

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
	bit7 = C >> 7;

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
	bit7 = D >> 7;

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
	bit7 = E >> 7;

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
	bit7 = H >> 7;

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
	bit7 = L >> 7;

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
	bit7 = data >> 7;

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
	bit7 = A >> 7;

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