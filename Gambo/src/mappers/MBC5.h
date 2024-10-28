#pragma once
#include "BaseMapper.h"

class Cartridge;

class MBC5 :
    public BaseMapper
{
    MBC5() = delete; // default constructor
    MBC5(const MBC5& other) = delete; // copy constructor
    MBC5& operator=(const MBC5& other) = delete; // copy assignment operator
    MBC5(const MBC5&& other) = delete; // move constructor
    MBC5& operator=(const MBC5&& other) = delete; // move assignment operator

public:
    MBC5(Cartridge* cart);
    ~MBC5();

    u8 Read(u16 addr) override;
    void Write(u16 addr, u8 data) override;
    
private:
    bool ramEnabled;
    u16 romBankNumber; // 9 bits
    u8 ramBankNumber;
};