#pragma once
#include "GamboDefine.h"


class BootRom
{
	friend class BootRomDMG;
public:
	BootRom() {};
	virtual ~BootRom() {};

	virtual const u8 Read(u8 addr) const = 0;
	virtual void Reset() = 0;
private:
	std::array<u8, 256> rom;
};