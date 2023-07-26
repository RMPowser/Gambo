#pragma once
#include "GamboDefine.h"

class GamboCore;

enum class CPUFlags : u8
{
	// 0 thru 3 are unused
	C = (1 << 4), // carry
	H = (1 << 5), // half carry
	N = (1 << 6), // subtract
	Z = (1 << 7), // zero
};


class CPU
{
	struct CPUInstruction
	{
		std::string mnemonic;
		u8 bytes = 0;
		u8 cycles = 0;
		u8(CPU::* Execute)(void);
	};

	CPU() = delete;
	bool operator==(const CPU& other) const = delete;

public:	
	CPU(GamboCore* c);
	~CPU();
	
	u8 RunFor(u8 ticks);
	void Reset();

	const u8& GetA() const;
	const u8& GetF() const;
	const u8& GetB() const;
	const u8& GetC() const;
	const u8& GetD() const;
	const u8& GetE() const;
	const u8& GetH() const;
	const u8& GetL() const;
	const u16& GetAF() const;
	const u16& GetBC() const;
	const u16& GetDE() const;
	const u16& GetHL() const;
	const u16& GetSP() const;
	const u16& GetPC() const;
	bool GetFlag(CPUFlags f);
	bool GetIME();

	void RequestInterrupt(InterruptFlags f);

	std::map<u16, std::string> Disassemble(u16 startAddr, int numInstr);

private:
	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	u8& Get(u16 addr);

	void SetFlag(CPUFlags f, bool v);

	void Push(const std::same_as<u16> auto data);
	void Pop(std::same_as<u16> auto& data);

	void UpdateTimers(u8 ticks);
	bool InterruptPending();
	InterruptFlags GetPendingInterrupt();
	bool HandleInterrupt(InterruptFlags f);

	union {	struct { u8 F; u8 A; }; u16 AF; };
	union { struct { u8 C; u8 B; }; u16 BC; };
	union { struct { u8 E; u8 D; }; u16 DE; };
	union { struct { u8 L; u8 H; }; u16 HL; };

	u16 SP; // stack pointer
	u16 PC; // program counter

	GamboCore* core;
	bool stopMode;					// set to true by the stop command, set back to false by reset command
	bool isHalted;					// set to true by the halt command, set back to false by reset or interrupt
	bool haltBug;
	int unhaltCycles;				
	int currentCycles;
	int vblankInterruptCycles;
	int opcodeTimingDelay;
	int opcode;
	bool isCB;
	bool instructionComplete;
	const std::vector<CPUInstruction>* opcodeTable;
	bool IME;						// interrupt master enable flag
	int IMEcycles;					// used to delay the enabling of IME by one instruction
	int DIVCounter;
	int TIMACounter;

	// instruction helpers
	void ADC(const u8 data);
	void SBC(const u8 data);
	void BIT(u8& reg, int bit);

#pragma region Instructions
	const std::vector<CPUInstruction> instructions8bit =
	{
		{"NOP"       ,1,4 ,&CPU::NOP     }, {"LD BC, {}" ,3,12,&CPU::LD_BC_d16 }, {"LD (BC), A"  ,1,8 ,&CPU::LD_aBC_A   }, {"INC BC"    ,1,8 ,&CPU::INC_BC  }, {"INC B"       ,1,4 ,&CPU::INC_B      }, {"DEC B"     ,1,4 ,&CPU::DEC_B   }, {"LD B, {}"   ,2,8 ,&CPU::LD_B_d8  }, {"RLCA"      ,1,4 ,&CPU::RLCA    }, {"LD ({}), SP" ,3,20,&CPU::LD_a16_SP     }, {"ADD HL, BC",1,8 ,&CPU::ADD_HL_BC}, {"LD A, (BC)"  ,1,8 ,&CPU::LD_A_aBC   }, {"DEC BC"  ,1,8 ,&CPU::DEC_BC }, {"INC C"      ,1,4 ,&CPU::INC_C     }, {"DEC C"   ,1,4 ,&CPU::DEC_C   }, {"LD C, {}"   ,2,8 ,&CPU::LD_C_d8  }, {"RRCA"    ,1,4 ,&CPU::RRCA   },
		{"STOP"      ,1,4 ,&CPU::STOP    }, {"LD DE, {}" ,3,12,&CPU::LD_DE_d16 }, {"LD (DE), A"  ,1,8 ,&CPU::LD_aDE_A   }, {"INC DE"    ,1,8 ,&CPU::INC_DE  }, {"INC D"       ,1,4 ,&CPU::INC_D      }, {"DEC D"     ,1,4 ,&CPU::DEC_D   }, {"LD D, {}"   ,2,8 ,&CPU::LD_D_d8  }, {"RLA"       ,1,4 ,&CPU::RLA     }, {"JR {}"       ,2,12,&CPU::JR_s8         }, {"ADD HL, DE",1,8 ,&CPU::ADD_HL_DE}, {"LD A, (DE)"  ,1,8 ,&CPU::LD_A_aDE   }, {"DEC DE"  ,1,8 ,&CPU::DEC_DE }, {"INC E"      ,1,4 ,&CPU::INC_E     }, {"DEC E"   ,1,4 ,&CPU::DEC_E   }, {"LD E, {}"   ,2,8 ,&CPU::LD_E_d8  }, {"RRA"     ,1,4 ,&CPU::RRA    },
		{"JR NZ, {}" ,2,8 ,&CPU::JR_NZ_s8}, {"LD HL, {}" ,3,12,&CPU::LD_HL_d16 }, {"LD (HL)++, A",1,8 ,&CPU::LD_aHLinc_A}, {"INC HL"    ,1,8 ,&CPU::INC_HL  }, {"INC H"       ,1,4 ,&CPU::INC_H      }, {"DEC H"     ,1,4 ,&CPU::DEC_H   }, {"LD H, {}"   ,2,8 ,&CPU::LD_H_d8  }, {"DAA"       ,1,4 ,&CPU::DAA     }, {"JR Z, {}"    ,2,8 ,&CPU::JR_Z_s8       }, {"ADD HL, HL",1,8 ,&CPU::ADD_HL_HL}, {"LD A, (HL)++",1,8 ,&CPU::LD_A_aHLinc}, {"DEC HL"  ,1,8 ,&CPU::DEC_HL }, {"INC L"      ,1,4 ,&CPU::INC_L     }, {"DEC L"   ,1,4 ,&CPU::DEC_L   }, {"LD L, {}"   ,2,8 ,&CPU::LD_L_d8  }, {"CPL"     ,1,4 ,&CPU::CPL    },
		{"JR NC, {}" ,2,8 ,&CPU::JR_NC_s8}, {"LD SP, {}" ,3,12,&CPU::LD_SP_d16 }, {"LD (HL)--, A",1,8 ,&CPU::LD_aHLdec_A}, {"INC SP"    ,1,8 ,&CPU::INC_SP  }, {"INC (HL)"    ,1,12,&CPU::INC_aHL    }, {"DEC (HL)"  ,1,12,&CPU::DEC_aHL }, {"LD (HL), {}",2,12,&CPU::LD_aHL_d8}, {"SCF"       ,1,4 ,&CPU::SCF     }, {"JR C, {}"    ,2,8 ,&CPU::JR_C_s8       }, {"ADD HL, SP",1,8 ,&CPU::ADD_HL_SP}, {"LD A, (HL)--",1,8 ,&CPU::LD_A_aHLdec}, {"DEC SP"  ,1,8 ,&CPU::DEC_SP }, {"INC A"      ,1,4 ,&CPU::INC_A     }, {"DEC A"   ,1,4 ,&CPU::DEC_A   }, {"LD A, {}"   ,2,8 ,&CPU::LD_A_d8  }, {"CCF"     ,1,4 ,&CPU::CCF    },
		{"LD B, B"   ,1,4 ,&CPU::LD_B_B  }, {"LD B, C"   ,1,4 ,&CPU::LD_B_C    }, {"LD B, D"     ,1,4 ,&CPU::LD_B_D     }, {"LD B, E"   ,1,4 ,&CPU::LD_B_E  }, {"LD B, H"     ,1,4 ,&CPU::LD_B_H     }, {"LD B, L"   ,1,4 ,&CPU::LD_B_L  }, {"LD B, (HL)" ,1,8 ,&CPU::LD_B_aHL }, {"LD B, A"   ,1,4 ,&CPU::LD_B_A  }, {"LD C, B"     ,1,4 ,&CPU::LD_C_B        }, {"LD C, C"   ,1,4 ,&CPU::LD_C_C   }, {"LD C, D"     ,1,4 ,&CPU::LD_C_D     }, {"LD C, E" ,1,4 ,&CPU::LD_C_E }, {"LD C, H"    ,1,4 ,&CPU::LD_C_H    }, {"LD C, L" ,1,4 ,&CPU::LD_C_L  }, {"LD C, (HL)" ,1,8 ,&CPU::LD_C_aHL }, {"LD C, A" ,1,4 ,&CPU::LD_C_A },
		{"LD D, B"   ,1,4 ,&CPU::LD_D_B  }, {"LD D, C"   ,1,4 ,&CPU::LD_D_C    }, {"LD D, D"     ,1,4 ,&CPU::LD_D_D     }, {"LD D, E"   ,1,4 ,&CPU::LD_D_E  }, {"LD D, H"     ,1,4 ,&CPU::LD_D_H     }, {"LD D, L"   ,1,4 ,&CPU::LD_D_L  }, {"LD D, (HL)" ,1,8 ,&CPU::LD_D_aHL }, {"LD D, A"   ,1,4 ,&CPU::LD_D_A  }, {"LD E, B"     ,1,4 ,&CPU::LD_E_B        }, {"LD E, C"   ,1,4 ,&CPU::LD_E_C   }, {"LD E, D"     ,1,4 ,&CPU::LD_E_D     }, {"LD E, E" ,1,4 ,&CPU::LD_E_E }, {"LD E, H"    ,1,4 ,&CPU::LD_E_H    }, {"LD E, L" ,1,4 ,&CPU::LD_E_L  }, {"LD E, (HL)" ,1,8 ,&CPU::LD_E_aHL }, {"LD E, A" ,1,4 ,&CPU::LD_E_A },
		{"LD H, B"   ,1,4 ,&CPU::LD_H_B  }, {"LD H, C"   ,1,4 ,&CPU::LD_H_C    }, {"LD H, D"     ,1,4 ,&CPU::LD_H_D     }, {"LD H, E"   ,1,4 ,&CPU::LD_H_E  }, {"LD H, H"     ,1,4 ,&CPU::LD_H_H     }, {"LD H, L"   ,1,4 ,&CPU::LD_H_L  }, {"LD H, (HL)" ,1,8 ,&CPU::LD_H_aHL }, {"LD H, A"   ,1,4 ,&CPU::LD_H_A  }, {"LD L, B"     ,1,4 ,&CPU::LD_L_B        }, {"LD L, C"   ,1,4 ,&CPU::LD_L_C   }, {"LD L, D"     ,1,4 ,&CPU::LD_L_D     }, {"LD L, E" ,1,4 ,&CPU::LD_L_E }, {"LD L, H"    ,1,4 ,&CPU::LD_L_H    }, {"LD L, L" ,1,4 ,&CPU::LD_L_L  }, {"LD L, (HL)" ,1,8 ,&CPU::LD_L_aHL }, {"LD L, A" ,1,4 ,&CPU::LD_L_A },
		{"LD (HL), B",1,8 ,&CPU::LD_aHL_B}, {"LD (HL), C",1,8 ,&CPU::LD_aHL_C  }, {"LD (HL), D"  ,1,8 ,&CPU::LD_aHL_D   }, {"LD (HL), E",1,8 ,&CPU::LD_aHL_E}, {"LD (HL), H"  ,1,8 ,&CPU::LD_aHL_H   }, {"LD (HL), L",1,8 ,&CPU::LD_aHL_L}, {"HALT"       ,1,4 ,&CPU::HALT     }, {"LD (HL), A",1,8 ,&CPU::LD_aHL_A}, {"LD A, B"     ,1,4 ,&CPU::LD_A_B        }, {"LD A, C"   ,1,4 ,&CPU::LD_A_C   }, {"LD A, D"     ,1,4 ,&CPU::LD_A_D     }, {"LD A, E" ,1,4 ,&CPU::LD_A_E }, {"LD A, H"    ,1,4 ,&CPU::LD_A_H    }, {"LD A, L" ,1,4 ,&CPU::LD_A_L  }, {"LD A, (HL)" ,1,8 ,&CPU::LD_A_aHL }, {"LD A, A" ,1,4 ,&CPU::LD_A_A },
		{"ADD A, B"  ,1,4 ,&CPU::ADD_A_B }, {"ADD A, C"  ,1,4 ,&CPU::ADD_A_C   }, {"ADD A, D"    ,1,4 ,&CPU::ADD_A_D    }, {"ADD A, E"  ,1,4 ,&CPU::ADD_A_E }, {"ADD A, H"    ,1,4 ,&CPU::ADD_A_H    }, {"ADD A, L"  ,1,4 ,&CPU::ADD_A_L }, {"ADD A, (HL)",1,8 ,&CPU::ADD_A_aHL}, {"ADD A, A"  ,1,4 ,&CPU::ADD_A_A }, {"ADC A, B"    ,1,4 ,&CPU::ADC_A_B       }, {"ADC A, C"  ,1,4 ,&CPU::ADC_A_C  }, {"ADC A, D"    ,1,4 ,&CPU::ADC_A_D    }, {"ADC A, E",1,4 ,&CPU::ADC_A_E}, {"ADC A, H"   ,1,4 ,&CPU::ADC_A_H   }, {"ADC A, L",1,4 ,&CPU::ADC_A_L }, {"ADC A, (HL)",1,8 ,&CPU::ADC_A_aHL}, {"ADC A, A",1,4 ,&CPU::ADC_A_A},
		{"SUB A, B"  ,1,4 ,&CPU::SUB_A_B }, {"SUB A, C"  ,1,4 ,&CPU::SUB_A_C   }, {"SUB A, D"    ,1,4 ,&CPU::SUB_A_D    }, {"SUB A, E"  ,1,4 ,&CPU::SUB_A_E }, {"SUB A, H"    ,1,4 ,&CPU::SUB_A_H    }, {"SUB A, L"  ,1,4 ,&CPU::SUB_A_L }, {"SUB A, (HL)",1,8 ,&CPU::SUB_A_aHL}, {"SUB A, A"  ,1,4 ,&CPU::SUB_A_A }, {"SBC A, B"    ,1,4 ,&CPU::SBC_A_B       }, {"SBC A, C"  ,1,4 ,&CPU::SBC_A_C  }, {"SBC A, D"    ,1,4 ,&CPU::SBC_A_D    }, {"SBC A, E",1,4 ,&CPU::SBC_A_E}, {"SBC A, H"   ,1,4 ,&CPU::SBC_A_H   }, {"SBC A, L",1,4 ,&CPU::SBC_A_L }, {"SBC A, (HL)",1,8 ,&CPU::SBC_A_aHL}, {"SBC A, A",1,4 ,&CPU::SBC_A_A},
		{"AND A, B"  ,1,4 ,&CPU::AND_A_B }, {"AND A, C"  ,1,4 ,&CPU::AND_A_C   }, {"AND A, D"    ,1,4 ,&CPU::AND_A_D    }, {"AND A, E"  ,1,4 ,&CPU::AND_A_E }, {"AND A, H"    ,1,4 ,&CPU::AND_A_H    }, {"AND A, L"  ,1,4 ,&CPU::AND_A_L }, {"AND A, (HL)",1,8 ,&CPU::AND_A_aHL}, {"AND A, A"  ,1,4 ,&CPU::AND_A_A }, {"XOR A, B"    ,1,4 ,&CPU::XOR_A_B       }, {"XOR A, C"  ,1,4 ,&CPU::XOR_A_C  }, {"XOR A, D"    ,1,4 ,&CPU::XOR_A_D    }, {"XOR A, E",1,4 ,&CPU::XOR_A_E}, {"XOR A, H"   ,1,4 ,&CPU::XOR_A_H   }, {"XOR A, L",1,4 ,&CPU::XOR_A_L }, {"XOR A, (HL)",1,8 ,&CPU::XOR_A_aHL}, {"XOR A, A",1,4 ,&CPU::XOR_A_A},
		{"OR A, B"   ,1,4 ,&CPU::OR_A_B  }, {"OR A, C"   ,1,4 ,&CPU::OR_A_C    }, {"OR A, D"     ,1,4 ,&CPU::OR_A_D     }, {"OR A, E"   ,1,4 ,&CPU::OR_A_E  }, {"OR A, H"     ,1,4 ,&CPU::OR_A_H     }, {"OR A, L"   ,1,4 ,&CPU::OR_A_L  }, {"OR A, (HL)" ,1,8 ,&CPU::OR_A_aHL }, {"OR A, A"   ,1,4 ,&CPU::OR_A_A  }, {"CP A, B"     ,1,4 ,&CPU::CP_A_B        }, {"CP A, C"   ,1,4 ,&CPU::CP_A_C   }, {"CP A, D"     ,1,4 ,&CPU::CP_A_D     }, {"CP A, E" ,1,4 ,&CPU::CP_A_E }, {"CP A, H"    ,1,4 ,&CPU::CP_A_H    }, {"CP A, L" ,1,4 ,&CPU::CP_A_L  }, {"CP A, (HL)" ,1,8 ,&CPU::CP_A_aHL }, {"CP A, A" ,1,4 ,&CPU::CP_A_A },
		{"RET NZ"    ,1,8 ,&CPU::RET_NZ  }, {"POP BC"    ,1,12,&CPU::POP_BC    }, {"JP NZ, {}"   ,3,12,&CPU::JP_NZ_a16  }, {"JP {}"     ,3,16,&CPU::JP_a16  }, {"CALL NZ, {}" ,3,12,&CPU::CALL_NZ_a16}, {"PUSH BC"   ,1,16,&CPU::PUSH_BC }, {"ADD A, {}"  ,2,8 ,&CPU::ADD_A_d8 }, {"RST 0"     ,1,16,&CPU::RST_0   }, {"RET Z"       ,1,8 ,&CPU::RET_Z         }, {"RET"       ,1,16,&CPU::RET      }, {"JP Z, {}"    ,3,12,&CPU::JP_Z_a16   }, {"???"     ,0,0 ,&CPU::XXX    }, {"CALL Z, {}" ,3,12,&CPU::CALL_Z_a16}, {"CALL {}" ,3,24,&CPU::CALL_a16}, {"ADC A, {}"  ,2,8 ,&CPU::ADC_A_d8 }, {"RST 1"   ,1,16,&CPU::RST_1  },
		{"RET NC"    ,1,8 ,&CPU::RET_NC  }, {"POP DE"    ,1,12,&CPU::POP_DE    }, {"JP NC, {}"   ,3,12,&CPU::JP_NC_a16  }, {"???"       ,0,0 ,&CPU::XXX     }, {"CALL NC, {}" ,3,12,&CPU::CALL_NC_a16}, {"PUSH DE"   ,1,16,&CPU::PUSH_DE }, {"SUB A, {}"  ,2,8 ,&CPU::SUB_A_d8 }, {"RST 2"     ,1,16,&CPU::RST_2   }, {"RET C"       ,1,8 ,&CPU::RET_C         }, {"RETI"      ,1,16,&CPU::RETI     }, {"JP C, {}"    ,3,12,&CPU::JP_C_a16   }, {"???"     ,0,0 ,&CPU::XXX    }, {"CALL C, {}" ,3,12,&CPU::CALL_C_a16}, {"???"     ,0,0 ,&CPU::XXX     }, {"SBC A, {}"  ,2,8 ,&CPU::SBC_A_d8 }, {"RST 3"   ,1,16,&CPU::RST_3  },
		{"LD ({}), A",2,12,&CPU::LD_aa8_A}, {"POP HL"    ,1,12,&CPU::POP_HL    }, {"LD (C), A"   ,1,8 ,&CPU::LD_aC_A    }, {"???"       ,0,0 ,&CPU::XXX     }, {"???"         ,0,0 ,&CPU::XXX        }, {"PUSH HL"   ,1,16,&CPU::PUSH_HL }, {"AND A, {}"  ,2,8 ,&CPU::AND_A_d8 }, {"RST 4"     ,1,16,&CPU::RST_4   }, {"ADD SP, {}"  ,2,16,&CPU::ADD_SP_s8     }, {"JP HL"     ,1,4 ,&CPU::JP_HL    }, {"LD ({}), A"  ,3,16,&CPU::LD_aa16_A  }, {"???"     ,0,0 ,&CPU::XXX    }, {"???"        ,0,0 ,&CPU::XXX       }, {"???"     ,0,0 ,&CPU::XXX     }, {"XOR A, {}"  ,2,8 ,&CPU::XOR_A_d8 }, {"RST 5"   ,1,16,&CPU::RST_5  },
		{"LD A, ({})",2,12,&CPU::LD_A_aa8}, {"POP AF"    ,1,12,&CPU::POP_AF    }, {"LD A, (C)"   ,1,8 ,&CPU::LD_A_aC    }, {"DI"        ,1,4 ,&CPU::DI      }, {"???"         ,0,0 ,&CPU::XXX        }, {"PUSH AF"   ,1,16,&CPU::PUSH_AF }, {"OR A, {}"   ,2,8 ,&CPU::OR_A_d8  }, {"RST 6"     ,1,16,&CPU::RST_6   }, {"LD HL, SP+{}",2,12,&CPU::LD_HL_SPinc_s8}, {"LD SP, HL" ,1,8 ,&CPU::LD_SP_HL }, {"LD A, ({})"  ,3,16,&CPU::LD_A_aa16  }, {"EI"      ,1,4 ,&CPU::EI     }, {"???"        ,0,0 ,&CPU::XXX       }, {"???"     ,0,0 ,&CPU::XXX     }, {"CP A, {}"   ,2,8 ,&CPU::CP_A_d8  }, {"RST 7"   ,1,16,&CPU::RST_7  },
	};
	const std::vector<CPUInstruction> instructions16bit =
	{
		{"RLC B"   ,2,8 ,&CPU::RLC_B  }, {"RLC C"   ,2,8 ,&CPU::RLC_C  }, {"RLC D"   ,2,8 ,&CPU::RLC_D  }, {"RLC E"   ,2,8 ,&CPU::RLC_E  }, {"RLC H"   ,2,8 ,&CPU::RLC_H  }, {"RLC L"   ,2,8 ,&CPU::RLC_L  }, {"RLC (HL)"   ,2,16,&CPU::RLC_aHL  }, {"RLC A"   ,2,8 ,&CPU::RLC_A  }, {"RRC B"   ,2,8 ,&CPU::RRC_B  }, {"RRC C"   ,2,8 ,&CPU::RRC_C  }, {"RRC D"   ,2,8 ,&CPU::RRC_D  }, {"RRC E"   ,2,8 ,&CPU::RRC_E  }, {"RRC H"   ,2,8 ,&CPU::RRC_H  }, {"RRC L"   ,2,8 ,&CPU::RRC_L  }, {"RRC (HL)"   ,2,16,&CPU::RRC_aHL  }, {"RRC A"   ,2,8 ,&CPU::RRC_A  },
		{"RL B"    ,2,8 ,&CPU::RL_B   }, {"RL C"    ,2,8 ,&CPU::RL_C   }, {"RL D"    ,2,8 ,&CPU::RL_D   }, {"RL E"    ,2,8 ,&CPU::RL_E   }, {"RL H"    ,2,8 ,&CPU::RL_H   }, {"RL L"    ,2,8 ,&CPU::RL_L   }, {"RL (HL)"    ,2,16,&CPU::RL_aHL   }, {"RL A"    ,2,8 ,&CPU::RL_A   }, {"RR B"    ,2,8 ,&CPU::RR_B   }, {"RR C"    ,2,8 ,&CPU::RR_C   }, {"RR D"    ,2,8 ,&CPU::RR_D   }, {"RR E"    ,2,8 ,&CPU::RR_E   }, {"RR H"    ,2,8 ,&CPU::RR_H   }, {"RR L"    ,2,8 ,&CPU::RR_L   }, {"RR (HL)"    ,2,16,&CPU::RR_aHL   }, {"RR A"    ,2,8 ,&CPU::RR_A   },
		{"SLA B"   ,2,8 ,&CPU::SLA_B  }, {"SLA C"   ,2,8 ,&CPU::SLA_C  }, {"SLA D"   ,2,8 ,&CPU::SLA_D  }, {"SLA E"   ,2,8 ,&CPU::SLA_E  }, {"SLA H"   ,2,8 ,&CPU::SLA_H  }, {"SLA L"   ,2,8 ,&CPU::SLA_L  }, {"SLA (HL)"   ,2,16,&CPU::SLA_aHL  }, {"SLA A"   ,2,8 ,&CPU::SLA_A  }, {"SRA B"   ,2,8 ,&CPU::SRA_B  }, {"SRA C"   ,2,8 ,&CPU::SRA_C  }, {"SRA D"   ,2,8 ,&CPU::SRA_D  }, {"SRA E"   ,2,8 ,&CPU::SRA_E  }, {"SRA H"   ,2,8 ,&CPU::SRA_H  }, {"SRA L"   ,2,8 ,&CPU::SRA_L  }, {"SRA (HL)"   ,2,16,&CPU::SRA_aHL  }, {"SRA A"   ,2,8 ,&CPU::SRA_A  },
		{"SWAP B"  ,2,8 ,&CPU::SWAP_B }, {"SWAP C"  ,2,8 ,&CPU::SWAP_C }, {"SWAP D"  ,2,8 ,&CPU::SWAP_D }, {"SWAP E"  ,2,8 ,&CPU::SWAP_E }, {"SWAP H"  ,2,8 ,&CPU::SWAP_H }, {"SWAP L"  ,2,8 ,&CPU::SWAP_L }, {"SWAP (HL)"  ,2,16,&CPU::SWAP_aHL }, {"SWAP A"  ,2,8 ,&CPU::SWAP_A }, {"SRL B"   ,2,8 ,&CPU::SRL_B  }, {"SRL C"   ,2,8 ,&CPU::SRL_C  }, {"SRL D"   ,2,8 ,&CPU::SRL_D  }, {"SRL E"   ,2,8 ,&CPU::SRL_E  }, {"SRL H"   ,2,8 ,&CPU::SRL_H  }, {"SRL L"   ,2,8 ,&CPU::SRL_L  }, {"SRL (HL)"   ,2,16,&CPU::SRL_aHL  }, {"SRL A"   ,2,8 ,&CPU::SRL_A  },
		{"BIT 0, B",2,8 ,&CPU::BIT_0_B}, {"BIT 0, C",2,8 ,&CPU::BIT_0_C}, {"BIT 0, D",2,8 ,&CPU::BIT_0_D}, {"BIT 0, E",2,8 ,&CPU::BIT_0_E}, {"BIT 0, H",2,8 ,&CPU::BIT_0_H}, {"BIT 0, L",2,8 ,&CPU::BIT_0_L}, {"BIT 0, (HL)",2,12,&CPU::BIT_0_aHL}, {"BIT 0, A",2,8 ,&CPU::BIT_0_A}, {"BIT 1, B",2,8 ,&CPU::BIT_1_B}, {"BIT 1, C",2,8 ,&CPU::BIT_1_C}, {"BIT 1, D",2,8 ,&CPU::BIT_1_D}, {"BIT 1, E",2,8 ,&CPU::BIT_1_E}, {"BIT 1, H",2,8 ,&CPU::BIT_1_H}, {"BIT 1, L",2,8 ,&CPU::BIT_1_L}, {"BIT 1, (HL)",2,12,&CPU::BIT_1_aHL}, {"BIT 1, A",2,8 ,&CPU::BIT_1_A},
		{"BIT 2, B",2,8 ,&CPU::BIT_2_B}, {"BIT 2, C",2,8 ,&CPU::BIT_2_C}, {"BIT 2, D",2,8 ,&CPU::BIT_2_D}, {"BIT 2, E",2,8 ,&CPU::BIT_2_E}, {"BIT 2, H",2,8 ,&CPU::BIT_2_H}, {"BIT 2, L",2,8 ,&CPU::BIT_2_L}, {"BIT 2, (HL)",2,12,&CPU::BIT_2_aHL}, {"BIT 2, A",2,8 ,&CPU::BIT_2_A}, {"BIT 3, B",2,8 ,&CPU::BIT_3_B}, {"BIT 3, C",2,8 ,&CPU::BIT_3_C}, {"BIT 3, D",2,8 ,&CPU::BIT_3_D}, {"BIT 3, E",2,8 ,&CPU::BIT_3_E}, {"BIT 3, H",2,8 ,&CPU::BIT_3_H}, {"BIT 3, L",2,8 ,&CPU::BIT_3_L}, {"BIT 3, (HL)",2,12,&CPU::BIT_3_aHL}, {"BIT 3, A",2,8 ,&CPU::BIT_3_A},
		{"BIT 4, B",2,8 ,&CPU::BIT_4_B}, {"BIT 4, C",2,8 ,&CPU::BIT_4_C}, {"BIT 4, D",2,8 ,&CPU::BIT_4_D}, {"BIT 4, E",2,8 ,&CPU::BIT_4_E}, {"BIT 4, H",2,8 ,&CPU::BIT_4_H}, {"BIT 4, L",2,8 ,&CPU::BIT_4_L}, {"BIT 4, (HL)",2,12,&CPU::BIT_4_aHL}, {"BIT 4, A",2,8 ,&CPU::BIT_4_A}, {"BIT 5, B",2,8 ,&CPU::BIT_5_B}, {"BIT 5, C",2,8 ,&CPU::BIT_5_C}, {"BIT 5, D",2,8 ,&CPU::BIT_5_D}, {"BIT 5, E",2,8 ,&CPU::BIT_5_E}, {"BIT 5, H",2,8 ,&CPU::BIT_5_H}, {"BIT 5, L",2,8 ,&CPU::BIT_5_L}, {"BIT 5, (HL)",2,12,&CPU::BIT_5_aHL}, {"BIT 5, A",2,8 ,&CPU::BIT_5_A},
		{"BIT 6, B",2,8 ,&CPU::BIT_6_B}, {"BIT 6, C",2,8 ,&CPU::BIT_6_C}, {"BIT 6, D",2,8 ,&CPU::BIT_6_D}, {"BIT 6, E",2,8 ,&CPU::BIT_6_E}, {"BIT 6, H",2,8 ,&CPU::BIT_6_H}, {"BIT 6, L",2,8 ,&CPU::BIT_6_L}, {"BIT 6, (HL)",2,12,&CPU::BIT_6_aHL}, {"BIT 6, A",2,8 ,&CPU::BIT_6_A}, {"BIT 7, B",2,8 ,&CPU::BIT_7_B}, {"BIT 7, C",2,8 ,&CPU::BIT_7_C}, {"BIT 7, D",2,8 ,&CPU::BIT_7_D}, {"BIT 7, E",2,8 ,&CPU::BIT_7_E}, {"BIT 7, H",2,8 ,&CPU::BIT_7_H}, {"BIT 7, L",2,8 ,&CPU::BIT_7_L}, {"BIT 7, (HL)",2,12,&CPU::BIT_7_aHL}, {"BIT 7, A",2,8 ,&CPU::BIT_7_A},
		{"RES 0, B",2,8 ,&CPU::RES_0_B}, {"RES 0, C",2,8 ,&CPU::RES_0_C}, {"RES 0, D",2,8 ,&CPU::RES_0_D}, {"RES 0, E",2,8 ,&CPU::RES_0_E}, {"RES 0, H",2,8 ,&CPU::RES_0_H}, {"RES 0, L",2,8 ,&CPU::RES_0_L}, {"RES 0, (HL)",2,16,&CPU::RES_0_aHL}, {"RES 0, A",2,8 ,&CPU::RES_0_A}, {"RES 1, B",2,8 ,&CPU::RES_1_B}, {"RES 1, C",2,8 ,&CPU::RES_1_C}, {"RES 1, D",2,8 ,&CPU::RES_1_D}, {"RES 1, E",2,8 ,&CPU::RES_1_E}, {"RES 1, H",2,8 ,&CPU::RES_1_H}, {"RES 1, L",2,8 ,&CPU::RES_1_L}, {"RES 1, (HL)",2,16,&CPU::RES_1_aHL}, {"RES 1, A",2,8 ,&CPU::RES_1_A},
		{"RES 2, B",2,8 ,&CPU::RES_2_B}, {"RES 2, C",2,8 ,&CPU::RES_2_C}, {"RES 2, D",2,8 ,&CPU::RES_2_D}, {"RES 2, E",2,8 ,&CPU::RES_2_E}, {"RES 2, H",2,8 ,&CPU::RES_2_H}, {"RES 2, L",2,8 ,&CPU::RES_2_L}, {"RES 2, (HL)",2,16,&CPU::RES_2_aHL}, {"RES 2, A",2,8 ,&CPU::RES_2_A}, {"RES 3, B",2,8 ,&CPU::RES_3_B}, {"RES 3, C",2,8 ,&CPU::RES_3_C}, {"RES 3, D",2,8 ,&CPU::RES_3_D}, {"RES 3, E",2,8 ,&CPU::RES_3_E}, {"RES 3, H",2,8 ,&CPU::RES_3_H}, {"RES 3, L",2,8 ,&CPU::RES_3_L}, {"RES 3, (HL)",2,16,&CPU::RES_3_aHL}, {"RES 3, A",2,8 ,&CPU::RES_3_A},
		{"RES 4, B",2,8 ,&CPU::RES_4_B}, {"RES 4, C",2,8 ,&CPU::RES_4_C}, {"RES 4, D",2,8 ,&CPU::RES_4_D}, {"RES 4, E",2,8 ,&CPU::RES_4_E}, {"RES 4, H",2,8 ,&CPU::RES_4_H}, {"RES 4, L",2,8 ,&CPU::RES_4_L}, {"RES 4, (HL)",2,16,&CPU::RES_4_aHL}, {"RES 4, A",2,8 ,&CPU::RES_4_A}, {"RES 5, B",2,8 ,&CPU::RES_5_B}, {"RES 5, C",2,8 ,&CPU::RES_5_C}, {"RES 5, D",2,8 ,&CPU::RES_5_D}, {"RES 5, E",2,8 ,&CPU::RES_5_E}, {"RES 5, H",2,8 ,&CPU::RES_5_H}, {"RES 5, L",2,8 ,&CPU::RES_5_L}, {"RES 5, (HL)",2,16,&CPU::RES_5_aHL}, {"RES 5, A",2,8 ,&CPU::RES_5_A},
		{"RES 6, B",2,8 ,&CPU::RES_6_B}, {"RES 6, C",2,8 ,&CPU::RES_6_C}, {"RES 6, D",2,8 ,&CPU::RES_6_D}, {"RES 6, E",2,8 ,&CPU::RES_6_E}, {"RES 6, H",2,8 ,&CPU::RES_6_H}, {"RES 6, L",2,8 ,&CPU::RES_6_L}, {"RES 6, (HL)",2,16,&CPU::RES_6_aHL}, {"RES 6, A",2,8 ,&CPU::RES_6_A}, {"RES 7, B",2,8 ,&CPU::RES_7_B}, {"RES 7, C",2,8 ,&CPU::RES_7_C}, {"RES 7, D",2,8 ,&CPU::RES_7_D}, {"RES 7, E",2,8 ,&CPU::RES_7_E}, {"RES 7, H",2,8 ,&CPU::RES_7_H}, {"RES 7, L",2,8 ,&CPU::RES_7_L}, {"RES 7, (HL)",2,16,&CPU::RES_7_aHL}, {"RES 7, A",2,8 ,&CPU::RES_7_A},
		{"SET 0, B",2,8 ,&CPU::SET_0_B}, {"SET 0, C",2,8 ,&CPU::SET_0_C}, {"SET 0, D",2,8 ,&CPU::SET_0_D}, {"SET 0, E",2,8 ,&CPU::SET_0_E}, {"SET 0, H",2,8 ,&CPU::SET_0_H}, {"SET 0, L",2,8 ,&CPU::SET_0_L}, {"SET 0, (HL)",2,16,&CPU::SET_0_aHL}, {"SET 0, A",2,8 ,&CPU::SET_0_A}, {"SET 1, B",2,8 ,&CPU::SET_1_B}, {"SET 1, C",2,8 ,&CPU::SET_1_C}, {"SET 1, D",2,8 ,&CPU::SET_1_D}, {"SET 1, E",2,8 ,&CPU::SET_1_E}, {"SET 1, H",2,8 ,&CPU::SET_1_H}, {"SET 1, L",2,8 ,&CPU::SET_1_L}, {"SET 1, (HL)",2,16,&CPU::SET_1_aHL}, {"SET 1, A",2,8 ,&CPU::SET_1_A},
		{"SET 2, B",2,8 ,&CPU::SET_2_B}, {"SET 2, C",2,8 ,&CPU::SET_2_C}, {"SET 2, D",2,8 ,&CPU::SET_2_D}, {"SET 2, E",2,8 ,&CPU::SET_2_E}, {"SET 2, H",2,8 ,&CPU::SET_2_H}, {"SET 2, L",2,8 ,&CPU::SET_2_L}, {"SET 2, (HL)",2,16,&CPU::SET_2_aHL}, {"SET 2, A",2,8 ,&CPU::SET_2_A}, {"SET 3, B",2,8 ,&CPU::SET_3_B}, {"SET 3, C",2,8 ,&CPU::SET_3_C}, {"SET 3, D",2,8 ,&CPU::SET_3_D}, {"SET 3, E",2,8 ,&CPU::SET_3_E}, {"SET 3, H",2,8 ,&CPU::SET_3_H}, {"SET 3, L",2,8 ,&CPU::SET_3_L}, {"SET 3, (HL)",2,16,&CPU::SET_3_aHL}, {"SET 3, A",2,8 ,&CPU::SET_3_A},
		{"SET 4, B",2,8 ,&CPU::SET_4_B}, {"SET 4, C",2,8 ,&CPU::SET_4_C}, {"SET 4, D",2,8 ,&CPU::SET_4_D}, {"SET 4, E",2,8 ,&CPU::SET_4_E}, {"SET 4, H",2,8 ,&CPU::SET_4_H}, {"SET 4, L",2,8 ,&CPU::SET_4_L}, {"SET 4, (HL)",2,16,&CPU::SET_4_aHL}, {"SET 4, A",2,8 ,&CPU::SET_4_A}, {"SET 5, B",2,8 ,&CPU::SET_5_B}, {"SET 5, C",2,8 ,&CPU::SET_5_C}, {"SET 5, D",2,8 ,&CPU::SET_5_D}, {"SET 5, E",2,8 ,&CPU::SET_5_E}, {"SET 5, H",2,8 ,&CPU::SET_5_H}, {"SET 5, L",2,8 ,&CPU::SET_5_L}, {"SET 5, (HL)",2,16,&CPU::SET_5_aHL}, {"SET 5, A",2,8 ,&CPU::SET_5_A},
		{"SET 6, B",2,8 ,&CPU::SET_6_B}, {"SET 6, C",2,8 ,&CPU::SET_6_C}, {"SET 6, D",2,8 ,&CPU::SET_6_D}, {"SET 6, E",2,8 ,&CPU::SET_6_E}, {"SET 6, H",2,8 ,&CPU::SET_6_H}, {"SET 6, L",2,8 ,&CPU::SET_6_L}, {"SET 6, (HL)",2,16,&CPU::SET_6_aHL}, {"SET 6, A",2,8 ,&CPU::SET_6_A}, {"SET 7, B",2,8 ,&CPU::SET_7_B}, {"SET 7, C",2,8 ,&CPU::SET_7_C}, {"SET 7, D",2,8 ,&CPU::SET_7_D}, {"SET 7, E",2,8 ,&CPU::SET_7_E}, {"SET 7, H",2,8 ,&CPU::SET_7_H}, {"SET 7, L",2,8 ,&CPU::SET_7_L}, {"SET 7, (HL)",2,16,&CPU::SET_7_aHL}, {"SET 7, A",2,8 ,&CPU::SET_7_A},
	};
#pragma endregion
#pragma region CPU Control Instructions
	u8 XXX(); // catch all non existing instructions8bit
	u8 NOP();
	u8 STOP();
	u8 DAA();
	u8 CPL();
	u8 SCF();
	u8 CCF();
	u8 HALT();
	u8 DI();
	u8 EI();
#pragma endregion
#pragma region Jump Instructions
	u8 JR_s8();
	u8 JR_NZ_s8();
	u8 JR_Z_s8();
	u8 JR_NC_s8();
	u8 JR_C_s8();
	u8 RET_NZ();
	u8 JP_NZ_a16();
	u8 JP_a16();
	u8 CALL_NZ_a16();
	u8 RST_0();
	u8 RET_Z();
	u8 RET();
	u8 JP_Z_a16();
	u8 CALL_Z_a16();
	u8 CALL_a16();
	u8 RST_1();
	u8 RET_NC();
	u8 JP_NC_a16();
	u8 CALL_NC_a16();
	u8 RST_2();
	u8 RET_C();
	u8 RETI();
	u8 JP_C_a16();
	u8 CALL_C_a16();
	u8 RST_3();
	u8 RST_4();
	u8 JP_HL();
	u8 RST_5();
	u8 RST_6();
	u8 RST_7();
#pragma endregion
#pragma region 8bit Load Instructions
	u8 LD_aBC_A();
	u8 LD_B_d8();
	u8 LD_A_aBC();
	u8 LD_C_d8();
	u8 LD_aDE_A();
	u8 LD_D_d8();
	u8 LD_A_aDE();
	u8 LD_E_d8();
	u8 LD_aHLinc_A();
	u8 LD_H_d8();
	u8 LD_A_aHLinc();
	u8 LD_L_d8();
	u8 LD_aHLdec_A();
	u8 LD_aHL_d8();
	u8 LD_A_aHLdec();
	u8 LD_A_d8();
	u8 LD_B_B();
	u8 LD_B_C();
	u8 LD_B_D();
	u8 LD_B_E();
	u8 LD_B_H();
	u8 LD_B_L();
	u8 LD_B_aHL();
	u8 LD_B_A();
	u8 LD_C_B();
	u8 LD_C_C();
	u8 LD_C_D();
	u8 LD_C_E();
	u8 LD_C_H();
	u8 LD_C_L();
	u8 LD_C_aHL();
	u8 LD_C_A();
	u8 LD_D_B();
	u8 LD_D_C();
	u8 LD_D_D();
	u8 LD_D_E();
	u8 LD_D_H();
	u8 LD_D_L();
	u8 LD_D_aHL();
	u8 LD_D_A();
	u8 LD_E_B();
	u8 LD_E_C();
	u8 LD_E_D();
	u8 LD_E_E();
	u8 LD_E_H();
	u8 LD_E_L();
	u8 LD_E_aHL();
	u8 LD_E_A();
	u8 LD_H_B();
	u8 LD_H_C();
	u8 LD_H_D();
	u8 LD_H_E();
	u8 LD_H_H();
	u8 LD_H_L();
	u8 LD_H_aHL();
	u8 LD_H_A();
	u8 LD_L_B();
	u8 LD_L_C();
	u8 LD_L_D();
	u8 LD_L_E();
	u8 LD_L_H();
	u8 LD_L_L();
	u8 LD_L_aHL();
	u8 LD_L_A();
	u8 LD_aHL_B();
	u8 LD_aHL_C();
	u8 LD_aHL_D();
	u8 LD_aHL_E();
	u8 LD_aHL_H();
	u8 LD_aHL_L();
	//u8 LD_aHL_HL(); // does not exist
	u8 LD_aHL_A();
	u8 LD_A_B();
	u8 LD_A_C();
	u8 LD_A_D();
	u8 LD_A_E();
	u8 LD_A_H();
	u8 LD_A_L();
	u8 LD_A_aHL();
	u8 LD_A_A();
	u8 LD_aa8_A();
	u8 LD_aC_A();
	u8 LD_aa16_A();
	u8 LD_A_aa8();
	u8 LD_A_aC();
	u8 LD_A_aa16();
#pragma endregion
#pragma region 16bit Load Instructions
	u8 LD_BC_d16();
	u8 LD_a16_SP();
	u8 LD_DE_d16();
	u8 LD_HL_d16();
	u8 LD_SP_d16();
	u8 POP_BC();
	u8 PUSH_BC();
	u8 POP_DE();
	u8 PUSH_DE();
	u8 POP_HL();
	u8 PUSH_HL();
	u8 POP_AF();
	u8 PUSH_AF();
	u8 LD_HL_SPinc_s8();
	u8 LD_SP_HL();
#pragma endregion
#pragma region 8bit Arithmetic Instructions
	u8 INC_B();
	u8 DEC_B();
	u8 INC_C();
	u8 DEC_C();
	u8 INC_D();
	u8 DEC_D();
	u8 INC_E();
	u8 DEC_E(); 
	u8 INC_H();
	u8 DEC_H();
	u8 INC_L();
	u8 DEC_L();
	u8 INC_aHL();
	u8 DEC_aHL();
	u8 INC_A();
	u8 DEC_A();
	u8 ADD_A_B();
	u8 ADD_A_C();
	u8 ADD_A_D();
	u8 ADD_A_E();
	u8 ADD_A_H();
	u8 ADD_A_L();
	u8 ADD_A_aHL();
	u8 ADD_A_A();
	u8 ADC_A_B();
	u8 ADC_A_C();
	u8 ADC_A_D();
	u8 ADC_A_E();
	u8 ADC_A_H();
	u8 ADC_A_L();
	u8 ADC_A_aHL();
	u8 ADC_A_A();
	u8 SUB_A_B();
	u8 SUB_A_C();
	u8 SUB_A_D();
	u8 SUB_A_E();
	u8 SUB_A_H();
	u8 SUB_A_L();
	u8 SUB_A_aHL();
	u8 SUB_A_A();
	u8 SBC_A_B();
	u8 SBC_A_C();
	u8 SBC_A_D();
	u8 SBC_A_E();
	u8 SBC_A_H();
	u8 SBC_A_L();
	u8 SBC_A_aHL();
	u8 SBC_A_A();
	u8 AND_A_B();
	u8 AND_A_C();
	u8 AND_A_D();
	u8 AND_A_E();
	u8 AND_A_H();
	u8 AND_A_L();
	u8 AND_A_aHL();
	u8 AND_A_A();
	u8 XOR_A_B();
	u8 XOR_A_C();
	u8 XOR_A_D();
	u8 XOR_A_E();
	u8 XOR_A_H();
	u8 XOR_A_L();
	u8 XOR_A_aHL();
	u8 XOR_A_A();
	u8 OR_A_B();
	u8 OR_A_C();
	u8 OR_A_D();
	u8 OR_A_E();
	u8 OR_A_H();
	u8 OR_A_L();
	u8 OR_A_aHL();
	u8 OR_A_A();
	u8 CP_A_B();
	u8 CP_A_C();
	u8 CP_A_D();
	u8 CP_A_E();
	u8 CP_A_H();
	u8 CP_A_L();
	u8 CP_A_aHL();
	u8 CP_A_A();
	u8 ADD_A_d8();
	u8 ADC_A_d8();
	u8 SUB_A_d8();
	u8 SBC_A_d8();
	u8 AND_A_d8();
	u8 XOR_A_d8();
	u8 OR_A_d8();
	u8 CP_A_d8();
#pragma endregion
#pragma region 16bit Arithmetic Instructions
	u8 INC_BC();
	u8 ADD_HL_BC();
	u8 DEC_BC();
	u8 INC_DE();
	u8 ADD_HL_DE();
	u8 DEC_DE();
	u8 INC_HL();
	u8 ADD_HL_HL();
	u8 DEC_HL();
	u8 INC_SP();
	u8 ADD_HL_SP();
	u8 DEC_SP();
	u8 ADD_SP_s8();
#pragma endregion
#pragma region 8bit bit instructions
	u8 RLCA();
	u8 RRCA();
	u8 RLA();
	u8 RRA();
	u8 RLC_B();
	u8 RLC_C();
	u8 RLC_D();
	u8 RLC_E();
	u8 RLC_H();
	u8 RLC_L();
	u8 RLC_aHL();
	u8 RLC_A();

	u8 RRC_B();
	u8 RRC_C();
	u8 RRC_D();
	u8 RRC_E();
	u8 RRC_H();
	u8 RRC_L();
	u8 RRC_aHL();
	u8 RRC_A();

	u8 RL_B();
	u8 RL_C();
	u8 RL_D();
	u8 RL_E();
	u8 RL_H();
	u8 RL_L();
	u8 RL_aHL();
	u8 RL_A();

	u8 RR_B();
	u8 RR_C();
	u8 RR_D();
	u8 RR_E();
	u8 RR_H();
	u8 RR_L();
	u8 RR_aHL();
	u8 RR_A();

	u8 SLA_B();
	u8 SLA_C();
	u8 SLA_D();
	u8 SLA_E();
	u8 SLA_H();
	u8 SLA_L();
	u8 SLA_aHL();
	u8 SLA_A();

	u8 SRA_B();
	u8 SRA_C();
	u8 SRA_D();
	u8 SRA_E();
	u8 SRA_H();
	u8 SRA_L();
	u8 SRA_aHL();
	u8 SRA_A();

	u8 SWAP_B();
	u8 SWAP_C();
	u8 SWAP_D();
	u8 SWAP_E();
	u8 SWAP_H();
	u8 SWAP_L();
	u8 SWAP_aHL();
	u8 SWAP_A();

	u8 SRL_B();
	u8 SRL_C();
	u8 SRL_D();
	u8 SRL_E();
	u8 SRL_H();
	u8 SRL_L();
	u8 SRL_aHL();
	u8 SRL_A();

	u8 BIT_0_B();
	u8 BIT_0_C();
	u8 BIT_0_D();
	u8 BIT_0_E();
	u8 BIT_0_H();
	u8 BIT_0_L();
	u8 BIT_0_aHL();
	u8 BIT_0_A();
	u8 BIT_1_B();
	u8 BIT_1_C();
	u8 BIT_1_D();
	u8 BIT_1_E();
	u8 BIT_1_H();
	u8 BIT_1_L();
	u8 BIT_1_aHL();
	u8 BIT_1_A();
	u8 BIT_2_B();
	u8 BIT_2_C();
	u8 BIT_2_D();
	u8 BIT_2_E();
	u8 BIT_2_H();
	u8 BIT_2_L();
	u8 BIT_2_aHL();
	u8 BIT_2_A();
	u8 BIT_3_B();
	u8 BIT_3_C();
	u8 BIT_3_D();
	u8 BIT_3_E();
	u8 BIT_3_H();
	u8 BIT_3_L();
	u8 BIT_3_aHL();
	u8 BIT_3_A();
	u8 BIT_4_B();
	u8 BIT_4_C();
	u8 BIT_4_D();
	u8 BIT_4_E();
	u8 BIT_4_H();
	u8 BIT_4_L();
	u8 BIT_4_aHL();
	u8 BIT_4_A();
	u8 BIT_5_B();
	u8 BIT_5_C();
	u8 BIT_5_D();
	u8 BIT_5_E();
	u8 BIT_5_H();
	u8 BIT_5_L();
	u8 BIT_5_aHL();
	u8 BIT_5_A();
	u8 BIT_6_B();
	u8 BIT_6_C();
	u8 BIT_6_D();
	u8 BIT_6_E();
	u8 BIT_6_H();
	u8 BIT_6_L();
	u8 BIT_6_aHL();
	u8 BIT_6_A();
	u8 BIT_7_B();
	u8 BIT_7_C();
	u8 BIT_7_D();
	u8 BIT_7_E();
	u8 BIT_7_H();
	u8 BIT_7_L();
	u8 BIT_7_aHL();
	u8 BIT_7_A();

	u8 RES_0_B();
	u8 RES_0_C();
	u8 RES_0_D();
	u8 RES_0_E();
	u8 RES_0_H();
	u8 RES_0_L();
	u8 RES_0_aHL();
	u8 RES_0_A();
	u8 RES_1_B();
	u8 RES_1_C();
	u8 RES_1_D();
	u8 RES_1_E();
	u8 RES_1_H();
	u8 RES_1_L();
	u8 RES_1_aHL();
	u8 RES_1_A();
	u8 RES_2_B();
	u8 RES_2_C();
	u8 RES_2_D();
	u8 RES_2_E();
	u8 RES_2_H();
	u8 RES_2_L();
	u8 RES_2_aHL();
	u8 RES_2_A();
	u8 RES_3_B();
	u8 RES_3_C();
	u8 RES_3_D();
	u8 RES_3_E();
	u8 RES_3_H();
	u8 RES_3_L();
	u8 RES_3_aHL();
	u8 RES_3_A();
	u8 RES_4_B();
	u8 RES_4_C();
	u8 RES_4_D();
	u8 RES_4_E();
	u8 RES_4_H();
	u8 RES_4_L();
	u8 RES_4_aHL();
	u8 RES_4_A();
	u8 RES_5_B();
	u8 RES_5_C();
	u8 RES_5_D();
	u8 RES_5_E();
	u8 RES_5_H();
	u8 RES_5_L();
	u8 RES_5_aHL();
	u8 RES_5_A();
	u8 RES_6_B();
	u8 RES_6_C();
	u8 RES_6_D();
	u8 RES_6_E();
	u8 RES_6_H();
	u8 RES_6_L();
	u8 RES_6_aHL();
	u8 RES_6_A();
	u8 RES_7_B();
	u8 RES_7_C();
	u8 RES_7_D();
	u8 RES_7_E();
	u8 RES_7_H();
	u8 RES_7_L();
	u8 RES_7_aHL();
	u8 RES_7_A();

	u8 SET_0_B();
	u8 SET_0_C();
	u8 SET_0_D();
	u8 SET_0_E();
	u8 SET_0_H();
	u8 SET_0_L();
	u8 SET_0_aHL();
	u8 SET_0_A();
	u8 SET_1_B();
	u8 SET_1_C();
	u8 SET_1_D();
	u8 SET_1_E();
	u8 SET_1_H();
	u8 SET_1_L();
	u8 SET_1_aHL();
	u8 SET_1_A();
	u8 SET_2_B();
	u8 SET_2_C();
	u8 SET_2_D();
	u8 SET_2_E();
	u8 SET_2_H();
	u8 SET_2_L();
	u8 SET_2_aHL();
	u8 SET_2_A();
	u8 SET_3_B();
	u8 SET_3_C();
	u8 SET_3_D();
	u8 SET_3_E();
	u8 SET_3_H();
	u8 SET_3_L();
	u8 SET_3_aHL();
	u8 SET_3_A();
	u8 SET_4_B();
	u8 SET_4_C();
	u8 SET_4_D();
	u8 SET_4_E();
	u8 SET_4_H();
	u8 SET_4_L();
	u8 SET_4_aHL();
	u8 SET_4_A();
	u8 SET_5_B();
	u8 SET_5_C();
	u8 SET_5_D();
	u8 SET_5_E();
	u8 SET_5_H();
	u8 SET_5_L();
	u8 SET_5_aHL();
	u8 SET_5_A();
	u8 SET_6_B();
	u8 SET_6_C();
	u8 SET_6_D();
	u8 SET_6_E();
	u8 SET_6_H();
	u8 SET_6_L();
	u8 SET_6_aHL();
	u8 SET_6_A();
	u8 SET_7_B();
	u8 SET_7_C();
	u8 SET_7_D();
	u8 SET_7_E();
	u8 SET_7_H();
	u8 SET_7_L();
	u8 SET_7_aHL();
	u8 SET_7_A();
#pragma endregion
};