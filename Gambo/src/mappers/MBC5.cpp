#include "MBC5.h"
#include "Cartridge.h"

using namespace std;

MBC5::MBC5(Cartridge* cart)
	: BaseMapper(cart)
	, ramEnabled(false)
	, romBankNumber(0)
	, ramBankNumber(0)
{
}

MBC5::~MBC5()
{
}

u8 MBC5::Read(u16 addr)
{
	u32 wAddr = 0;

	if (0x0000 <= addr && addr <= 0x3FFF) // just the first 16kb of the rom, nothing fancy.
	{
		wAddr = addr;
		return cart->rom[wAddr];
	}
	else if (0x4000 <= addr && addr <= 0x7FFF)
	{
		// bits 0-13 come from gameboy address.
		wAddr = addr & 0x3FFF;

		// bits 14-18 are from the rom bank number
		wAddr |= romBankNumber << 14;

		return cart->rom[wAddr];
	}
	else if (0xA000 <= addr && addr <= 0xBFFF)
	{
		if (ramEnabled)
		{
			// truncate to 8kb range and then offset by ram bank number times the size of a bank.
			wAddr = addr & 0x1FFF;
			wAddr += ramBankNumber * 8KiB;

			return cart->ram[wAddr];
		}
		else
		{
			// if ram is disabled, reads return open bus values,
			// often 0xFF, but not guaranteed, but who cares. for
			// now, always 0xFF
			return 0xFF;
		}
	}

	throw;
}

void MBC5::Write(u16 addr, u8 data)
{
	if (0x0000 <= addr && addr <= 0x1FFF)
	{
		// writing exactly 0xA in the bottom nybble enables ram. anything else
		// disables ram.
		if (cart->GetRamSize() > 0)
			ramEnabled = (data & 0x0A) == 0x0A;
		else
			ramEnabled = false;
	}
	else if (0x2000 <= addr && addr <= 0x2FFF)
	{
		// 8 least significant bits of the ROM bank number.
		romBankNumber &= ~(0xFF);
		romBankNumber |= data;
	}
	else if (0x3000 <= addr && addr <= 0x3FFF)
	{
		// 9th bit of ROM bank number.
		romBankNumber &= ~(1 << 8);
		romBankNumber |= u16(data) << 8;
	}
	else if (0x4000 <= addr && addr <= 0x5FFF)
	{
		// ram bank number from 0x00-0x0F.
		ramBankNumber = data & 0x0F;
	}
	else if (0xA000 <= addr && addr <= 0xBFFF)
	{
		u32 wAddr = 0;

		// writing to cartridge ram
		if (ramEnabled)
		{
			// bits 0-12 come from gameboy address.
			wAddr = addr & 0x1FFF;

			// bits 13-14 come from ramBankNumber
			wAddr |= ramBankNumber << 12;

			cart->ram[wAddr] = data;
		}
		else
		{
			// writes are ignored if ram is disabled
		}
	}
}