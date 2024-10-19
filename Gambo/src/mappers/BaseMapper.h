#pragma once
#include "GamboDefine.h"

class Cartridge;

class BaseMapper
{
	BaseMapper() = delete; // default constructor
	BaseMapper(const BaseMapper& other) = delete; // copy constructor
	BaseMapper& operator=(const BaseMapper& other) = delete; // copy assignment operator
	BaseMapper(const BaseMapper&& other) = delete; // move constructor
	BaseMapper& operator=(const BaseMapper&& other) = delete; // move assignment operator.

public:
	BaseMapper(Cartridge* cartridge) : cart(cartridge) {};
	virtual ~BaseMapper() {};

	virtual u8 Read(u16 addr) = 0;
	virtual void Write(u16 addr, u8 data) = 0;

protected:
	Cartridge* cart;
};

