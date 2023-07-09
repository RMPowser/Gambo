#pragma once
#include "GamboDefine.h"

class Cartridge
{
public:
	Cartridge(std::wstring filePath);
	~Cartridge();

	std::vector<u8> data;

private:
	void DeserializeHeader();

	struct CartridgeHeader
	{
		std::array<u8, 16>		title;				// 0x0134-0x0143
		std::array<u8, 4>		manufacture_code;	// 0x013F-0x0142
		u8						cgb_flag;			// 0x0143
		std::array<u8, 2>		new_publisher_code;	// 0x0144-0x0145
		u8						sgb_flag;			// 0x0146
		u8						type;				// 0x0147
		u8						rom_size;			// 0x0148
		u8						ram_size;			// 0x0149
		u8						region_code;		// 0x014A
		u8						old_publisher_code;	// 0x014B
		u8						rom_version_number;	// 0x014C
		u8						header_checksum;	// 0x014D
		std::array<u8, 2>		global_checksum;	// 0x014E-0x014F
	} header;
	
};

