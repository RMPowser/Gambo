#pragma once
#include "GamboDefine.h"

class Bus;

class PPU
{
public:
	Bus* bus = nullptr;
	bool DoDMATransfer = false;

	PPU(Bus* b);
	~PPU();

	u8 Read(u16 addr);
	void Write(u16 addr, u8 data);
	void Clock(void* target);
};