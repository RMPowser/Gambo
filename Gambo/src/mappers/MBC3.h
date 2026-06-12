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
    
    struct RTC
    {
        u8 seconds; // seconds 0-59
        u8 minutes; // minutes 0-59
        u8 hours; // hours 0-23
        u8 dayLow; // lower byte of day counter 0x00-0xFF
        u8 dayHigh; // upper 1 bit of day counter, halt flag, and carry bit.
                    // bit 0 is the most significant bit of the day counter.
                    // bit 6 is halt flag. 1 if halted.
                    // bit 7 is the carry bit for the day counter. 1 indicates overflow. 
    };

    RTC clock;

private:
    bool ramAndRTCEnabled; // RTC stands for real time clock
    u8 romBankNumber;
    u8 ramBankNumber;

    enum class MappedRegister : u8
    {
        NONE,
        RTC_S = 0x08,
        RTC_M,
        RTC_H,
        RTC_DL,
        RTC_DH,
    } mappedRTCRegister; // indicates which rtc register is mapped to range 0xA000-0xBFFF.

    bool isHalted;

    // When writing 0x00, and then 0x01 to 0x6000-0x7FFF, the current time becomes latched 
    // into the RTC registers. The latched data will not change until it becomes latched 
    // again, by repeating the write 0x00->0x01 procedure. This provides a way to read the 
    // RTC registers while the clock keeps ticking.
    bool wasZeroWritten;
    bool isLatched; 
};