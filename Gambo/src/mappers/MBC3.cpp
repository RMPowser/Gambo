#include "MBC3.h"
#include "Cartridge.h"

using namespace std;

MBC3::MBC3(Cartridge* cart)
	: BaseMapper(cart)
	, ramAndRTCEnabled(0)
	, romBankNumber(0)
	, ramBankNumber(0)
	, mappedRTCRegister(MappedRegister::NONE)
	, rtcS(0)
	, rtcM(0)
	, rtcH(0)
	, rtcDL(0)
	, rtcDH(0)
	, isHalted(false)
	, wasZeroWritten(0)
	, isLatched(false)
{
}

MBC3::~MBC3()
{
}

u8 MBC3::Read(u16 addr)
{
	// mbc3 supports up to 2mb rom and/or 32kb ram + RTC, so we actually only need 21 bits in this u32.
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
		if (ramAndRTCEnabled)
		{
			// update rtc registers if not latched or halted.
			if (!isLatched && !isHalted)
			{
				// Get the current time
				auto now = chrono::system_clock::now();

				// Convert to time_t to get the number of seconds since epoch
				time_t currentTime = chrono::system_clock::to_time_t(now);

				// Convert to tm structure for local time
				tm* localTime = localtime(&currentTime);

				// tm_sec includes leap seconds. ie: 0-60 instead of 0-59. need to account for that.
				rtcS = localTime->tm_sec == 60 ? 59 : localTime->tm_sec;
				rtcM = localTime->tm_min;
				rtcH = localTime->tm_hour;
				rtcDL = localTime->tm_yday & 0xFF;
				rtcDH = localTime->tm_yday & 0x100;
			}

			switch (mappedRTCRegister)
			{
				case MBC3::MappedRegister::NONE:
				{
					// bits 0-14 come from gameboy address.
					wAddr = addr & 0x1FFF;
					return cart->ram[wAddr];
				}
				case MBC3::MappedRegister::RTC_S:
				{
					return rtcS;
				}
				case MBC3::MappedRegister::RTC_M:
				{
					return rtcM;
				}
				case MBC3::MappedRegister::RTC_H:
				{
					return rtcH;
				}
				case MBC3::MappedRegister::RTC_DL:
				{
					return rtcDL;
				}
				case MBC3::MappedRegister::RTC_DH:
				{
					return rtcDH;
				}
			}
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

void MBC3::Write(u16 addr, u8 data)
{
	if (0x0000 <= addr && addr <= 0x1FFF)
	{
		// writing exactly 0xA in the bottom nybble enables ram. anything else
		// disables ram.
		if (cart->GetRamSize() > 0)
			ramAndRTCEnabled = (data & 0x0A) == 0x0A;
		else
			ramAndRTCEnabled = false;
	}
	else if (0x2000 <= addr && addr <= 0x3FFF)
	{
		// Same as for MBC1, except that the whole 7 bits of the ROM Bank Number are written instead of only 5 bits.
		
		// writing 0x00 will select bank 0x01 instead.
		if (data == 0x00)
		{
			romBankNumber = 1;
		}
		else
		{
			romBankNumber = (data & 0b1111111);
		}
	}
	else if (0x4000 <= addr && addr <= 0x5FFF)
	{
		// As for the MBC1s RAM Banking Mode, writing a value in range for 0x00-0x03 
		// maps the corresponding external RAM Bank(if any) into memory at 0xA000-0xBFFF.
		// When writing a value of 0x08-0x0C, this will map the corresponding RTC register
		// into memory at 0xA000-0xBFFF. That register could then be read/written by 
		// accessing any address in that area, typically that is done by using address 0xA000.
		
		if (0x00 <= data && data <= 0x03)
		{
			ramBankNumber = data;
		}
		else if (0x08 <= data && data <= 0x0C)
		{
			mappedRTCRegister = MappedRegister(data);
		}
	}
	else if (0x6000 <= addr && addr <= 0x7FFF)
	{
		if (data == 0x00)
		{
			wasZeroWritten = true;
		}
		if (data == 0x01 && wasZeroWritten)
		{
			wasZeroWritten = false;
			isLatched = !isLatched;
		}
	}
	else if (0xA000 <= addr && addr <= 0xBFFF)
	{
		u32 wAddr = 0;

		// writing to cartridge ram or rtc registers
		if (ramAndRTCEnabled)
		{

			switch (mappedRTCRegister)
			{
				case MBC3::MappedRegister::NONE:
				{
					// truncate to 8kb range and then offset by ram bank number times the size of a bank.
					wAddr = addr & 0x1FFF;
					wAddr += ramBankNumber * 8_KB;
					cart->ram[wAddr] = data;
					break;
				}
				case MBC3::MappedRegister::RTC_S:
				{
					rtcS = data;
					break;
				}
				case MBC3::MappedRegister::RTC_M:
				{
					rtcM = data;
					break;
				}
				case MBC3::MappedRegister::RTC_H:
				{
					rtcH = data;
					break;
				}
				case MBC3::MappedRegister::RTC_DL:
				{
					rtcDL = data;
					break;
				}
				case MBC3::MappedRegister::RTC_DH:
				{
					rtcDH = data;
					isHalted = GetBits(rtcDH, 6, 0b1);
					break;
				}
			}
		}
		else
		{
			// writes are ignored if ram is disabled
		}
	}
}