#pragma once
#include "GamboDefine.h"


enum class MapperType
{
	ROM_ONLY = 0x00,
	MBC1 = 0x01,
	MBC1_RAM = 0x02,
	MBC1_RAM_BATTERY = 0x03,
	MBC2 = 0x05,
	MBC2_BATERRY = 0x06,
	ROM_RAM = 0x08, // No licensed cartridge makes use of this option. The exact behavior is unknown.
	ROM_RAM_BATTERY = 0x09, // No licensed cartridge makes use of this option. The exact behavior is unknown.
	MMM01 = 0x0B,
	MMM01_RAM = 0x0C,
	MMM01_RAM_BATTERY = 0x0D,
	MBC3_TIMER_BATTERY = 0x0F,
	MBC3_TIMER_RAM_BATTERY = 0x10, // MBC3 with 64 KiB of SRAM refers to MBC30, used only in Pocket Monsters : Crystal Version(the Japanese version of Pokémon Crystal Version).
	MBC3 = 0x11,
	MBC3_RAM = 0x12, // MBC3 with 64 KiB of SRAM refers to MBC30, used only in Pocket Monsters : Crystal Version(the Japanese version of Pokémon Crystal Version).
	MBC3_RAM_BATTERY = 0x13, // MBC3 with 64 KiB of SRAM refers to MBC30, used only in Pocket Monsters : Crystal Version(the Japanese version of Pokémon Crystal Version).
	MBC5 = 0x19,
	MBC5_RAM = 0x1A,
	MBC5_RAM_BATTERY = 0x1B,
	MBC5_RUMBLE = 0x1C,
	MBC5_RUMBLE_RAM = 0x1D,
	MBC5_RUMBLE_RAM_BATTERY = 0x1E,
	MBC6 = 0x20,
	MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
	POCKET_CAMERA = 0xFC,
	BANDAI_TAMA5 = 0xFD,
	HuC3 = 0xFE,
	HuC1_RAM_BATTERY = 0xFF,
};

class BaseMapper;
class MBC1;

class Cartridge
{
	friend class MBC1;

public:
	Cartridge(std::wstring filePath);
	~Cartridge();

	Cartridge() = delete;
	
	u8			Read(u16 addr) const;
	void		Write(u16 addr, u8 data);

	std::string GetTitle() const;
	std::string	GetManufacturerCode() const;
	u8			GetCGBFlag() const;
	std::string	GetPublisher() const;
	u8			GetSGBFlag() const;
	MapperType	GetMapperType() const;
	u64			GetRomSize() const;
	u16			GetRomBanks() const;
	u64			GetRamSize() const;
	u8			GetRamBanks() const;
	u8			GetRegionCode() const;
	u8			GetRomVersionNumber() const;
	u8			GetHeaderChecksum() const;
	u16			GetGlobalChecksum() const;

	const BaseMapper* GetMapper() const;

private:
	void DeserializeHeader();
	void InitializeMapper();

	
	BaseMapper* mapper;
	std::vector<u8> rom;
	std::vector<u8> ram;

	struct
	{
		std::array<u8, 16>		title;				// 0x0134-0x0143
		std::array<u8, 4>		manufacturer_code;	// 0x013F-0x0142
		u8						cgb_flag;			// 0x0143
		std::array<u8, 2>		new_publisher_code;	// 0x0144-0x0145
		u8						sgb_flag;			// 0x0146
		MapperType				type;				// 0x0147
		u8						rom_size;			// 0x0148
		u8						ram_size;			// 0x0149
		u8						region_code;		// 0x014A
		u8						old_publisher_code;	// 0x014B
		u8						rom_version_number;	// 0x014C
		u8						header_checksum;	// 0x014D
		std::array<u8, 2>		global_checksum;	// 0x014E-0x014F
	} header;
};

