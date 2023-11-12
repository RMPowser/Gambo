#pragma once
#include "BaseMapper.h"

class Cartridge;

class MBC3 :
    public BaseMapper
{
    MBC3() = delete;
public:

    MBC3(Cartridge* cart);
    ~MBC3();

    u8 Read(u16 addr) override;
    void Write(u16 addr, u8 data) override;

    bool operator==(const MBC3& other) const = delete;

private:
    bool ramAndRTCEnabled;
    u8 romBankNumber;
    u8 ramBankNumber;
    bool bankingModeSelect;
    u8 bankNumberBitWidth;
};