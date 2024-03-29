#include "Cartridge.h"
#include "BaseMapper.h"
#include "MBC1.h"
#include "MBC3.h"
#include <iostream>
#include <fstream>
#include <sstream>


std::map<MapperType, std::string> MapperTypeToString {
	{ MapperType::ROM_ONLY, "ROM_ONLY" },
	{ MapperType::MBC1, "MBC1" },
	{ MapperType::MBC1_RAM, "MBC1_RAM" },
	{ MapperType::MBC1_RAM_BATTERY, "MBC1_RAM_BATTERY" },
	{ MapperType::MBC2, "MBC2" },
	{ MapperType::MBC2_BATERRY, "MBC2_BATERRY" },
	{ MapperType::ROM_RAM, "ROM_RAM" },
	{ MapperType::ROM_RAM_BATTERY, "ROM_RAM_BATTERY" },
	{ MapperType::MMM01, "MMM01" },
	{ MapperType::MMM01_RAM, "MMM01_RAM" },
	{ MapperType::MMM01_RAM_BATTERY, "MMM01_RAM_BATTERY" },
	{ MapperType::MBC3_TIMER_BATTERY, "MBC3_TIMER_BATTERY" },
	{ MapperType::MBC3_TIMER_RAM_BATTERY, "MBC3_TIMER_RAM_BATTERY" },
	{ MapperType::MBC3, "MBC3" },
	{ MapperType::MBC3_RAM, "MBC3_RAM" },
	{ MapperType::MBC3_RAM_BATTERY, "MBC3_RAM_BATTERY" },
	{ MapperType::MBC5, "MBC5" },
	{ MapperType::MBC5_RAM, "MBC5_RAM" },
	{ MapperType::MBC5_RAM_BATTERY, "MBC5_RAM_BATTERY" },
	{ MapperType::MBC5_RUMBLE, "MBC5_RUMBLE" },
	{ MapperType::MBC5_RUMBLE_RAM, "MBC5_RUMBLE_RAM" },
	{ MapperType::MBC5_RUMBLE_RAM_BATTERY, "MBC5_RUMBLE_RAM_BATTERY" },
	{ MapperType::MBC6, "MBC6" },
	{ MapperType::MBC7_SENSOR_RUMBLE_RAM_BATTERY, "MBC7_SENSOR_RUMBLE_RAM_BATTERY" },
	{ MapperType::POCKET_CAMERA, "POCKET_CAMERA" },
	{ MapperType::BANDAI_TAMA5, "BANDAI_TAMA5" },
	{ MapperType::HuC3, "HuC3" },
	{ MapperType::HuC1_RAM_BATTERY, "HuC1_RAM_BATTERY" },
};

struct ROM_info
{
	u64 size; // size in bytes
	u16 banks; // number of 16KiB banks
};

const ROM_info rom_info[] =
{
	{	 32KiB,   2 },
	{	 64KiB,   4 },
	{	128KiB,   8 },
	{	256KiB,  16 },
	{	512KiB,  32 },
	{	  1MiB,  64 },
	{	  2MiB, 128 },
	{	  4MiB, 256 },
	{	  8MiB, 512 },
};


struct RAM_info
{
	u64 size; // size in bytes
	u8 banks; // number of 8KiB banks
};

const RAM_info ram_info[] =
{
	{		 0,  0 },
	{		 0,  0 },
	{	  8KiB,  1 },
	{	 32KiB,  4 },
	{	128KiB, 16 },
	{	 64KiB,  8 },

	// Index 2 in this list is listed in various unofficial docs as 2 KiB
	// in size. However, a 2 KiB RAM chip was never used in a cartridge. 
	// The source of this value is unknown. I will treat it as 0.
};


const std::map<u8, std::string> old_publisher_info =
{
	{ 0x00, "None" },
	{ 0x01, "Nintendo" },
	{ 0x08, "Capcom" },
	{ 0x09, "Hot-B" },
	{ 0x0A, "Jaleco" },
	{ 0x0B, "Coconuts Japan" },
	{ 0x0C, "Elite Systems" },
	{ 0x13, "EA (Electronic Arts)" },
	{ 0x18, "Hudsonsoft" },
	{ 0x19, "ITC Entertainment" },
	{ 0x1A, "Yanoman" },
	{ 0x1D, "Japan Clary" },
	{ 0x1F, "Virgin Interactive" },
	{ 0x24, "PCM Complete" },
	{ 0x25, "San-X" },
	{ 0x28, "Kotobuki Systems" },
	{ 0x29, "Seta" },
	{ 0x30, "Infogrames" },
	{ 0x31, "Nintendo" },
	{ 0x32, "Bandai" },
	//{ 0x33, "Indicates that the New licensee code should be used instead." },
	{ 0x34, "Konami" },
	{ 0x35, "HectorSoft" },
	{ 0x38, "Capcom" },
	{ 0x39, "Banpresto" },
	{ 0x3C, ".Entertainment i" },
	{ 0x3E, "Gremlin" },
	{ 0x41, "Ubisoft" },
	{ 0x42, "Atlus" },
	{ 0x44, "Malibu" },
	{ 0x46, "Angel" },
	{ 0x47, "Spectrum Holoby" },
	{ 0x49, "Irem" },
	{ 0x4A, "Virgin Interactive" },
	{ 0x4D, "Malibu" },
	{ 0x4F, "U.S. Gold" },
	{ 0x50, "Absolute" },
	{ 0x51, "Acclaim" },
	{ 0x52, "Activision" },
	{ 0x53, "American Sammy" },
	{ 0x54, "GameTek" },
	{ 0x55, "Park Place" },
	{ 0x56, "LJN" },
	{ 0x57, "Matchbox" },
	{ 0x59, "Milton Bradley" },
	{ 0x5A, "Mindscape" },
	{ 0x5B, "Romstar" },
	{ 0x5C, "Naxat Soft" },
	{ 0x5D, "Tradewest" },
	{ 0x60, "Titus" },
	{ 0x61, "Virgin Interactive" },
	{ 0x67, "Ocean Interactive" },
	{ 0x69, "EA (Electronic Arts)" },
	{ 0x6E, "Elite Systems" },
	{ 0x6F, "Electro Brain" },
	{ 0x70, "Infogrames" },
	{ 0x71, "Interplay" },
	{ 0x72, "Broderbund" },
	{ 0x73, "Sculptered Soft" },
	{ 0x75, "The Sales Curve" },
	{ 0x78, "t.hq" },
	{ 0x79, "Accolade" },
	{ 0x7A, "Triffix Entertainment" },
	{ 0x7C, "Microprose" },
	{ 0x7F, "Kemco" },
	{ 0x80, "Misawa Entertainment" },
	{ 0x83, "Lozc" },
	{ 0x86, "Tokuma Shoten Intermedia" },
	{ 0x8B, "Bullet-Proof Software" },
	{ 0x8C, "Vic Tokai" },
	{ 0x8E, "Ape" },
	{ 0x8F, "I\'Max" },
	{ 0x91, "Chunsoft Co." },
	{ 0x92, "Video System" },
	{ 0x93, "Tsubaraya Productions Co." },
	{ 0x95, "Varie Corporation" },
	{ 0x96, "Yonezawa/S\'Pal" },
	{ 0x97, "Kaneko" },
	{ 0x99, "Arc" },
	{ 0x9A, "Nihon Bussan" },
	{ 0x9B, "Tecmo" },
	{ 0x9C, "Imagineer" },
	{ 0x9D, "Banpresto" },
	{ 0x9F, "Nova" },
	{ 0xA1, "Hori Electric" },
	{ 0xA2, "Bandai" },
	{ 0xA4, "Konami" },
	{ 0xA6, "Kawada" },
	{ 0xA7, "Takara" },
	{ 0xA9, "Technos Japan" },
	{ 0xAA, "Broderbund" },
	{ 0xAC, "Toei Animation" },
	{ 0xAD, "Toho" },
	{ 0xAF, "Namco" },
	{ 0xB0, "acclaim" },
	{ 0xB1, "ASCII or Nexsoft" },
	{ 0xB2, "Bandai" },
	{ 0xB4, "Square Enix" },
	{ 0xB6, "HAL Laboratory" },
	{ 0xB7, "SNK" },
	{ 0xB9, "Pony Canyon" },
	{ 0xBA, "Culture Brain" },
	{ 0xBB, "Sunsoft" },
	{ 0xBD, "Sony Imagesoft" },
	{ 0xBF, "Sammy" },
	{ 0xC0, "Taito" },
	{ 0xC2, "Kemco" },
	{ 0xC3, "Squaresoft" },
	{ 0xC4, "Tokuma Shoten Intermedia" },
	{ 0xC5, "Data East" },
	{ 0xC6, "Tonkinhouse" },
	{ 0xC8, "Koei" },
	{ 0xC9, "UFL" },
	{ 0xCA, "Ultra" },
	{ 0xCB, "Vap" },
	{ 0xCC, "Use Corporation" },
	{ 0xCD, "Meldac" },
	{ 0xCE, ".Pony Canyon or" },
	{ 0xCF, "Angel" },
	{ 0xD0, "Taito" },
	{ 0xD1, "Sofel" },
	{ 0xD2, "Quest" },
	{ 0xD3, "Sigma Enterprises" },
	{ 0xD4, "ASK Kodansha Co." },
	{ 0xD6, "Naxat Soft" },
	{ 0xD7, "Copya System" },
	{ 0xD9, "Banpresto" },
	{ 0xDA, "Tomy" },
	{ 0xDB, "LJN" },
	{ 0xDD, "NCS" },
	{ 0xDE, "Human" },
	{ 0xDF, "Altron" },
	{ 0xE0, "Jaleco" },
	{ 0xE1, "Towa Chiki" },
	{ 0xE2, "Yutaka" },
	{ 0xE3, "Varie" },
	{ 0xE5, "Epcoh" },
	{ 0xE7, "Athena" },
	{ 0xE8, "Asmik ACE Entertainment" },
	{ 0xE9, "Natsume" },
	{ 0xEA, "King Records" },
	{ 0xEB, "Atlus" },
	{ 0xEC, "Epic/Sony Records" },
	{ 0xEE, "IGS" },
	{ 0xF0, "A Wave" },
	{ 0xF3, "Extreme Entertainment" },
	{ 0xFF, "LJN" },
};

const std::map<std::string, std::string> new_publisher_info =
{
	{ "00",	"None" },
	{ "01",	"Nintendo R&D1" },
	{ "08",	"Capcom" },
	{ "13",	"Electronic Arts" },
	{ "18",	"Hudson Soft" },
	{ "19",	"b-ai" },
	{ "20",	"kss" },
	{ "22",	"pow" },
	{ "24",	"PCM Complete" },
	{ "25",	"san-x" },
	{ "28",	"Kemco Japan" },
	{ "29",	"seta" },
	{ "30",	"Viacom" },
	{ "31",	"Nintendo" },
	{ "32",	"Bandai" },
	{ "33",	"Ocean/Acclaim" },
	{ "34",	"Konami" },
	{ "35",	"Hector" },
	{ "37",	"Taito" },
	{ "38",	"Hudson" },
	{ "39",	"Banpresto" },
	{ "41",	"Ubi Soft" },
	{ "42",	"Atlus" },
	{ "44",	"Malibu" },
	{ "46",	"angel" },
	{ "47",	"Bullet-Proof" },
	{ "49",	"irem" },
	{ "50",	"Absolute" },
	{ "51",	"Acclaim" },
	{ "52",	"Activision" },
	{ "53",	"American sammy" },
	{ "54",	"Konami" },
	{ "55",	"Hi tech entertainment" },
	{ "56",	"LJN" },
	{ "57",	"Matchbox" },
	{ "58",	"Mattel" },
	{ "59",	"Milton Bradley" },
	{ "60",	"Titus" },
	{ "61",	"Virgin" },
	{ "64",	"LucasArts" },
	{ "67",	"Ocean" },
	{ "69",	"Electronic Arts" },
	{ "70",	"Infogrames" },
	{ "71",	"Interplay" },
	{ "72",	"Broderbund" },
	{ "73",	"sculptured" },
	{ "75",	"sci" },
	{ "78",	"THQ" },
	{ "79",	"Accolade" },
	{ "80",	"misawa" },
	{ "83",	"lozc" },
	{ "86",	"Tokuma Shoten Intermedia" },
	{ "87",	"Tsukuda Original" },
	{ "91",	"Chunsoft" },
	{ "92",	"Video system" },
	{ "93",	"Ocean/Acclaim" },
	{ "95",	"Varie" },
	{ "96",	"Yonezawa/s\'pal" },
	{ "97",	"Kaneko" },
	{ "99",	"Pack in soft" },
	{ "A4",	"Konami (Yu-Gi-Oh!)" },
};


#pragma warning(push)
#pragma warning(disable : 26495)
Cartridge::Cartridge()
{
}
#pragma warning(pop)

Cartridge::~Cartridge()
{
	SAFE_DELETE(mapper);
}

void Cartridge::Load(std::filesystem::path path)
{
	Reset();

	std::ifstream input(path, std::ios::binary | std::ios::ate);
	if (input.is_open())
	{
		input.seekg(0, std::ios::beg);

		// reserve enough room for the header
		rom.resize(0x150);

		// read in the header
		for (size_t i = 0; i < rom.size(); i++)
		{
			input >> std::noskipws >> rom[i];
		}
		DeserializeHeader();
		isLoaded = true;

		// set rom and ram sizes according to the header
		rom.resize(GetRomSize());
		ram.resize(GetRamSize());

		// read in the entire rom
		input.seekg(0, std::ios::beg);
		rom.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		input.close();

		
		// init the mapper
		InitializeMapper();
	}
}

u8 Cartridge::Read(u16 addr) const
{
	if (mapper != nullptr)
	{
		return mapper->Read(addr);
	}

	return rom[addr];
}

void Cartridge::Write(u16 addr, u8 data)
{
	if (mapper != nullptr)
	{
		mapper->Write(addr, data);
	}
}

void Cartridge::Reset()
{
	mapper = nullptr;
	rom.clear();
	ram.clear();
	mapperSupported = false;
	isLoaded = false;
}

std::string Cartridge::GetTitle() const
{
	if (!isLoaded)
		return "";

	std::stringstream ss;
	for (auto c : header.title)
	{
		if (c != '\0')
			ss << c;
		else
			break;
	}

	return ss.str();
}

std::string Cartridge::GetManufacturerCode() const
{
	if (!isLoaded)
		return "";

	return std::string(header.manufacturer_code.begin(), header.manufacturer_code.end());
}

u8 Cartridge::GetCGBFlag() const
{
	if (!isLoaded)
		return 0;

	return header.cgb_flag;
}

std::string Cartridge::GetPublisher() const
{
	if (!isLoaded)
		return "";

	if (header.old_publisher_code != 0x33)
	{
		return old_publisher_info.at(header.old_publisher_code);
	}
	else
	{
		std::string code(header.new_publisher_code.begin(), header.new_publisher_code.end());
		if (!new_publisher_info.contains(code))
		{
			return code;
		}
		else
		{
			return new_publisher_info.at(code);
		}
	}
}

u8 Cartridge::GetSGBFlag() const
{
	if (!isLoaded)
		return 0;

	return header.sgb_flag;
}

MapperType Cartridge::GetMapperType() const
{
	if (!isLoaded)
		return MapperType::ROM_ONLY;

	return header.type;
}

std::string Cartridge::GetMapperTypeAsString() const
{
	if (!isLoaded)
		return "";

	if (MapperTypeToString.contains(header.type))
		return MapperTypeToString.at(header.type);
	else
		return "";
}

u64 Cartridge::GetRomSize() const
{
	if (!isLoaded)
		return 0;

	return rom_info[header.rom_size].size;
}

u16 Cartridge::GetRomBanks() const
{
	if (!isLoaded)
		return 0;

	return rom_info[header.rom_size].banks;
}

u64 Cartridge::GetRamSize() const
{
	if (!isLoaded)
		return 0;

	return ram_info[header.ram_size].size;
}

u8 Cartridge::GetRamBanks() const
{
	if (!isLoaded)
		return 0;

	return ram_info[header.ram_size].banks;
}

u8 Cartridge::GetRegionCode() const
{
	if (!isLoaded)
		return 0;

	// 0x00 is japan, 0x01 is everywhere else
	return header.region_code;
}

u8 Cartridge::GetRomVersionNumber() const
{
	if (!isLoaded)
		return 0;

	return header.rom_version_number;
}

u8 Cartridge::GetHeaderChecksum() const
{
	if (!isLoaded)
		return 0;

	return header.header_checksum;
}

u16 Cartridge::GetGlobalChecksum() const
{
	if (!isLoaded)
		return 0;

	// checksum is big endian
	u16 checksum = header.global_checksum[0] << 7 | header.global_checksum[1];
	return checksum;
}

const BaseMapper* Cartridge::GetMapper() const
{
	if (!isLoaded)
		return nullptr;

	return mapper;
}

bool Cartridge::IsMapperSupported() const
{
	if (!isLoaded)
		return false;

	return mapperSupported;
}

bool Cartridge::IsLoaded() const
{
	return isLoaded;
}

void Cartridge::DeserializeHeader()
{
	for (size_t i = 0; i < header.title.size(); i++)
		header.title[i]					= rom[i + 0x0134]; // 0x0134-0x0143
	
	for (size_t i = 0; i < header.manufacturer_code.size(); i++)
		header.manufacturer_code[i]		= rom[i + 0x013F]; // 0x013F-0x0142
	
	header.cgb_flag						= rom[0x0143];

	for (size_t i = 0; i < header.new_publisher_code.size(); i++)
		header.new_publisher_code[i]	= rom[i + 0x0144]; // 0x0144-0x0145
	
	header.sgb_flag						= rom[0x0143];
	header.type							= (MapperType)rom[0x0147];
	header.rom_size						= rom[0x0148];
	header.ram_size						= rom[0x0149];
	header.region_code					= rom[0x014A];
	header.old_publisher_code			= rom[0x014B];
	header.rom_version_number			= rom[0x014C];
	header.header_checksum				= rom[0x014D];

	for (size_t i = 0; i < header.global_checksum.size(); i++)
		header.global_checksum[i]		= rom[i + 0x014E]; // 0x014E-0x014F
}

void Cartridge::InitializeMapper()
{
	mapperSupported = true;

	switch (GetMapperType())
	{
		case MapperType::ROM_ONLY:
			mapper = nullptr;
			break;

		case MapperType::MBC1:
		case MapperType::MBC1_RAM:
		case MapperType::MBC1_RAM_BATTERY:
			mapper = new MBC1(this);
			break;

		case MapperType::MBC3:
		case MapperType::MBC3_RAM:
		case MapperType::MBC3_RAM_BATTERY:
			mapper = new MBC3(this);
			break;

		default:
			mapper = nullptr;
			mapperSupported = false;
			break;
	}
}