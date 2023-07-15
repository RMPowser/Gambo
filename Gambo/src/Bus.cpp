#include "Bus.h"

Bus::Bus()
	: cpu(this)
	, ppu(this)
	, cart(nullptr)
{
}

Bus::~Bus()
{
	SAFE_DELETE(cart);
}

u8 Bus::Read(u16 addr)
{
	lastRead = addr;

	// boot rom
	if (IsBootRomAddress(addr))
	{
		return cpu.bootRom[addr];
	}
	// cartridge
	else if (IsCartridgeAddress(addr))
	{
		if (cart != nullptr)
		{
			return cart->Read(addr);
		}
		else
		{
			return cpu.useBootRom ? 0xFF : ram[addr];
		}
	}
	// normal ram
	else
	{
		return ram[addr];
	}
}

void Bus::Write(u16 addr, u8 data)
{
	// TODO: remove this after implementing input
	if (addr == HWAddr::P1)
	{
		return;
	}

	lastWrite = addr;

	// cartridge
	if (IsCartridgeAddress(addr) && cart != nullptr)
	{
		cart->Write(addr, data);
	}
	// normal ram
	else
	{
		ram[addr] = data;

		// this is the implementation for echo ram
		if (addr >= 0xE000 && addr <= 0xFDFF)
		{
			ram[addr - 0x2000] = data;
		}
		else if (addr >= 0xC000 && addr <= 0xDDFF)
		{
			ram[addr + 0x2000] = data;
		}

		// top 3 bits in IF are always read as 1
		if (addr == HWAddr::IF)
		{
			ram[HWAddr::IF] |= 0b11100000;
		}

		// writing anything to the DIV register resets it to 0
		if (addr == HWAddr::DIV)
		{
			ram[addr] = 0;
		}
	}
}

void Bus::Reset()
{
	ram = std::vector<u8>(64KiB, 0x00);
	mapAsm.clear();
	lastWrite = 0;
	lastRead = 0;
	cpu.Reset();
	ppu.Reset();
	SAFE_DELETE(cart);
}

void Bus::InsertCartridge(std::wstring filePath)
{
	SAFE_DELETE(cart);
	cart = new Cartridge(filePath);

	// copy the first 32KiB of the cartridge data into ram
	for (u16 i = 0; i < 32KiB; i++)
	{
		ram[i] = cart->Read(i);
	}
}

void Bus::Disassemble(u16 startAddr, int numInstr)
{
	u32 addr = startAddr;
	u8 value = 0x00, lo = 0x00, hi = 0x00;
	u16 lineAddr = 0;

	mapAsm.clear();

	while (mapAsm.size() < numInstr)
	{

		//if ((0x4000 <= addr && addr <= 0xBFFF) || // skip vram
		//	(0x0104 <= addr && addr <= 0x014F) || // skip cartridge header
		//	(0xFE00 <= addr && addr <= 0xFE7F) || // skip OAM and IO
		//	(addr == 0xD800)) // skip this address in particular because if i dont, it breaks disassembly
		//{
		//	addr++;
		//	continue;
		//}

		lineAddr = addr;

		// prefix line with instruction addr
		std::string s = "$" + hex(addr, 4) + ": ";

		// read instruction and get readable name
		u8 opcode = Read(addr++);
		if (opcode == 0xCB)
		{
			// its a 16bit opcode so read another byte
			opcode = Read(addr++);

			s += cpu.instructions16bit[opcode].mnemonic;
		}
		else
		{
			auto& instruction = cpu.instructions8bit[opcode];
			switch (instruction.bytes)
			{
				case 0:
				case 1:
				{
					s += cpu.instructions8bit[opcode].mnemonic;
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
}

bool Bus::IsBootRomAddress(u16 addr)
{
	return
		!(ram[HWAddr::BOOT] & 1) && 
		(0x0000 <= addr && addr <= 0x00FF);
}

bool Bus::IsCartridgeAddress(u16 addr)
{
	return
		!IsBootRomAddress(addr) &&
		(0x0000 <= addr && addr <= 0x7FFF) ||	// rom
		(0xA000 <= addr && addr <= 0xBFFF);		// ram
}
