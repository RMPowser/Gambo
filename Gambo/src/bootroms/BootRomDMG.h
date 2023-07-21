#pragma once
#include "BootRom.h"

class BootRomDMG
	: public BootRom
{
public:
	BootRomDMG();
	~BootRomDMG();

	virtual const u8 Read(u8 addr) const override;
	virtual void Reset() override;
};