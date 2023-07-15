#pragma once
#include "BaseMapper.h"

class Cartridge;

class MBC1 :
    public BaseMapper
{
public:

    MBC1(Cartridge* cart);
    ~MBC1();

    u8 Read(u16 addr) override;
    void Write(u16 addr, u8 data) override;

    bool operator==(const MBC1& other) const = delete;

private:
    bool ramEnabled;
    u8 romBankNumber;
    u8 ramBankNumber;
    bool bankingModeSelect;
    u8 bankNumberBitWidth;
};

