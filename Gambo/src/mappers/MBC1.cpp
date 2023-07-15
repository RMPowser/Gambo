#include "MBC1.h"
#include "Cartridge.h"

MBC1::MBC1(Cartridge* cart)
	: BaseMapper(cart)
	, ramEnabled(0)
	, romBankNumber(0)
	, ramBankNumber(0)
	, bankingModeSelect(0)
	, bankNumberBitWidth(0)
{
	int numberOfBanks = cart->GetRomBanks();
	while (numberOfBanks > 1)
	{
		numberOfBanks >>= 1;
		bankNumberBitWidth++;

		// max width is 5 bits
		if (bankNumberBitWidth == 5)
			break;
	}
}

MBC1::~MBC1()
{
}

u8 MBC1::Read(u16 addr)
{
	// mbc1 supports up to 2mb rom and/or 32kb ram so we actually only need 21 bits in this u32.
	u32 wAddr = 0;

	if (0x0000 <= addr && addr <= 0x3FFF)
	{
		if (bankingModeSelect == 0)
		{
			// bits 0-13 come from gameboy address.
			// other 7 bits are always 0;
			wAddr = addr & 0x3FFF;
		}
		else
		{
			// bits 0-13 come from gameboy address.
			wAddr = addr & 0x3FFF;

			// bits 14-18 are always 0 and bits 19-20
			// come from ramBankNumber
			wAddr |= ramBankNumber << 18;
		}
	}
	else if (0x4000 <= addr && addr <= 0x7FFF)
	{
		// bits 0-13 come from gameboy address.
		wAddr = addr & 0x3FFF;

		// bits 14-18 are from the rom bank number
		wAddr |= romBankNumber << 13;

		// bits 19-20 come from ramBankNumber
		wAddr |= ramBankNumber << 18;
	}
	else if (0xA000 <= addr && addr <= 0xBFFF)
	{
		if (ramEnabled)
		{
			if (bankingModeSelect == 0)
			{
				// bits 0-12 come from gameboy address.
				// bits 13-14 are always 0;
				wAddr = addr & 0x1FFF;
			}
			else
			{
				// bits 0-12 come from gameboy address.
				wAddr = addr & 0x1FFF;

				// bits 13-14 come from ramBankNumber
				wAddr |= ramBankNumber << 12;
			}

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

	return cart->rom[wAddr];
}

void MBC1::Write(u16 addr, u8 data)
{
	if (0x0000 <= addr && addr <= 0x1FFF)
	{
		// writing exactly 0xA in the bottom nybble enables ram. anything else
		// disables ram.
		ramEnabled = (data & 0x0A) == 0x0A;
	}
	else if (0x2000 <= addr && addr <= 0x3FFF)
	{
		// figure out our bit mask
		int bitMask = 0;
		for (size_t i = 0; i < bankNumberBitWidth; i++)
		{
			bitMask <<= 1;
			bitMask += 1;
		}

		// if the bottom 5 bits of data is 0, set rom bank number to 1, otherwise
		// rom bank number only uses the bits it needs according to the rom size.
		// this makes it possible to map bank 0 if the bit width is less than 5,  
		// meaning the rom is 256k or smaller. You can do this by setting any bits 
		// above the bit width to 1, causing the comparison to fail, and bank 0 to 
		// be mapped. example: the bit width is 3 and you write 0b01000. 
		romBankNumber = (data & 0b11111) == 0 ? 1 : data & bitMask;
	}
	else if (0x4000 <= addr && addr <= 0x5FFF)
	{
		// ram bank number is always only 2 bits
		ramBankNumber = data & 0b11;
	}
	else if (0x6000 <= addr && addr <= 0x7FFF)
	{
		// banking mode select is a 1 bit register
		bankingModeSelect = data & 0b1;
	}
	else if (0xA000 <= addr && addr <= 0xBFFF)
	{
		// writing to cartridge ram
		if (ramEnabled)
		{
			u16 wAddr = 0;
			if (bankingModeSelect == 0)
			{
				// bits 0-12 come from gameboy address.
				// bits 13-14 are always 0;
				wAddr = addr & 0x1FFF;
			}
			else
			{
				// bits 0-12 come from gameboy address.
				wAddr = addr & 0x1FFF;

				// bits 13-14 come from ramBankNumber
				wAddr |= ramBankNumber << 12;
			}

			cart->ram[wAddr] = data;
		}
		else
		{
			// writes are ignored if ram is disabled
		}
	}
}