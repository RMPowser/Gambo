#include "Cartridge.h"
#include <iostream>
#include <fstream>

Cartridge::Cartridge(std::wstring filePath)
{
	std::ifstream input(filePath, std::ios::binary | std::ios::ate);
	if (input.is_open())
	{
		data.clear();

		std::streampos size = input.tellg();
		data.reserve(size);

		input.seekg(0, std::ios::beg);
		data.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		input.close();

		DeserializeHeader();
	}
}

Cartridge::~Cartridge()
{
}

const std::vector<u8>& Cartridge::GetData()
{
	return data;
}

void Cartridge::DeserializeHeader()
{
	for (size_t i = 0; i < header.title.size(); i++)
		header.title[i]					= data[i + 0x0134]; // 0x0134-0x0143
	
	for (size_t i = 0; i < header.manufacture_code.size(); i++)
		header.manufacture_code[i]		= data[i + 0x013F]; // 0x013F-0x0142
	
	header.cgb_flag						= data[0x0143];

	for (size_t i = 0; i < header.new_publisher_code.size(); i++)
		header.new_publisher_code[i]	= data[i + 0x0144]; // 0x0144-0x0145
	
	header.sgb_flag						= data[0x0143];
	header.type							= data[0x0147];
	header.rom_size						= data[0x0148];
	header.ram_size						= data[0x0149];
	header.region_code					= data[0x014A];
	header.old_publisher_code			= data[0x014B];
	header.rom_version_number			= data[0x014C];
	header.header_checksum				= data[0x014D];

	for (size_t i = 0; i < header.global_checksum.size(); i++)
		header.global_checksum[i]		= data[i + 0x014E]; // 0x014E-0x014F
}