#pragma once
#include "GamboDefine.h"
#include "Register.h"

class GamboCore;
struct SDL_AudioStream;

enum NR50Bits
{
	RightVolume = 0,
	VIN_Right = 3,
	LeftVolume = 4,
	VIN_Left = 7,
};

enum NR51Bits
{
	CH1_Right = 0,
	CH2_Right = 1,
	CH3_Right = 2,
	CH4_Right = 3,
	CH1_Left = 4,
	CH2_Left = 5,
	CH3_Left = 6,
	CH4_Left = 7,
};

enum NR52Bits
{
	CH1_Enable = 0,
	CH2_Enable = 1,
	CH3_Enable = 2,
	CH4_Enable = 3,
	Audio_Enable = 7,
};

enum NR10Bits : u8
{
	SweepShift = 0,
	Direction = 3,
	SweepPeriod = 4,
	UnusedNR10 = 7,
};

enum NRx1Bits
{
	InitialLengthTimer = 0,
	DutyCycleIndex = 6
};

enum NRx2Bits
{
	EnvelopePeriod = 0,
	EnvelopeDirection = 3,
	InitialVolume = 4,
};

enum NRx4Bits : u8
{
	PeriodHigh = 0,
	UnusedNRx4 = 3,
	LengthEnable = 6,
	Trigger = 7,
};

enum NR30Bits
{
	UnusedNR30 = 0,
	DACEnabled = 7,
};

enum NR32Bits
{
	UnusedNR32_1 = 0,
	VolumeScale = 5,
	UnusedNR32_2 = 7
};

enum NR43Bits
{
	ClockDivider = 0, 
	LFSRWidth = 3,
	ClockShift = 4,
};

enum class AudioOutputMode
{
	Mono,
	Stereo
};

enum class ChannelId
{
	Square1,
	Square2,
	Wave,
	Noise
};

class APU
{
public:
	APU(const GamboCore* core);
	~APU();

	APU(const APU& other) = delete;
	APU& operator=(const APU& other) = delete;

	void RunFor(int cycles);

	void Enable();
	void Disable();
	void Reset();

	void TriggerSquare1();
	void DisableSquare1();
	void TriggerSquare2();
	void DisableSquare2();
	void TriggerWave();
	void DisableWave();
	void TriggerNoise();
	void DisableNoise();

private:
	void StepFrequency();
	void StepEnvelope();
	void StepLength();
	void GenerateSamples();
	void OutputSamples();

	struct AudioChannel
	{
		AudioChannel(const GamboCore* core);

		virtual void Trigger() = 0;
		virtual u16 GetFrequencyTimer() = 0;
		virtual void StepFrequency() = 0;
		virtual void StepEnvelope() = 0;
		virtual void StepLength();
		virtual void GenerateSample(int channel) = 0;
		virtual void Reset();

		bool enabled;

		int frequencyTimer; // loaded from 11 bits. tells us when to step frequency
		Register<6> lengthTimer; // tells us when disable the channel
		bool lengthTimerEnabled; 

		Register<4> volume;
		Register<3> envelopePeriod; // amount of ticks before stepping volume envelope
		Register<3> envelopeTimer; // tells us when to step volume envelope
		bool envelopeDirection; // false increase, true decrease

		std::vector<float> samples;
		const GamboCore* core;
	};

	struct SquareChannel : AudioChannel
	{
		SquareChannel(const GamboCore* core);

		virtual void Trigger() = 0;
		virtual u16 GetFrequencyTimer() = 0;

		virtual void Reset() override;
		virtual void StepFrequency() override;
		virtual void StepEnvelope() override;
		virtual void GenerateSample(int channel) final override;

		Register<2> dutyCycle; // 2-bit duty cycle (12.5%, 25%, 50%, 75%)
		Register<3> dutyStep; // 3-bit step position in the duty cycle

	private:
		using super = AudioChannel;
	};

	struct SquareChannel1 : SquareChannel
	{
		SquareChannel1(const GamboCore* core);

		virtual void Trigger() final override;
		virtual u16 GetFrequencyTimer() final override;
		u16 CalculateNewFrequency();
		virtual void Reset() final override;
		
		void StepSweep();

		Register<3> sweepTimer; // tells us when to step sweep
		Register<3> sweepPeriod; // amount of ticks before stepping sweep
		bool sweepDirection; // false incread, true decrease
		bool sweepEnabled;

	private:
		using super = SquareChannel;
	};

	struct SquareChannel2 : SquareChannel 
	{
		SquareChannel2(const GamboCore* core);
		
		virtual void Trigger() final override;
		virtual u16 GetFrequencyTimer() final override;

	private:
		using super = SquareChannel;
	};

	struct NoiseChannel : AudioChannel
	{
		NoiseChannel(const GamboCore* core);

		virtual void Trigger() final override;
		virtual u16 GetFrequencyTimer() final override;
		virtual void Reset() final override;
		virtual void StepFrequency() final override;
		virtual void StepEnvelope() final override;
		virtual void GenerateSample(int channel) final override;

	private:
		Register<16> lfsr; // Linear Feedback Shift Register (15-bit)

		int frequencyDivider; // Frequency divider from NR43

		using super = AudioChannel;
	};

	struct WaveChannel : AudioChannel {
		WaveChannel(const GamboCore* core);

		virtual void Trigger() final override;
		virtual u16 GetFrequencyTimer() final override;
		virtual void Reset() final override;
		virtual void StepFrequency() final override;
		virtual void StepLength() final override;
		virtual void StepEnvelope() final override {} // Wave channel has no envelope
		virtual void GenerateSample(int channel) final override;

	private:
		Register<5> waveIndex; // 5-bit index into Wave RAM
		Register<2> volumeScale; // Volume scaling factor (NR32)
		using super = AudioChannel;
	};

	const GamboCore* core;

	SquareChannel1 square1;
	SquareChannel2 square2;
	WaveChannel wave;
	NoiseChannel noise;

	AudioOutputMode outputMode;
	SDL_AudioStream* outputStream;
	std::vector<float> masterSamples;

	int samplePeriod; // number of cycles before generating a new sample
	int sampleTimer; // tells us when to generate a new sample
	int frameTimer; // tells us when to output samples
	int lengthTimer; // tells us when to tick length
	int volumeTimer; // tells us when to tick volume envelope
	int sweepTimer; // tells us when to tick square1 sweep
};