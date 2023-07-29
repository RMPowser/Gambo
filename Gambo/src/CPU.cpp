#include "CPU.h"
#include "GamboCore.h"
#include "PPU.h"
#include "RAM.h"
#include "spdlog/spdlog.h"

const std::array<u8, 256> OpcodeTiming8Bit = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 12, 12, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0, 0,
	4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0
};

const std::array<u8, 256> OpcodeTiming16Bit = {
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4, 0,
	0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4, 0,
	0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4, 0,
	0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 4, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0,
	0, 0, 0, 0, 0, 0, 12, 0, 0, 0, 0, 0, 0, 0, 12, 0
};

CPU::CPU(GamboCore* c)
	: core(c)
{
	spdlog::enable_backtrace(50);
	Reset();
}

CPU::~CPU()
{
}

u8 CPU::Read(u16 addr)
{
	if ((core->ppu->IsEnabled()) && 
		(
			(core->ppu->GetMode() == PPUMode::OAMScan && (0xFE00 <= addr && addr <= 0xFE9F)) ||										// accessing oam during oam scan
			(core->ppu->GetMode() == PPUMode::Draw && ((0xFE00 <= addr && addr <= 0xFE9F) || (0x8000 <= addr && addr <= 0x9FFF)))	// accessing oam or vram during drawing
		)) 
	{
		return 0xFF;
	}
	else
	{
		return core->Read(addr);
	}
}

void CPU::Write(u16 addr, u8 data)
{
	if ((core->ppu->IsEnabled()) && 
		(
			(core->ppu->GetMode() == PPUMode::OAMScan && (0xFE00 <= addr && addr <= 0xFE9F)) ||										// accessing oam during oam scan
			(core->ppu->GetMode() == PPUMode::Draw && ((0xFE00 <= addr && addr <= 0xFE9F) || (0x8000 <= addr && addr <= 0x9FFF)))	// accessing oam or vram during drawing
		)) 
	{
		return;
	}

    core->Write(addr, data);
	if (addr == HWAddr::DMA)
	{
		core->ppu->SetDoDMATransfer(true);
	}
}

u8& CPU::Get(u16 addr)
{
	return core->ram->Get(addr);
}

u8 CPU::RunFor(u8 ticks)
{
	int cycles = 0;

	while (cycles < ticks)
	{
		if (isHalted)
		{
			// count cycles as if NOP during halt
			cycles += 4;

			if (isHalted && InterruptPending())
			{
				isHalted = false;
			}
		}

		bool handledInterrupt = false;

		if (!isHalted)
		{
			if (IME && InterruptPending() && opcodeTimingDelay < 0)
			{
				handledInterrupt = HandleInterrupt(GetPendingInterrupt());

				// it takes 5 m-cycles just to dispatch the interrupt
				cycles += 20;
			}
			else
			{
				if (opcodeTimingDelay < 0)
				{
					opcodeTimingDelay = 0;
					currentCycles = 0;
					opcode = Read(PC++);
					isCB = opcode == 0xCB;
					
#if defined(_DEBUG) && 0
					std::string s = "$" + hex(PC - 1, 4) + ": ";
					if (opcode == 0xCB)
					{
						u8 cbOpcode = Read(PC);
						s += instructions16bit[cbOpcode].mnemonic;
					}
					else
					{
						auto& instruction = instructions8bit[opcode];
						switch (instruction.bytes)
						{
							case 0:
							case 1:
							{
								s += instruction.mnemonic;
								break;
							}
							case 2:
							{
								u8 data = Read(PC);
								std::string firstTwoChar(instruction.mnemonic.begin(), instruction.mnemonic.begin() + 2);
								if (firstTwoChar == "JR")
								{
									s16 sdata = (s8)data;
									sdata += PC + 1;
									s += std::vformat(instruction.mnemonic, std::make_format_args(hex(sdata, 4)));
									break;
								}
								s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 2)));
								break;
							}
							case 3:
							{
								u16 lo = Read(PC);
								u16 hi = Read(PC + 1);
								u16 data = (hi << 8) | lo;
								s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 4)));
								break;
							}
							default:
								throw("opcode has more than 3 bytes");
						}
					}
					spdlog::debug(s);
#endif

					if (haltBug)
					{
						haltBug = false;
						PC--;
					}

					// timing for non cb instructions
					if (opcode == 0x36
						|| opcode == 0xE0
						|| opcode == 0xF0)
					{
						opcodeTimingDelay = 1;
					}

					if (opcode == 0xEA
						|| opcode == 0xFA)
					{
						opcodeTimingDelay = 2;
					}
				}
				

				
				if (isCB)
				{
					if (opcode == 0xCB)
					{
						opcode = Read(PC++);

						// halt bug applies to both bytes
						if (haltBug)
						{
							haltBug = false;
							PC--;
						}

						// timing for CB instructinos
						if (opcode == 0x46
							|| opcode == 0x4E
							|| opcode == 0x56
							|| opcode == 0x5E
							|| opcode == 0x66
							|| opcode == 0x6E
							|| opcode == 0x76
							|| opcode == 0x7E)
						{
							opcodeTimingDelay = 1;
						}
					}

					opcodeTable = &instructions16bit;
				}
				else
				{
					opcodeTable = &instructions8bit;
				}

				switch (opcodeTimingDelay)
				{
					// using the default case allows us to delay by any arbitrary amount
					default:
					{
						// throw if the delay is negative somehow. this should never happen
						if (opcodeTimingDelay < 0)
							throw;

						// add one m-cycle and set delay to the next lowest state
						cycles += 4;
						currentCycles += 4;
						opcodeTimingDelay--;
						break;
					}
					case 0:
					{
						// figure out the cycles remaining. for any opcode that is NOT delayed, 
						// this should be equal to whatever is in the opcode table
						currentCycles = (*opcodeTable)[opcode].cycles - currentCycles;

						// execute the instruction and see if we require additional clock cycles
						currentCycles += (this->*(*opcodeTable)[opcode].Execute)();

						cycles += currentCycles;
						opcodeTimingDelay--;
						break;
					}
				}
			}
		}

		if (!handledInterrupt && IMEcycles > 0)
		{
			if ((IMEcycles -= currentCycles) <= 0)
			{
				IMEcycles = 0;
				IME = true;
			}
		}
	}

	UpdateTimers(cycles);
	return cycles;
}

#pragma warning(push)
#pragma warning(disable: 26813)
void CPU::RequestInterrupt(InterruptFlags f)
{
	Write(HWAddr::IF, Read(HWAddr::IF) | f);
}
#pragma warning(pop)

bool CPU::HandleInterrupt(InterruptFlags f)
{	

	// disable interrupts
	IME = false;

	// acknowledge the interrupt by clearing its flag
	Get(HWAddr::IF) &= ~(f);

	// push the program counter so we can get back to 
	// where we left off
	Push(PC);

	// jump to the corresponding address
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

	return true;
}

void CPU::UpdateTimers(u8 ticks)
{
	// DIV register always counts up every 256 clock cycles
	if ((DIVCounter += ticks) >= 256)
	{
		DIVCounter -= 256;
		Get(HWAddr::DIV)++; // set directly to prevent reset from Write() logic
	}

	// if TIMA is enabled
	u8& TAC = Get(HWAddr::TAC);
	if (TAC & 0b100)
	{
		int TIMAFreq = 0;
		// switch on frequency index
		switch (TAC & 0b011)
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

		u8& TIMA = Get(HWAddr::TIMA);
		TIMACounter += ticks;
		while (TIMACounter >= TIMAFreq)
		{
			TIMACounter -= TIMAFreq;
			if (TIMA == 0xFF)
			{
				// TIMA resets to TMA register value
				TIMA = Read(HWAddr::TMA);
				RequestInterrupt(InterruptFlags::Timer);
			}
			else
				TIMA++;
		}
	}
}

bool CPU::InterruptPending()
{
	u8 IE = Read(HWAddr::IE);
	u8 IF = Read(HWAddr::IF);
	return (IE & IF & 0b11111) != 0;
}

InterruptFlags CPU::GetPendingInterrupt()
{
	u8 IE = Read(HWAddr::IE);
	u8 IF = Read(HWAddr::IF);
	u8 IE_IF = IE & IF;

	if (IE_IF & InterruptFlags::VBlank)
		return InterruptFlags::VBlank;

	if (IE_IF & InterruptFlags::LCDStat)
		return InterruptFlags::LCDStat;

	if (IE_IF & InterruptFlags::Timer)
		return InterruptFlags::Timer;

	if (IE_IF & InterruptFlags::Serial)
		return InterruptFlags::Serial;

	if (IE_IF & InterruptFlags::Joypad)
		return InterruptFlags::Joypad;

	return InterruptFlags::None;
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

void CPU::Reset()
{
	stopMode = false;
	isHalted = false;
	haltBug = false;
	unhaltCycles = 0;
	currentCycles = 0;
	opcodeTimingDelay = 0;
	opcode = 0;
	isCB = false;
	instructionComplete = true;
	opcodeTable = &instructions8bit;
	IME = false;
	IMEcycles = false;
	DIVCounter = 0;
	TIMACounter = 0;

	if (core->IsUseBootRom())
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
	}
	else
	{
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
	}
}

const u8& CPU::GetA() const
{
	return A;
}

const u8& CPU::GetF() const
{
	return F;
}

const u8& CPU::GetB() const
{
	return B;
}

const u8& CPU::GetC() const
{
	return C;
}

const u8& CPU::GetD() const
{
	return D;
}

const u8& CPU::GetE() const
{
	return E;
}

const u8& CPU::GetH() const
{
	return H;
}

const u8& CPU::GetL() const
{
	return L;
}

const u16& CPU::GetAF() const
{
	return AF;
}

const u16& CPU::GetBC() const
{
	return BC;
}

const u16& CPU::GetDE() const
{
	return DE;
}

const u16& CPU::GetHL() const
{
	return HL;
}

const u16& CPU::GetSP() const
{
	return SP;
}

const u16& CPU::GetPC() const
{
	return PC;
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

bool CPU::GetIME()
{
	return IME;
}

std::map<u16, std::string> CPU::Disassemble(u16 startAddr, int numInstr)
{
	u32 addr = startAddr;
	u8 value = 0x00, lo = 0x00, hi = 0x00;
	u16 lineAddr = 0;

	std::map<u16, std::string> mapAsm;

	while (mapAsm.size() < numInstr)
	{
		lineAddr = addr;

		// prefix line with instruction addr
		std::string s = "$" + hex(addr, 4) + ": ";

		// read instruction and get readable name
		u8 opcode = Read(addr++);
		if (opcode == 0xCB)
		{
			// its a 16bit opcode so read another byte
			opcode = Read(addr++);

			s += instructions16bit[opcode].mnemonic;
		}
		else
		{
			auto& instruction = instructions8bit[opcode];
			switch (instruction.bytes)
			{
				case 0:
				case 1:
				{
					s += instructions8bit[opcode].mnemonic;
					break;
				}
				case 2:
				{
					u8 data = Read(addr++);
					std::string firstTwoChar(instruction.mnemonic.begin(), instruction.mnemonic.begin() + 2);
					if (firstTwoChar == "JR")
					{
						s16 sdata = (s8)data;
						sdata += addr;
						s += std::vformat(instruction.mnemonic, std::make_format_args(hex(sdata, 4)));
						break;
					}
					s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 2)));
					break;
				}
				case 3:
				{
					u16 lo = Read(addr++);
					u16 hi = Read(addr++);
					u16 data = (hi << 8) | lo;
					s += std::vformat(instruction.mnemonic, std::make_format_args(hex(data, 4)));
					break;
				}
				default:
					throw("opcode has more than 3 bytes");
			}
		}

		mapAsm[lineAddr] = s;
	}

	return mapAsm;
}

void CPU::Push(const std::same_as<u16> auto data)
{
	u8 high = data >> 8;
	u8 low = data & 0xFF;

	Write(--SP, high);
	Write(--SP, low);
}

void CPU::Pop(std::same_as<u16> auto& reg)
{
	u16 low = Read(SP++);
	u16 high = Read(SP++);

	reg = (high << 8) | low;
}

#pragma region CPU Control Instructions
// catch all non existing instructions
u8 CPU::XXX()
{
	spdlog::dump_backtrace();
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
	if (IMEcycles > 0)
	{
		// pending interrupts are triggered before the halt
		IMEcycles = 0;
		IME = true;
		PC--; // go back one instruction so the halt executes again after the interrupt
	}
	else
	{
		isHalted = true;
		
		// the halt instruction will fail to increment the program counter in this case
		if (!IME && (Read(HWAddr::IF) & Read(HWAddr::IE) & 0x0b11111))
		{
			haltBug = true;
		}
	}

	return 0;
}

u8 CPU::DI()
{
	IME = false;
	IMEcycles = 0;

	return 0;
}

u8 CPU::EI()
{
	IMEcycles = 8;

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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);

	PC = (high << 8) | low;
	return 0;
}

u8 CPU::CALL_NZ_a16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;
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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;

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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;

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
	RET();
	IME = true;

	return 0;
}

u8 CPU::JP_C_a16()
{
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;

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
	u16 low = Read(PC++);
	u16 high = Read(PC++);
	u16 addr = (high << 8) | low;

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
	u8 data = Read(PC++);
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
	s8 data;

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
	int prev = B;
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
	int prev = C;
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
	int prev = D;
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
	int prev = E;
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
	int prev = H;
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
	int prev = L;
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
	int prev = Read(HL);
	Write(HL, prev + 1);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, (Read(HL) & 0xF) < (prev & 0xF));

	return 0;
}

u8 CPU::DEC_aHL()
{
	int data = Read(HL);
	Write(HL, --data);

	SetFlag(CPUFlags::Z, data == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (data & 0xF) == 0xF);

	return 0;
}

u8 CPU::INC_A()
{
	int prev = A;
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
	u8 data = Read(HL);

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
	u16 result = A - B;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (B & 0xF));
	SetFlag(CPUFlags::C, A < B);

	return 0;
}

u8 CPU::CP_A_C()
{
	u16 result = A - C;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (C & 0xF));
	SetFlag(CPUFlags::C, A < C);

	return 0;
}

u8 CPU::CP_A_D()
{
	u16 result = A - D;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (D & 0xF));
	SetFlag(CPUFlags::C, A < D);

	return 0;
}

u8 CPU::CP_A_E()
{
	u16 result = A - E;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (E & 0xF));
	SetFlag(CPUFlags::C, A < E);

	return 0;
}

u8 CPU::CP_A_H()
{
	u16 result = A - H;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (H & 0xF));
	SetFlag(CPUFlags::C, A < H);

	return 0;
}

u8 CPU::CP_A_L()
{
	u16 result = A - L;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (L & 0xF));
	SetFlag(CPUFlags::C, A < L);

	return 0;
}

u8 CPU::CP_A_aHL()
{
	u8 data = Read(HL);
	u16 result = A - data;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (data & 0xF));
	SetFlag(CPUFlags::C, A < data);

	return 0;
}

u8 CPU::CP_A_A()
{
	s16 result = A - A;

	SetFlag(CPUFlags::Z, result == 0);
	SetFlag(CPUFlags::N, 1);
	SetFlag(CPUFlags::H, (A & 0xF) < (A & 0xF));
	SetFlag(CPUFlags::C, A < A);

	return 0;
}

u8 CPU::ADD_A_d8()
{
	u8 data = Read(PC++);

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
	u8 data = Read(PC++);

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
	u8 data = Read(PC++);
	u16 result = A - data;

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
	bool bit7 = A & 0b10000000;

	A = (A << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RRCA()
{
	u8 bit0 = A & 0b00000001;

	A = (A >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RLA()
{
	bool bit7 = A & 0b10000000;

	A = (A << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RRA()
{
	bool bit0 = A & 0b00000001;

	A = (A >> 1) | ((u8)GetFlag(CPUFlags::C) << 7);

	SetFlag(CPUFlags::Z, 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RLC_B()
{
	bool bit7 = B & 0b10000000;

	B = (B << 1) | (u8)bit7;
	
	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);
	
	return 0;
}

u8 CPU::RLC_C()
{
	bool bit7 = C & 0b10000000;

	C = (C << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_D()
{
	bool bit7 = D & 0b10000000;

	D = (D << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_E()
{
	bool bit7 = E & 0b10000000;

	E = (E << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_H()
{
	bool bit7 = H & 0b10000000;

	H = (H << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_L()
{
	bool bit7 = L & 0b10000000;

	L = (L << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_aHL()
{
	u8 data = Read(HL);
	bool bit7 = data & 0b10000000;

	Write(HL, (data << 1) | (u8)bit7);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RLC_A()
{
	bool bit7 = A & 0b10000000;

	A = (A << 1) | (u8)bit7;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RRC_B()
{
	u8 bit0 = B & 0b00000001;

	B = (B >> 1) | (bit0 << 7);
	
	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_C()
{
	u8 bit0 = C & 0b00000001;

	C = (C >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_D()
{
	u8 bit0 = D & 0b00000001;

	D = (D >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_E()
{
	u8 bit0 = E & 0b00000001;

	E = (E >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_H()
{
	u8 bit0 = H & 0b00000001;

	H = (H >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_L()
{
	u8 bit0 = L & 0b00000001;

	L = (L >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_aHL()
{
	u8 data = Read(HL);
	u8 bit0 = data & 0b00000001;

	Write(HL, (data >> 1) | (bit0 << 7));

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RRC_A()
{
	u8 bit0  = A & 0b00000001;

	A = (A >> 1) | (bit0 << 7);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RL_B()
{
	bool bit7 = B & 0b10000000;

	B = (B << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_C()
{
	bool bit7 = C & 0b10000000;

	C = (C << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_D()
{
	bool bit7 = D & 0b10000000;

	D = (D << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_E()
{
	bool bit7 = E & 0b10000000;

	E = (E << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_H()
{
	bool bit7 = H & 0b10000000;

	H = (H << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_L()
{
	bool bit7 = L & 0b10000000;

	L = (L << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_aHL()
{
	u8 data = Read(HL);
	bool bit7 = data & 0b10000000;

	Write(HL, (data << 1) | (u8)GetFlag(CPUFlags::C));

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RL_A()
{
	bool bit7 = A & 0b10000000;

	A = (A << 1) | (u8)GetFlag(CPUFlags::C);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::RR_B()
{
	bool bit0 = B & 0b00000001;

	B = (B >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_C()
{
	bool bit0 = C & 0b00000001;

	C = (C >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_D()
{
	bool bit0 = D & 0b00000001;

	D = (D >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_E()
{
	bool bit0 = E & 0b00000001;

	E = (E >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_H()
{
	bool bit0 = H & 0b00000001;

	H = (H >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_L()
{
	bool bit0 = L & 0b00000001;

	L = (L >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_aHL()
{
	u8 data = Read(HL);
	bool bit0 = data & 0b00000001;

	Write(HL, (data >> 1) | (u8)GetFlag(CPUFlags::C) << 7);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::RR_A()
{
	bool bit0 = A & 0b00000001;

	A = (A >> 1) | (u8)GetFlag(CPUFlags::C) << 7;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SLA_B()
{
	bool bit7 = B & 0b10000000;

	B <<= 1;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_C()
{
	bool bit7 = C & 0b10000000;

	C <<= 1;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_D()
{
	bool bit7 = D & 0b10000000;

	D <<= 1;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_E()
{
	bool bit7 = E & 0b10000000;

	E <<= 1;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_H()
{
	bool bit7 = H & 0b10000000;

	H <<= 1;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_L()
{
	bool bit7 = L & 0b10000000;

	L <<= 1;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_aHL()
{
	u8 data = Read(HL);
	bool bit7 = data & 0b10000000;

	Write(HL, data << 1);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SLA_A()
{
	bool bit7 = A & 0b10000000;

	A <<= 1;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit7);

	return 0;
}

u8 CPU::SRA_B()
{
	bool bit0 = B & 0b00000001;
	bool bit7 = B & 0b10000000;

	B = (B >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_C()
{
	bool bit0 = C & 0b00000001;
	bool bit7 = C & 0b10000000;

	C = (C >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_D()
{
	bool bit0 = D & 0b00000001;
	bool bit7 = D & 0b10000000;

	D = (D >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_E()
{
	bool bit0 = E & 0b00000001;
	bool bit7 = E & 0b10000000;

	E = (E >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_H()
{
	bool bit0 = H & 0b00000001;
	bool bit7 = H & 0b10000000;

	H = (H >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_L()
{
	bool bit0 = L & 0b00000001;
	bool bit7 = L & 0b10000000;

	L = (L >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_aHL()
{
	u8 data = Read(HL);
	bool bit0 = data & 0b00000001;
	bool bit7 = (data & 0b10000000) >> 7;

	Write(HL, (data >> 1) | (bit7 << 7));

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRA_A()
{
	bool bit0 = A & 0b00000001;
	bool bit7 = A & 0b10000000;

	A = (A >> 1) | (bit7 << 7);

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SWAP_B()
{
	u8 low = B & 0b00001111;
	u8 high = B >> 4;

	B = (low << 4) | high;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_C()
{
	u8 low = C & 0b00001111;
	u8 high = C >> 4;

	C = (low << 4) | high;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_D()
{
	u8 low = D & 0b00001111;
	u8 high = D >> 4;

	D = (low << 4) | high;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_E()
{
	u8 low = E & 0b00001111;
	u8 high = E >> 4;

	E = (low << 4) | high;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_H()
{
	u8 low = H & 0b00001111;
	u8 high = H >> 4;

	H = (low << 4) | high;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_L()
{
	u8 low = L & 0b00001111;
	u8 high = L >> 4;

	L = (low << 4) | high;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_aHL()
{
	u8 data = Read(HL);

	u8 low = data & 0b00001111;
	u8 high = data >> 4;

	Write(HL, (low << 4) | high);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SWAP_A()
{
	u8 low = A & 0b00001111;
	u8 high = A >> 4;

	A = (low << 4) | high;

	SetFlag(CPUFlags::Z, A == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, 0);

	return 0;
}

u8 CPU::SRL_B()
{
	bool bit0 = B & 0b00000001;

	B >>= 1;

	SetFlag(CPUFlags::Z, B == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_C()
{
	bool bit0 = C & 0b00000001;

	C >>= 1;

	SetFlag(CPUFlags::Z, C == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_D()
{
	bool bit0 = D & 0b00000001;

	D >>= 1;

	SetFlag(CPUFlags::Z, D == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_E()
{
	bool bit0 = E & 0b00000001;

	E >>= 1;

	SetFlag(CPUFlags::Z, E == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_H()
{
	bool bit0 = H & 0b00000001;

	H >>= 1;

	SetFlag(CPUFlags::Z, H == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_L()
{
	bool bit0 = L & 0b00000001;

	L >>= 1;

	SetFlag(CPUFlags::Z, L == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_aHL()
{
	u8 data = Read(HL);
	bool bit0 = data & 0b00000001;

	Write(HL, data >> 1);

	SetFlag(CPUFlags::Z, Read(HL) == 0);
	SetFlag(CPUFlags::N, 0);
	SetFlag(CPUFlags::H, 0);
	SetFlag(CPUFlags::C, bit0);

	return 0;
}

u8 CPU::SRL_A()
{
	bool bit0 = A & 0b00000001;

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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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
	u8 data = Read(HL);
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