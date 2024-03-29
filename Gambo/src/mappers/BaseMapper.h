#pragma once
#include "GamboDefine.h"

class Cartridge;

class BaseMapper
{
	BaseMapper() = delete;
public:
	BaseMapper(Cartridge* cartridge) : cart(cartridge) {};
	virtual ~BaseMapper() {};

	virtual u8 Read(u16 addr) = 0;
	virtual void Write(u16 addr, u8 data) = 0;

	bool operator==(const BaseMapper& other) const = delete;

protected:
	Cartridge* cart;
};

