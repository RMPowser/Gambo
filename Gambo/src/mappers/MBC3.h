#pragma once
#include "BaseMapper.h"

class Cartridge;

class MBC3 :
    public BaseMapper
{
    MBC3() = delete; // default constructor
    MBC3(const MBC3& other) = delete; // copy constructor
    MBC3& operator=(const MBC3& other) = delete; // copy assignment operator
    MBC3(const MBC3&& other) = delete; // move constructor
    MBC3& operator=(const MBC3&& other) = delete; // move assignment operator

public:
    MBC3(Cartridge* cart);
    ~MBC3();

    u8 Read(u16 addr) override;
    void Write(u16 addr, u8 data) override;
    
private:
    bool ramAndRTCEnabled;
    u8 romBankNumber;
    u8 ramBankNumber;
    bool bankingModeSelect;
    u8 bankNumberBitWidth;
};