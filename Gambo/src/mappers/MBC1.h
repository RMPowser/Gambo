#pragma once
#include "BaseMapper.h"

class Cartridge;

class MBC1 :
    public BaseMapper
{
    MBC1() = delete; // default constructor
    MBC1(const MBC1& other) = delete; // copy constructor
    MBC1& operator=(const MBC1& other) = delete; // copy assignment operator
    MBC1(const MBC1&& other) = delete; // move constructor
    MBC1& operator=(const MBC1&& other) = delete; // move assignment operator

public:
    MBC1(Cartridge* cart);
    ~MBC1();

    u8 Read(u16 addr) override;
    void Write(u16 addr, u8 data) override;

private:
    bool ramEnabled;
    u8 romBankNumber;
    u8 ramBankNumber;
    bool bankingModeSelect;
    u8 bankNumberBitWidth;
};

