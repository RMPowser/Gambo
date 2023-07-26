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
	int opcodeTimingDelay;			// if this value is less than 0, we have finished an instruction
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
		{"$00 NOP"         ,1,4 ,&CPU::NOP     }, {"$01 LD BC, {}" ,3,12,&CPU::LD_BC_d16 }, {"$02 LD (BC), A"  ,1,8 ,&CPU::LD_aBC_A   }, {"$03 INC BC"    ,1,8 ,&CPU::INC_BC  }, {"$04 INC B"       ,1,4 ,&CPU::INC_B      }, {"$05 DEC B"     ,1,4 ,&CPU::DEC_B   }, {"$06 LD B, {}"   ,2,8 ,&CPU::LD_B_d8  }, {"$07 RLCA"      ,1,4 ,&CPU::RLCA    }, {"$08 LD ({}), SP" ,3,20,&CPU::LD_a16_SP     }, {"$09 ADD HL, BC",1,8 ,&CPU::ADD_HL_BC}, {"$0A LD A, (BC)"  ,1,8 ,&CPU::LD_A_aBC   }, {"$0B DEC BC"	      ,1,8,&CPU::DEC_BC }, {"$0C INC C"      ,1,4 ,&CPU::INC_C     }, {"$0D DEC C"   ,1,4 ,&CPU::DEC_C   }, {"$0E LD C, {}"   ,2,8 ,&CPU::LD_C_d8  }, {"$0F RRCA"    ,1,4 ,&CPU::RRCA   },
		{"$10 STOP"        ,1,4 ,&CPU::STOP    }, {"$11 LD DE, {}" ,3,12,&CPU::LD_DE_d16 }, {"$12 LD (DE), A"  ,1,8 ,&CPU::LD_aDE_A   }, {"$13 INC DE"    ,1,8 ,&CPU::INC_DE  }, {"$14 INC D"       ,1,4 ,&CPU::INC_D      }, {"$15 DEC D"     ,1,4 ,&CPU::DEC_D   }, {"$16 LD D, {}"   ,2,8 ,&CPU::LD_D_d8  }, {"$17 RLA"       ,1,4 ,&CPU::RLA     }, {"$18 JR {}"       ,2,12,&CPU::JR_s8         }, {"$19 ADD HL, DE",1,8 ,&CPU::ADD_HL_DE}, {"$1A LD A, (DE)"  ,1,8 ,&CPU::LD_A_aDE   }, {"$1B DEC DE"	      ,1,8,&CPU::DEC_DE }, {"$1C INC E"      ,1,4 ,&CPU::INC_E     }, {"$1D DEC E"   ,1,4 ,&CPU::DEC_E   }, {"$1E LD E, {}"   ,2,8 ,&CPU::LD_E_d8  }, {"$1F RRA"     ,1,4 ,&CPU::RRA    },
		{"$20 JR NZ, {}"   ,2,8 ,&CPU::JR_NZ_s8}, {"$21 LD HL, {}" ,3,12,&CPU::LD_HL_d16 }, {"$22 LD (HL)++, A",1,8 ,&CPU::LD_aHLinc_A}, {"$23 INC HL"    ,1,8 ,&CPU::INC_HL  }, {"$24 INC H"       ,1,4 ,&CPU::INC_H      }, {"$25 DEC H"     ,1,4 ,&CPU::DEC_H   }, {"$26 LD H, {}"   ,2,8 ,&CPU::LD_H_d8  }, {"$27 DAA"       ,1,4 ,&CPU::DAA     }, {"$28 JR Z, {}"    ,2,8 ,&CPU::JR_Z_s8       }, {"$29 ADD HL, HL",1,8 ,&CPU::ADD_HL_HL}, {"$2A LD A, (HL)++",1,8 ,&CPU::LD_A_aHLinc}, {"$2B DEC HL"	      ,1,8,&CPU::DEC_HL }, {"$2C INC L"      ,1,4 ,&CPU::INC_L     }, {"$2D DEC L"   ,1,4 ,&CPU::DEC_L   }, {"$2E LD L, {}"   ,2,8 ,&CPU::LD_L_d8  }, {"$2F CPL"     ,1,4 ,&CPU::CPL    },
		{"$30 JR NC, {}"   ,2,8 ,&CPU::JR_NC_s8}, {"$31 LD SP, {}" ,3,12,&CPU::LD_SP_d16 }, {"$32 LD (HL)--, A",1,8 ,&CPU::LD_aHLdec_A}, {"$33 INC SP"    ,1,8 ,&CPU::INC_SP  }, {"$34 INC (HL)"    ,1,12,&CPU::INC_aHL    }, {"$35 DEC (HL)"  ,1,12,&CPU::DEC_aHL }, {"$36 LD (HL), {}",2,12,&CPU::LD_aHL_d8}, {"$37 SCF"       ,1,4 ,&CPU::SCF     }, {"$38 JR C, {}"    ,2,8 ,&CPU::JR_C_s8       }, {"$39 ADD HL, SP",1,8 ,&CPU::ADD_HL_SP}, {"$3A LD A, (HL)--",1,8 ,&CPU::LD_A_aHLdec}, {"$3B DEC SP"	      ,1,8,&CPU::DEC_SP }, {"$3C INC A"      ,1,4 ,&CPU::INC_A     }, {"$3D DEC A"   ,1,4 ,&CPU::DEC_A   }, {"$3E LD A, {}"   ,2,8 ,&CPU::LD_A_d8  }, {"$3F CCF"     ,1,4 ,&CPU::CCF    },
		{"$40 LD B, B"     ,1,4 ,&CPU::LD_B_B  }, {"$41 LD B, C"   ,1,4 ,&CPU::LD_B_C    }, {"$42 LD B, D"     ,1,4 ,&CPU::LD_B_D     }, {"$43 LD B, E"   ,1,4 ,&CPU::LD_B_E  }, {"$44 LD B, H"     ,1,4 ,&CPU::LD_B_H     }, {"$45 LD B, L"   ,1,4 ,&CPU::LD_B_L  }, {"$46 LD B, (HL)" ,1,8 ,&CPU::LD_B_aHL }, {"$47 LD B, A"   ,1,4 ,&CPU::LD_B_A  }, {"$48 LD C, B"     ,1,4 ,&CPU::LD_C_B        }, {"$49 LD C, C"   ,1,4 ,&CPU::LD_C_C   }, {"$4A LD C, D"     ,1,4 ,&CPU::LD_C_D     }, {"$4B LD C, E"	  ,1,4,&CPU::LD_C_E }, {"$4C LD C, H"    ,1,4 ,&CPU::LD_C_H    }, {"$4D LD C, L" ,1,4 ,&CPU::LD_C_L  }, {"$4E LD C, (HL)" ,1,8 ,&CPU::LD_C_aHL }, {"$4F LD C, A" ,1,4 ,&CPU::LD_C_A },
		{"$50 LD D, B"     ,1,4 ,&CPU::LD_D_B  }, {"$51 LD D, C"   ,1,4 ,&CPU::LD_D_C    }, {"$52 LD D, D"     ,1,4 ,&CPU::LD_D_D     }, {"$53 LD D, E"   ,1,4 ,&CPU::LD_D_E  }, {"$54 LD D, H"     ,1,4 ,&CPU::LD_D_H     }, {"$55 LD D, L"   ,1,4 ,&CPU::LD_D_L  }, {"$56 LD D, (HL)" ,1,8 ,&CPU::LD_D_aHL }, {"$57 LD D, A"   ,1,4 ,&CPU::LD_D_A  }, {"$58 LD E, B"     ,1,4 ,&CPU::LD_E_B        }, {"$59 LD E, C"   ,1,4 ,&CPU::LD_E_C   }, {"$5A LD E, D"     ,1,4 ,&CPU::LD_E_D     }, {"$5B LD E, E"	  ,1,4,&CPU::LD_E_E }, {"$5C LD E, H"    ,1,4 ,&CPU::LD_E_H    }, {"$5D LD E, L" ,1,4 ,&CPU::LD_E_L  }, {"$5E LD E, (HL)" ,1,8 ,&CPU::LD_E_aHL }, {"$5F LD E, A" ,1,4 ,&CPU::LD_E_A },
		{"$60 LD H, B"     ,1,4 ,&CPU::LD_H_B  }, {"$61 LD H, C"   ,1,4 ,&CPU::LD_H_C    }, {"$62 LD H, D"     ,1,4 ,&CPU::LD_H_D     }, {"$63 LD H, E"   ,1,4 ,&CPU::LD_H_E  }, {"$64 LD H, H"     ,1,4 ,&CPU::LD_H_H     }, {"$65 LD H, L"   ,1,4 ,&CPU::LD_H_L  }, {"$66 LD H, (HL)" ,1,8 ,&CPU::LD_H_aHL }, {"$67 LD H, A"   ,1,4 ,&CPU::LD_H_A  }, {"$68 LD L, B"     ,1,4 ,&CPU::LD_L_B        }, {"$69 LD L, C"   ,1,4 ,&CPU::LD_L_C   }, {"$6A LD L, D"     ,1,4 ,&CPU::LD_L_D     }, {"$6B LD L, E"	  ,1,4,&CPU::LD_L_E }, {"$6C LD L, H"    ,1,4 ,&CPU::LD_L_H    }, {"$6D LD L, L" ,1,4 ,&CPU::LD_L_L  }, {"$6E LD L, (HL)" ,1,8 ,&CPU::LD_L_aHL }, {"$6F LD L, A" ,1,4 ,&CPU::LD_L_A },
		{"$70 LD (HL), B"  ,1,8 ,&CPU::LD_aHL_B}, {"$71 LD (HL), C",1,8 ,&CPU::LD_aHL_C  }, {"$72 LD (HL), D"  ,1,8 ,&CPU::LD_aHL_D   }, {"$73 LD (HL), E",1,8 ,&CPU::LD_aHL_E}, {"$74 LD (HL), H"  ,1,8 ,&CPU::LD_aHL_H   }, {"$75 LD (HL), L",1,8 ,&CPU::LD_aHL_L}, {"$76 HALT"       ,1,4 ,&CPU::HALT     }, {"$77 LD (HL), A",1,8 ,&CPU::LD_aHL_A}, {"$78 LD A, B"     ,1,4 ,&CPU::LD_A_B        }, {"$79 LD A, C"   ,1,4 ,&CPU::LD_A_C   }, {"$7A LD A, D"     ,1,4 ,&CPU::LD_A_D     }, {"$7B LD A, E"	  ,1,4,&CPU::LD_A_E }, {"$7C LD A, H"    ,1,4 ,&CPU::LD_A_H    }, {"$7D LD A, L" ,1,4 ,&CPU::LD_A_L  }, {"$7E LD A, (HL)" ,1,8 ,&CPU::LD_A_aHL }, {"$7F LD A, A" ,1,4 ,&CPU::LD_A_A },
		{"$80 ADD A, B"    ,1,4 ,&CPU::ADD_A_B }, {"$81 ADD A, C"  ,1,4 ,&CPU::ADD_A_C   }, {"$82 ADD A, D"    ,1,4 ,&CPU::ADD_A_D    }, {"$83 ADD A, E"  ,1,4 ,&CPU::ADD_A_E }, {"$84 ADD A, H"    ,1,4 ,&CPU::ADD_A_H    }, {"$85 ADD A, L"  ,1,4 ,&CPU::ADD_A_L }, {"$86 ADD A, (HL)",1,8 ,&CPU::ADD_A_aHL}, {"$87 ADD A, A"  ,1,4 ,&CPU::ADD_A_A }, {"$88 ADC A, B"    ,1,4 ,&CPU::ADC_A_B       }, {"$89 ADC A, C"  ,1,4 ,&CPU::ADC_A_C  }, {"$8A ADC A, D"    ,1,4 ,&CPU::ADC_A_D    }, {"$8B ADC A, E"	  ,1,4,&CPU::ADC_A_E}, {"$8C ADC A, H"   ,1,4 ,&CPU::ADC_A_H   }, {"$8D ADC A, L",1,4 ,&CPU::ADC_A_L }, {"$8E ADC A, (HL)",1,8 ,&CPU::ADC_A_aHL}, {"$8F ADC A, A",1,4 ,&CPU::ADC_A_A},
		{"$90 SUB A, B"    ,1,4 ,&CPU::SUB_A_B }, {"$91 SUB A, C"  ,1,4 ,&CPU::SUB_A_C   }, {"$92 SUB A, D"    ,1,4 ,&CPU::SUB_A_D    }, {"$93 SUB A, E"  ,1,4 ,&CPU::SUB_A_E }, {"$94 SUB A, H"    ,1,4 ,&CPU::SUB_A_H    }, {"$95 SUB A, L"  ,1,4 ,&CPU::SUB_A_L }, {"$96 SUB A, (HL)",1,8 ,&CPU::SUB_A_aHL}, {"$97 SUB A, A"  ,1,4 ,&CPU::SUB_A_A }, {"$98 SBC A, B"    ,1,4 ,&CPU::SBC_A_B       }, {"$99 SBC A, C"  ,1,4 ,&CPU::SBC_A_C  }, {"$9A SBC A, D"    ,1,4 ,&CPU::SBC_A_D    }, {"$9B SBC A, E"	  ,1,4,&CPU::SBC_A_E}, {"$9C SBC A, H"   ,1,4 ,&CPU::SBC_A_H   }, {"$9D SBC A, L",1,4 ,&CPU::SBC_A_L }, {"$9E SBC A, (HL)",1,8 ,&CPU::SBC_A_aHL}, {"$9F SBC A, A",1,4 ,&CPU::SBC_A_A},
		{"$A0 AND A, B"    ,1,4 ,&CPU::AND_A_B }, {"$A1 AND A, C"  ,1,4 ,&CPU::AND_A_C   }, {"$A2 AND A, D"    ,1,4 ,&CPU::AND_A_D    }, {"$A3 AND A, E"  ,1,4 ,&CPU::AND_A_E }, {"$A4 AND A, H"    ,1,4 ,&CPU::AND_A_H    }, {"$A5 AND A, L"  ,1,4 ,&CPU::AND_A_L }, {"$A6 AND A, (HL)",1,8 ,&CPU::AND_A_aHL}, {"$A7 AND A, A"  ,1,4 ,&CPU::AND_A_A }, {"$A8 XOR A, B"    ,1,4 ,&CPU::XOR_A_B       }, {"$A9 XOR A, C"  ,1,4 ,&CPU::XOR_A_C  }, {"$AA XOR A, D"    ,1,4 ,&CPU::XOR_A_D    }, {"$AB XOR A, E"	  ,1,4,&CPU::XOR_A_E}, {"$AC XOR A, H"   ,1,4 ,&CPU::XOR_A_H   }, {"$AD XOR A, L",1,4 ,&CPU::XOR_A_L }, {"$AE XOR A, (HL)",1,8 ,&CPU::XOR_A_aHL}, {"$AF XOR A, A",1,4 ,&CPU::XOR_A_A},
		{"$B0 OR A, B"     ,1,4 ,&CPU::OR_A_B  }, {"$B1 OR A, C"   ,1,4 ,&CPU::OR_A_C    }, {"$B2 OR A, D"     ,1,4 ,&CPU::OR_A_D     }, {"$B3 OR A, E"   ,1,4 ,&CPU::OR_A_E  }, {"$B4 OR A, H"     ,1,4 ,&CPU::OR_A_H     }, {"$B5 OR A, L"   ,1,4 ,&CPU::OR_A_L  }, {"$B6 OR A, (HL)" ,1,8 ,&CPU::OR_A_aHL }, {"$B7 OR A, A"   ,1,4 ,&CPU::OR_A_A  }, {"$B8 CP A, B"     ,1,4 ,&CPU::CP_A_B        }, {"$B9 CP A, C"   ,1,4 ,&CPU::CP_A_C   }, {"$BA CP A, D"     ,1,4 ,&CPU::CP_A_D     }, {"$BB CP A, E"	  ,1,4,&CPU::CP_A_E }, {"$BC CP A, H"    ,1,4 ,&CPU::CP_A_H    }, {"$BD CP A, L" ,1,4 ,&CPU::CP_A_L  }, {"$BE CP A, (HL)" ,1,8 ,&CPU::CP_A_aHL }, {"$BF CP A, A" ,1,4 ,&CPU::CP_A_A },
		{"$C0 RET NZ"      ,1,8 ,&CPU::RET_NZ  }, {"$C1 POP BC"    ,1,12,&CPU::POP_BC    }, {"$C2 JP NZ, {}"   ,3,12,&CPU::JP_NZ_a16  }, {"$C3 JP {}"     ,3,16,&CPU::JP_a16  }, {"$C4 CALL NZ, {}" ,3,12,&CPU::CALL_NZ_a16}, {"$C5 PUSH BC"   ,1,16,&CPU::PUSH_BC }, {"$C6 ADD A, {}"  ,2,8 ,&CPU::ADD_A_d8 }, {"$C7 RST 0"     ,1,16,&CPU::RST_0   }, {"$C8 RET Z"       ,1,8 ,&CPU::RET_Z         }, {"$C9 RET"       ,1,16,&CPU::RET      }, {"$CA JP Z, {}"    ,3,12,&CPU::JP_Z_a16   }, {"$CB 16-Bit Prefix",0,0,&CPU::XXX    }, {"$CC CALL Z, {}" ,3,12,&CPU::CALL_Z_a16}, {"$CD CALL {}" ,3,24,&CPU::CALL_a16}, {"$CE ADC A, {}"  ,2,8 ,&CPU::ADC_A_d8 }, {"$CF RST 1"   ,1,16,&CPU::RST_1  },
		{"$D0 RET NC"      ,1,8 ,&CPU::RET_NC  }, {"$D1 POP DE"    ,1,12,&CPU::POP_DE    }, {"$D2 JP NC, {}"   ,3,12,&CPU::JP_NC_a16  }, {"$D3 ???"       ,0,0 ,&CPU::XXX     }, {"$D4 CALL NC, {}" ,3,12,&CPU::CALL_NC_a16}, {"$D5 PUSH DE"   ,1,16,&CPU::PUSH_DE }, {"$D6 SUB A, {}"  ,2,8 ,&CPU::SUB_A_d8 }, {"$D7 RST 2"     ,1,16,&CPU::RST_2   }, {"$D8 RET C"       ,1,8 ,&CPU::RET_C         }, {"$D9 RETI"      ,1,16,&CPU::RETI     }, {"$DA JP C, {}"    ,3,12,&CPU::JP_C_a16   }, {"$DB ???"          ,0,0,&CPU::XXX    }, {"$DC CALL C, {}" ,3,12,&CPU::CALL_C_a16}, {"$DD ???"     ,0,0 ,&CPU::XXX     }, {"$DE SBC A, {}"  ,2,8 ,&CPU::SBC_A_d8 }, {"$DF RST 3"   ,1,16,&CPU::RST_3  },
		{"$E0 LD (FF{}), A",2,12,&CPU::LD_aa8_A}, {"$E1 POP HL"    ,1,12,&CPU::POP_HL    }, {"$E2 LD (C), A"   ,1,8 ,&CPU::LD_aC_A    }, {"$E3 ???"       ,0,0 ,&CPU::XXX     }, {"$E4 ???"         ,0,0 ,&CPU::XXX        }, {"$E5 PUSH HL"   ,1,16,&CPU::PUSH_HL }, {"$E6 AND A, {}"  ,2,8 ,&CPU::AND_A_d8 }, {"$E7 RST 4"     ,1,16,&CPU::RST_4   }, {"$E8 ADD SP, {}"  ,2,16,&CPU::ADD_SP_s8     }, {"$E9 JP HL"     ,1,4 ,&CPU::JP_HL    }, {"$EA LD ({}), A"  ,3,16,&CPU::LD_aa16_A  }, {"$EB ???"          ,0,0,&CPU::XXX    }, {"$EC ???"        ,0,0 ,&CPU::XXX       }, {"$ED ???"     ,0,0 ,&CPU::XXX     }, {"$EE XOR A, {}"  ,2,8 ,&CPU::XOR_A_d8 }, {"$EF RST 5"   ,1,16,&CPU::RST_5  },
		{"$F0 LD A, (FF{})",2,12,&CPU::LD_A_aa8}, {"$F1 POP AF"    ,1,12,&CPU::POP_AF    }, {"$F2 LD A, (C)"   ,1,8 ,&CPU::LD_A_aC    }, {"$F3 DI"        ,1,4 ,&CPU::DI      }, {"$F4 ???"         ,0,0 ,&CPU::XXX        }, {"$F5 PUSH AF"   ,1,16,&CPU::PUSH_AF }, {"$F6 OR A, {}"   ,2,8 ,&CPU::OR_A_d8  }, {"$F7 RST 6"     ,1,16,&CPU::RST_6   }, {"$F8 LD HL, SP+{}",2,12,&CPU::LD_HL_SPinc_s8}, {"$F9 LD SP, HL" ,1,8 ,&CPU::LD_SP_HL }, {"$FA LD A, ({})"  ,3,16,&CPU::LD_A_aa16  }, {"$FB EI"           ,1,4,&CPU::EI     }, {"$FC ???"        ,0,0 ,&CPU::XXX       }, {"$FD ???"     ,0,0 ,&CPU::XXX     }, {"$FE CP A, {}"   ,2,8 ,&CPU::CP_A_d8  }, {"$FF RST 7"   ,1,16,&CPU::RST_7  },
	};
	const std::vector<CPUInstruction> instructions16bit =
	{
		{"$CB00 RLC B"   ,2,8,&CPU::RLC_B  }, {"$CB01 RLC C"   ,2,8,&CPU::RLC_C  }, {"$CB02 RLC D"   ,2,8,&CPU::RLC_D  }, {"$CB03 RLC E"   ,2,8,&CPU::RLC_E  }, {"$CB04 RLC H"   ,2,8,&CPU::RLC_H  }, {"$CB05 RLC L"   ,2,8,&CPU::RLC_L  }, {"$CB06 RLC (HL)"   ,2,16,&CPU::RLC_aHL  }, {"$CB07 RLC A"   ,2,8,&CPU::RLC_A  }, {"$CB08 RRC B"   ,2,8,&CPU::RRC_B  }, {"$CB09 RRC C"   ,2,8,&CPU::RRC_C  }, {"$CB0A RRC D"   ,2,8,&CPU::RRC_D  }, {"$CB0B RRC E"   ,2,8,&CPU::RRC_E  }, {"$CB0C RRC H"   ,2,8,&CPU::RRC_H  }, {"$CB0D RRC L"   ,2,8,&CPU::RRC_L  }, {"$CB0E RRC (HL)"   ,2,16,&CPU::RRC_aHL  }, {"$CB0F RRC A"   ,2,8,&CPU::RRC_A  },
		{"$CB10 RL B"    ,2,8,&CPU::RL_B   }, {"$CB11 RL C"    ,2,8,&CPU::RL_C   }, {"$CB12 RL D"    ,2,8,&CPU::RL_D   }, {"$CB13 RL E"    ,2,8,&CPU::RL_E   }, {"$CB14 RL H"    ,2,8,&CPU::RL_H   }, {"$CB15 RL L"    ,2,8,&CPU::RL_L   }, {"$CB16 RL (HL)"    ,2,16,&CPU::RL_aHL   }, {"$CB17 RL A"    ,2,8,&CPU::RL_A   }, {"$CB18 RR B"    ,2,8,&CPU::RR_B   }, {"$CB19 RR C"    ,2,8,&CPU::RR_C   }, {"$CB1A RR D"    ,2,8,&CPU::RR_D   }, {"$CB1B RR E"    ,2,8,&CPU::RR_E   }, {"$CB1C RR H"    ,2,8,&CPU::RR_H   }, {"$CB1D RR L"    ,2,8,&CPU::RR_L   }, {"$CB1E RR (HL)"    ,2,16,&CPU::RR_aHL   }, {"$CB1F RR A"    ,2,8,&CPU::RR_A   },
		{"$CB20 SLA B"   ,2,8,&CPU::SLA_B  }, {"$CB21 SLA C"   ,2,8,&CPU::SLA_C  }, {"$CB22 SLA D"   ,2,8,&CPU::SLA_D  }, {"$CB23 SLA E"   ,2,8,&CPU::SLA_E  }, {"$CB24 SLA H"   ,2,8,&CPU::SLA_H  }, {"$CB25 SLA L"   ,2,8,&CPU::SLA_L  }, {"$CB26 SLA (HL)"   ,2,16,&CPU::SLA_aHL  }, {"$CB27 SLA A"   ,2,8,&CPU::SLA_A  }, {"$CB28 SRA B"   ,2,8,&CPU::SRA_B  }, {"$CB29 SRA C"   ,2,8,&CPU::SRA_C  }, {"$CB2A SRA D"   ,2,8,&CPU::SRA_D  }, {"$CB2B SRA E"   ,2,8,&CPU::SRA_E  }, {"$CB2C SRA H"   ,2,8,&CPU::SRA_H  }, {"$CB2D SRA L"   ,2,8,&CPU::SRA_L  }, {"$CB2E SRA (HL)"   ,2,16,&CPU::SRA_aHL  }, {"$CB2F SRA A"   ,2,8,&CPU::SRA_A  },
		{"$CB30 SWAP B"  ,2,8,&CPU::SWAP_B }, {"$CB31 SWAP C"  ,2,8,&CPU::SWAP_C }, {"$CB32 SWAP D"  ,2,8,&CPU::SWAP_D }, {"$CB33 SWAP E"  ,2,8,&CPU::SWAP_E }, {"$CB34 SWAP H"  ,2,8,&CPU::SWAP_H }, {"$CB35 SWAP L"  ,2,8,&CPU::SWAP_L }, {"$CB36 SWAP (HL)"  ,2,16,&CPU::SWAP_aHL }, {"$CB37 SWAP A"  ,2,8,&CPU::SWAP_A }, {"$CB38 SRL B"   ,2,8,&CPU::SRL_B  }, {"$CB39 SRL C"   ,2,8,&CPU::SRL_C  }, {"$CB3A SRL D"   ,2,8,&CPU::SRL_D  }, {"$CB3B SRL E"   ,2,8,&CPU::SRL_E  }, {"$CB3C SRL H"   ,2,8,&CPU::SRL_H  }, {"$CB3D SRL L"   ,2,8,&CPU::SRL_L  }, {"$CB3E SRL (HL)"   ,2,16,&CPU::SRL_aHL  }, {"$CB3F SRL A"   ,2,8,&CPU::SRL_A  },
		{"$CB40 BIT 0, B",2,8,&CPU::BIT_0_B}, {"$CB41 BIT 0, C",2,8,&CPU::BIT_0_C}, {"$CB42 BIT 0, D",2,8,&CPU::BIT_0_D}, {"$CB43 BIT 0, E",2,8,&CPU::BIT_0_E}, {"$CB44 BIT 0, H",2,8,&CPU::BIT_0_H}, {"$CB45 BIT 0, L",2,8,&CPU::BIT_0_L}, {"$CB46 BIT 0, (HL)",2,12,&CPU::BIT_0_aHL}, {"$CB47 BIT 0, A",2,8,&CPU::BIT_0_A}, {"$CB48 BIT 1, B",2,8,&CPU::BIT_1_B}, {"$CB49 BIT 1, C",2,8,&CPU::BIT_1_C}, {"$CB4A BIT 1, D",2,8,&CPU::BIT_1_D}, {"$CB4B BIT 1, E",2,8,&CPU::BIT_1_E}, {"$CB4C BIT 1, H",2,8,&CPU::BIT_1_H}, {"$CB4D BIT 1, L",2,8,&CPU::BIT_1_L}, {"$CB4E BIT 1, (HL)",2,12,&CPU::BIT_1_aHL}, {"$CB4F BIT 1, A",2,8,&CPU::BIT_1_A},
		{"$CB50 BIT 2, B",2,8,&CPU::BIT_2_B}, {"$CB51 BIT 2, C",2,8,&CPU::BIT_2_C}, {"$CB52 BIT 2, D",2,8,&CPU::BIT_2_D}, {"$CB53 BIT 2, E",2,8,&CPU::BIT_2_E}, {"$CB54 BIT 2, H",2,8,&CPU::BIT_2_H}, {"$CB55 BIT 2, L",2,8,&CPU::BIT_2_L}, {"$CB56 BIT 2, (HL)",2,12,&CPU::BIT_2_aHL}, {"$CB57 BIT 2, A",2,8,&CPU::BIT_2_A}, {"$CB58 BIT 3, B",2,8,&CPU::BIT_3_B}, {"$CB59 BIT 3, C",2,8,&CPU::BIT_3_C}, {"$CB5A BIT 3, D",2,8,&CPU::BIT_3_D}, {"$CB5B BIT 3, E",2,8,&CPU::BIT_3_E}, {"$CB5C BIT 3, H",2,8,&CPU::BIT_3_H}, {"$CB5D BIT 3, L",2,8,&CPU::BIT_3_L}, {"$CB5E BIT 3, (HL)",2,12,&CPU::BIT_3_aHL}, {"$CB5F BIT 3, A",2,8,&CPU::BIT_3_A},
		{"$CB60 BIT 4, B",2,8,&CPU::BIT_4_B}, {"$CB61 BIT 4, C",2,8,&CPU::BIT_4_C}, {"$CB62 BIT 4, D",2,8,&CPU::BIT_4_D}, {"$CB63 BIT 4, E",2,8,&CPU::BIT_4_E}, {"$CB64 BIT 4, H",2,8,&CPU::BIT_4_H}, {"$CB65 BIT 4, L",2,8,&CPU::BIT_4_L}, {"$CB66 BIT 4, (HL)",2,12,&CPU::BIT_4_aHL}, {"$CB67 BIT 4, A",2,8,&CPU::BIT_4_A}, {"$CB68 BIT 5, B",2,8,&CPU::BIT_5_B}, {"$CB69 BIT 5, C",2,8,&CPU::BIT_5_C}, {"$CB6A BIT 5, D",2,8,&CPU::BIT_5_D}, {"$CB6B BIT 5, E",2,8,&CPU::BIT_5_E}, {"$CB6C BIT 5, H",2,8,&CPU::BIT_5_H}, {"$CB6D BIT 5, L",2,8,&CPU::BIT_5_L}, {"$CB6E BIT 5, (HL)",2,12,&CPU::BIT_5_aHL}, {"$CB6F BIT 5, A",2,8,&CPU::BIT_5_A},
		{"$CB70 BIT 6, B",2,8,&CPU::BIT_6_B}, {"$CB71 BIT 6, C",2,8,&CPU::BIT_6_C}, {"$CB72 BIT 6, D",2,8,&CPU::BIT_6_D}, {"$CB73 BIT 6, E",2,8,&CPU::BIT_6_E}, {"$CB74 BIT 6, H",2,8,&CPU::BIT_6_H}, {"$CB75 BIT 6, L",2,8,&CPU::BIT_6_L}, {"$CB76 BIT 6, (HL)",2,12,&CPU::BIT_6_aHL}, {"$CB77 BIT 6, A",2,8,&CPU::BIT_6_A}, {"$CB78 BIT 7, B",2,8,&CPU::BIT_7_B}, {"$CB79 BIT 7, C",2,8,&CPU::BIT_7_C}, {"$CB7A BIT 7, D",2,8,&CPU::BIT_7_D}, {"$CB7B BIT 7, E",2,8,&CPU::BIT_7_E}, {"$CB7C BIT 7, H",2,8,&CPU::BIT_7_H}, {"$CB7D BIT 7, L",2,8,&CPU::BIT_7_L}, {"$CB7E BIT 7, (HL)",2,12,&CPU::BIT_7_aHL}, {"$CB7F BIT 7, A",2,8,&CPU::BIT_7_A},
		{"$CB80 RES 0, B",2,8,&CPU::RES_0_B}, {"$CB81 RES 0, C",2,8,&CPU::RES_0_C}, {"$CB82 RES 0, D",2,8,&CPU::RES_0_D}, {"$CB83 RES 0, E",2,8,&CPU::RES_0_E}, {"$CB84 RES 0, H",2,8,&CPU::RES_0_H}, {"$CB85 RES 0, L",2,8,&CPU::RES_0_L}, {"$CB86 RES 0, (HL)",2,16,&CPU::RES_0_aHL}, {"$CB87 RES 0, A",2,8,&CPU::RES_0_A}, {"$CB88 RES 1, B",2,8,&CPU::RES_1_B}, {"$CB89 RES 1, C",2,8,&CPU::RES_1_C}, {"$CB8A RES 1, D",2,8,&CPU::RES_1_D}, {"$CB8B RES 1, E",2,8,&CPU::RES_1_E}, {"$CB8C RES 1, H",2,8,&CPU::RES_1_H}, {"$CB8D RES 1, L",2,8,&CPU::RES_1_L}, {"$CB8E RES 1, (HL)",2,16,&CPU::RES_1_aHL}, {"$CB8F RES 1, A",2,8,&CPU::RES_1_A},
		{"$CB90 RES 2, B",2,8,&CPU::RES_2_B}, {"$CB91 RES 2, C",2,8,&CPU::RES_2_C}, {"$CB92 RES 2, D",2,8,&CPU::RES_2_D}, {"$CB93 RES 2, E",2,8,&CPU::RES_2_E}, {"$CB94 RES 2, H",2,8,&CPU::RES_2_H}, {"$CB95 RES 2, L",2,8,&CPU::RES_2_L}, {"$CB96 RES 2, (HL)",2,16,&CPU::RES_2_aHL}, {"$CB97 RES 2, A",2,8,&CPU::RES_2_A}, {"$CB98 RES 3, B",2,8,&CPU::RES_3_B}, {"$CB99 RES 3, C",2,8,&CPU::RES_3_C}, {"$CB9A RES 3, D",2,8,&CPU::RES_3_D}, {"$CB9B RES 3, E",2,8,&CPU::RES_3_E}, {"$CB9C RES 3, H",2,8,&CPU::RES_3_H}, {"$CB9D RES 3, L",2,8,&CPU::RES_3_L}, {"$CB9E RES 3, (HL)",2,16,&CPU::RES_3_aHL}, {"$CB9F RES 3, A",2,8,&CPU::RES_3_A},
		{"$CBA0 RES 4, B",2,8,&CPU::RES_4_B}, {"$CBA1 RES 4, C",2,8,&CPU::RES_4_C}, {"$CBA2 RES 4, D",2,8,&CPU::RES_4_D}, {"$CBA3 RES 4, E",2,8,&CPU::RES_4_E}, {"$CBA4 RES 4, H",2,8,&CPU::RES_4_H}, {"$CBA5 RES 4, L",2,8,&CPU::RES_4_L}, {"$CBA6 RES 4, (HL)",2,16,&CPU::RES_4_aHL}, {"$CBA7 RES 4, A",2,8,&CPU::RES_4_A}, {"$CBA8 RES 5, B",2,8,&CPU::RES_5_B}, {"$CBA9 RES 5, C",2,8,&CPU::RES_5_C}, {"$CBAA RES 5, D",2,8,&CPU::RES_5_D}, {"$CBAB RES 5, E",2,8,&CPU::RES_5_E}, {"$CBAC RES 5, H",2,8,&CPU::RES_5_H}, {"$CBAD RES 5, L",2,8,&CPU::RES_5_L}, {"$CBAE RES 5, (HL)",2,16,&CPU::RES_5_aHL}, {"$CBAF RES 5, A",2,8,&CPU::RES_5_A},
		{"$CBB0 RES 6, B",2,8,&CPU::RES_6_B}, {"$CBB1 RES 6, C",2,8,&CPU::RES_6_C}, {"$CBB2 RES 6, D",2,8,&CPU::RES_6_D}, {"$CBB3 RES 6, E",2,8,&CPU::RES_6_E}, {"$CBB4 RES 6, H",2,8,&CPU::RES_6_H}, {"$CBB5 RES 6, L",2,8,&CPU::RES_6_L}, {"$CBB6 RES 6, (HL)",2,16,&CPU::RES_6_aHL}, {"$CBB7 RES 6, A",2,8,&CPU::RES_6_A}, {"$CBB8 RES 7, B",2,8,&CPU::RES_7_B}, {"$CBB9 RES 7, C",2,8,&CPU::RES_7_C}, {"$CBBA RES 7, D",2,8,&CPU::RES_7_D}, {"$CBBB RES 7, E",2,8,&CPU::RES_7_E}, {"$CBBC RES 7, H",2,8,&CPU::RES_7_H}, {"$CBBD RES 7, L",2,8,&CPU::RES_7_L}, {"$CBBE RES 7, (HL)",2,16,&CPU::RES_7_aHL}, {"$CBBF RES 7, A",2,8,&CPU::RES_7_A},
		{"$CBC0 SET 0, B",2,8,&CPU::SET_0_B}, {"$CBC1 SET 0, C",2,8,&CPU::SET_0_C}, {"$CBC2 SET 0, D",2,8,&CPU::SET_0_D}, {"$CBC3 SET 0, E",2,8,&CPU::SET_0_E}, {"$CBC4 SET 0, H",2,8,&CPU::SET_0_H}, {"$CBC5 SET 0, L",2,8,&CPU::SET_0_L}, {"$CBC6 SET 0, (HL)",2,16,&CPU::SET_0_aHL}, {"$CBC7 SET 0, A",2,8,&CPU::SET_0_A}, {"$CBC8 SET 1, B",2,8,&CPU::SET_1_B}, {"$CBC9 SET 1, C",2,8,&CPU::SET_1_C}, {"$CBCA SET 1, D",2,8,&CPU::SET_1_D}, {"$CBCB SET 1, E",2,8,&CPU::SET_1_E}, {"$CBCC SET 1, H",2,8,&CPU::SET_1_H}, {"$CBCD SET 1, L",2,8,&CPU::SET_1_L}, {"$CBCE SET 1, (HL)",2,16,&CPU::SET_1_aHL}, {"$CBCF SET 1, A",2,8,&CPU::SET_1_A},
		{"$CBD0 SET 2, B",2,8,&CPU::SET_2_B}, {"$CBD1 SET 2, C",2,8,&CPU::SET_2_C}, {"$CBD2 SET 2, D",2,8,&CPU::SET_2_D}, {"$CBD3 SET 2, E",2,8,&CPU::SET_2_E}, {"$CBD4 SET 2, H",2,8,&CPU::SET_2_H}, {"$CBD5 SET 2, L",2,8,&CPU::SET_2_L}, {"$CBD6 SET 2, (HL)",2,16,&CPU::SET_2_aHL}, {"$CBD7 SET 2, A",2,8,&CPU::SET_2_A}, {"$CBD8 SET 3, B",2,8,&CPU::SET_3_B}, {"$CBD9 SET 3, C",2,8,&CPU::SET_3_C}, {"$CBDA SET 3, D",2,8,&CPU::SET_3_D}, {"$CBDB SET 3, E",2,8,&CPU::SET_3_E}, {"$CBDC SET 3, H",2,8,&CPU::SET_3_H}, {"$CBDD SET 3, L",2,8,&CPU::SET_3_L}, {"$CBDE SET 3, (HL)",2,16,&CPU::SET_3_aHL}, {"$CBDF SET 3, A",2,8,&CPU::SET_3_A},
		{"$CBE0 SET 4, B",2,8,&CPU::SET_4_B}, {"$CBE1 SET 4, C",2,8,&CPU::SET_4_C}, {"$CBE2 SET 4, D",2,8,&CPU::SET_4_D}, {"$CBE3 SET 4, E",2,8,&CPU::SET_4_E}, {"$CBE4 SET 4, H",2,8,&CPU::SET_4_H}, {"$CBE5 SET 4, L",2,8,&CPU::SET_4_L}, {"$CBE6 SET 4, (HL)",2,16,&CPU::SET_4_aHL}, {"$CBE7 SET 4, A",2,8,&CPU::SET_4_A}, {"$CBE8 SET 5, B",2,8,&CPU::SET_5_B}, {"$CBE9 SET 5, C",2,8,&CPU::SET_5_C}, {"$CBEA SET 5, D",2,8,&CPU::SET_5_D}, {"$CBEB SET 5, E",2,8,&CPU::SET_5_E}, {"$CBEC SET 5, H",2,8,&CPU::SET_5_H}, {"$CBED SET 5, L",2,8,&CPU::SET_5_L}, {"$CBEE SET 5, (HL)",2,16,&CPU::SET_5_aHL}, {"$CBEF SET 5, A",2,8,&CPU::SET_5_A},
		{"$CBF0 SET 6, B",2,8,&CPU::SET_6_B}, {"$CBF1 SET 6, C",2,8,&CPU::SET_6_C}, {"$CBF2 SET 6, D",2,8,&CPU::SET_6_D}, {"$CBF3 SET 6, E",2,8,&CPU::SET_6_E}, {"$CBF4 SET 6, H",2,8,&CPU::SET_6_H}, {"$CBF5 SET 6, L",2,8,&CPU::SET_6_L}, {"$CBF6 SET 6, (HL)",2,16,&CPU::SET_6_aHL}, {"$CBF7 SET 6, A",2,8,&CPU::SET_6_A}, {"$CBF8 SET 7, B",2,8,&CPU::SET_7_B}, {"$CBF9 SET 7, C",2,8,&CPU::SET_7_C}, {"$CBFA SET 7, D",2,8,&CPU::SET_7_D}, {"$CBFB SET 7, E",2,8,&CPU::SET_7_E}, {"$CBFC SET 7, H",2,8,&CPU::SET_7_H}, {"$CBFD SET 7, L",2,8,&CPU::SET_7_L}, {"$CBFE SET 7, (HL)",2,16,&CPU::SET_7_aHL}, {"$CBFF SET 7, A",2,8,&CPU::SET_7_A},
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