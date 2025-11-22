#include "APU.h"
#include "GamboCore.h"
#include "RAM.h"
#include "SDL3/SDL.h"
#include <vector>
#include <numbers>
#include <array>

constexpr float dutyWaveforms[4][8] = {
	{0, 0, 0, 0, 0, 0, 0, 1}, // 12.5%
	{1, 0, 0, 0, 0, 0, 0, 1}, // 25%
	{1, 1, 1, 0, 0, 0, 0, 1}, // 50%
	{0, 1, 1, 1, 1, 1, 1, 0}  // 75%
};

constexpr float waveVolumeScales[4] = { 0.0, 1.0, 0.5, 0.25 };

constexpr int sampleRate = 32768;
constexpr int squareFrequencyPeriod = 1048576_hz;
constexpr int waveFrequencyPeriod = 2097152_hz;
constexpr int noiseFrequencyPeriod = 262144_hz;
constexpr int lengthPeriod = 256_hz;
constexpr int volumePeriod = 64_hz;
constexpr int sweepPeriod = 128_hz;
constexpr int samplePeriod = GamboClockSpeed / sampleRate;

APU::APU(const GamboCore* core)
	: core(core)
	, square1(core)
	, square2(core)
	, wave(core)
	, noise(core)
	, outputMode(AudioOutputMode::Stereo)
	, outputStream(nullptr)
	, masterSamples()
	, sampleTimer(samplePeriod)
	, frameTimer(GamboCyclesPerFrame)
	, lengthTimer(lengthPeriod)
	, volumeTimer(volumePeriod)
	, sweepTimer(sweepPeriod)
{
	const SDL_AudioSpec outputSpec = { SDL_AUDIO_F32, 2, 48000 };

	SDL_AudioDeviceID deviceId = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &outputSpec);
	if (!deviceId)
	{
		SDL_Log("Couldn't open audio device: %s", SDL_GetError());
		throw;
	}

	const SDL_AudioSpec inputSpec = { SDL_AUDIO_F32, 2, sampleRate };
	outputStream = SDL_CreateAudioStream(&inputSpec, &outputSpec);

	if (outputStream)
	{
		SDL_BindAudioStream(deviceId, outputStream);
		SDL_ResumeAudioStreamDevice(outputStream);
	}
	else
	{
		SDL_Log("Failed to open one or more audio streams: %s", SDL_GetError());
		throw;
	}
}

APU::~APU()
{
	SDL_DestroyAudioStream(outputStream);
}

void APU::RunFor(int cycles)
{
	const u8& NR52 = core->ram->Get(HWAddr::NR52);
	if (!GetBits(NR52, Audio_Enable))
	{
		return;
	}

	while (--cycles >= 0)
	{
		StepFrequency();

		if (--lengthTimer <= 0)
		{
			lengthTimer += lengthPeriod;
			StepLength();
		}

		if (--volumeTimer <= 0)
		{
			volumeTimer += volumePeriod;
			StepEnvelope();
		}

		if (--sweepTimer <= 0)
		{
			sweepTimer += sweepPeriod;
			square1.StepSweep();
		}

		if (--sampleTimer <= 0)
		{
			sampleTimer += samplePeriod;
			GenerateSamples();
		}

		if (--frameTimer <= 0)
		{
			frameTimer += GamboCyclesPerFrame;
			OutputSamples();
		}
	}
}

void APU::Enable()
{
	Reset();
}

void APU::Disable()
{
	Reset();
}

void APU::Reset()
{
	sampleTimer = samplePeriod;
	frameTimer = GamboCyclesPerFrame;
	lengthTimer = lengthPeriod;
	volumeTimer = volumePeriod;
	sweepTimer = sweepPeriod;

	core->ram->Set(HWAddr::NR10, 0);
	core->ram->Set(HWAddr::NR11, 0);
	core->ram->Set(HWAddr::NR12, 0);
	core->ram->Set(HWAddr::NR13, 0);
	core->ram->Set(HWAddr::NR14, 0);
	core->ram->Set(HWAddr::NR21, 0);
	core->ram->Set(HWAddr::NR22, 0);
	core->ram->Set(HWAddr::NR23, 0);
	core->ram->Set(HWAddr::NR24, 0);
	core->ram->Set(HWAddr::NR30, 0);
	core->ram->Set(HWAddr::NR31, 0);
	core->ram->Set(HWAddr::NR32, 0);
	core->ram->Set(HWAddr::NR33, 0);
	core->ram->Set(HWAddr::NR34, 0);
	core->ram->Set(HWAddr::NR41, 0);
	core->ram->Set(HWAddr::NR42, 0);
	core->ram->Set(HWAddr::NR43, 0);
	core->ram->Set(HWAddr::NR44, 0);
	core->ram->Set(HWAddr::NR50, 0);
	core->ram->Set(HWAddr::NR51, 0);

	square1.Reset();
	square2.Reset();
	wave.Reset();
	noise.Reset();
}

void APU::TriggerSquare1()
{
	square1.Trigger();
}

void APU::DisableSquare1()
{
	square2.enabled = false;
	square2.Reset();
}

void APU::TriggerSquare2()
{
	square2.Trigger();
}

void APU::DisableSquare2()
{
	square2.enabled = false;
	square2.Reset();
}

void APU::TriggerWave()
{
	wave.Trigger();
}

void APU::DisableWave()
{
	wave.enabled = false;
	wave.Reset();
}

void APU::TriggerNoise()
{
	noise.Trigger();
}

void APU::DisableNoise()
{
	noise.enabled = false;
	noise.Reset();
}

void APU::StepFrequency()
{
	square1.StepFrequency();
	square2.StepFrequency();
	wave.StepFrequency();
	noise.StepFrequency();
}

void APU::StepEnvelope()
{
	square1.StepEnvelope();
	square2.StepEnvelope();
	wave.StepEnvelope();
	noise.StepEnvelope();
}

void APU::StepLength()
{
	square1.StepLength();
	square2.StepLength();
	wave.StepLength();
	noise.StepLength();
}

void APU::GenerateSamples()
{
	square1.GenerateSample(1);
	square2.GenerateSample(2);
	wave.GenerateSample(3);
	noise.GenerateSample(4);
}

void APU::OutputSamples()
{
	const u8& NR50 = core->ram->Get(HWAddr::NR51);
	float masterVolumeLeft = (float)GetBits(NR50, NR50Bits::LeftVolume, 0b111) / 7.f;
	float masterVolumeRight = (float)GetBits(NR50, NR50Bits::RightVolume, 0b111) / 7.f;

	masterSamples.resize(square1.samples.size());

	for (size_t i = 0; i < square1.samples.size(); i += 2)
	{
		float leftChannel = square1.samples[i];
		float rightChannel = square1.samples[i + 1];

		leftChannel += square2.samples[i];
		rightChannel += square2.samples[i + 1];

		leftChannel += wave.samples[i];
		rightChannel += wave.samples[i + 1];

		leftChannel += noise.samples[i];
		rightChannel += noise.samples[i + 1];

		leftChannel *= masterVolumeLeft;
		rightChannel *= masterVolumeRight;

		leftChannel /= 4;
		rightChannel /= 4;

		masterSamples[i] = leftChannel;
		masterSamples[i + 1] = rightChannel;
	}

	SDL_PutAudioStreamData(outputStream, masterSamples.data(), (int)masterSamples.size() * sizeof(decltype(masterSamples)::value_type));
	square1.samples.clear();
	square2.samples.clear();
	wave.samples.clear();
	noise.samples.clear();
	masterSamples.clear();
}

APU::AudioChannel::AudioChannel(const GamboCore* _core)
	: enabled(false)
	, frequencyTimer(0)
	, lengthTimer(0)
	, lengthTimerEnabled(false)
	, volume(0)
	, envelopePeriod(0)
	, envelopeTimer(0)
	, envelopeDirection(false)
	, core(_core)
{
}

void APU::AudioChannel::StepEnvelope()
{
	if (enabled)
	{
		if (envelopePeriod > 0 && ++envelopeTimer == envelopePeriod)
		{
			envelopeTimer = 0;
			if (envelopeDirection)
			{
				if (volume < 15)
					volume++;
			}
			else
			{
				if (volume > 0)
					volume--;
			}
		}
	}
}

void APU::AudioChannel::StepLength()
{
	if (enabled)
	{
		if (lengthTimerEnabled)
		{
			if (++lengthTimer == 64)
			{
				enabled = false;
			}
		}
	}
}

void APU::AudioChannel::Reset()
{
	enabled = false;
	frequencyTimer = 0;
	lengthTimer = 0;
	lengthTimerEnabled = false;
	volume = 0;
	envelopePeriod = 0;
	envelopeTimer = 0;
	envelopeDirection = false;
	samples.clear();
}


APU::SquareChannel::SquareChannel(const GamboCore* _core)
	: AudioChannel(_core)
	, dutyCycle(0)
	, dutyStep(0)
{
}

void APU::SquareChannel::Reset()
{
	super::Reset();
	dutyCycle = 0;
	dutyStep = 0;
}

void APU::SquareChannel::StepFrequency()
{
	if (enabled)
	{
		if (--frequencyTimer <= 0)
		{
			frequencyTimer = GetFrequencyTimer();
			++dutyStep;
		}
	}
}

void APU::SquareChannel::GenerateSample(int channel)
{
	if (!enabled)
	{
		samples.push_back(0);
		samples.push_back(0);

		return;
	}

	// Get waveform value. either 0 or 1
	float waveformOutput = dutyWaveforms[dutyCycle][dutyStep];

	// apply volume envelope, scaling output
	const float maxVolume = 15.f;
	waveformOutput *= (float)volume / maxVolume;

	const u8& NR51 = core->ram->Get(HWAddr::NR51);
		
	bool panLeft = false;
	bool panRight = false;
	if (channel == 1)
	{
		panLeft = GetBits(NR51, NR51Bits::CH1_Left);
		panRight = GetBits(NR51, NR51Bits::CH1_Right);
	}
	else if (channel == 2)
	{
		panLeft = GetBits(NR51, NR51Bits::CH2_Left);
		panRight = GetBits(NR51, NR51Bits::CH2_Right);
	}
	else
	{
		throw std::invalid_argument(std::string(__func__) + ": invalid channel \"" + std::to_string(channel) + "\"");
	}

	float leftChannel = panLeft ? waveformOutput : 0;
	float rightChannel = panRight ? waveformOutput : 0;

	samples.push_back(leftChannel);
	samples.push_back(rightChannel);
}

APU::SquareChannel1::SquareChannel1(const GamboCore* _core)
	: SquareChannel(_core)
	, sweepTimer(0)
	, sweepPeriod(0)
	, sweepDirection(false)
	, sweepEnabled(false)
{
}

void APU::SquareChannel1::Trigger()
{
	enabled = true;
	frequencyTimer = GetFrequencyTimer();

	const u8& NR11 = core->ram->Get(HWAddr::NR11);
	dutyCycle = GetBits(NR11, NRx1Bits::DutyCycleIndex);
	dutyStep = 0;
	lengthTimerEnabled = GetBits(core->ram->Get(HWAddr::NR14), NRx4Bits::LengthEnable);
	if (lengthTimer == 0)
	{
		lengthTimer = GetBits(NR11, NRx1Bits::InitialLengthTimer, 0b111111);
	}

	const u8& NR12 = core->ram->Get(HWAddr::NR12);
	envelopePeriod = GetBits(NR12, NRx2Bits::EnvelopePeriod, 0b111);
	envelopeDirection = GetBits(NR12, NRx2Bits::EnvelopeDirection);
	volume = GetBits(NR12, NRx2Bits::InitialVolume, 0b1111);
	envelopeTimer = envelopePeriod;

	const u8& NR10 = core->ram->Get(HWAddr::NR10);
	sweepPeriod = GetBits(NR10, NR10Bits::SweepPeriod, 0b111);
	sweepDirection = GetBits(NR10, NR10Bits::Direction);
	const u8 individualStep = GetBits(NR10, NR10Bits::SweepShift, 0b111);
	sweepEnabled = sweepPeriod > 0 || individualStep > 0;

	// frequency calculation and overflow check
	if (individualStep > 0)
	{
		u16 newFreq = CalculateNewFrequency();
		if (newFreq > 2047)
		{
			enabled = false;
		}
	}
}

u16 APU::SquareChannel1::GetFrequencyTimer()
{
	const u8& periodLow = core->ram->Get(HWAddr::NR13);
	const u8& NR14 = core->ram->Get(HWAddr::NR14);
	const u8 periodHigh = GetBits(NR14, NRx4Bits::PeriodHigh, 0b111);
	int currentFreq = (u16)periodLow | ((u16)periodHigh << 8);
	return (2048 - currentFreq) * squareFrequencyPeriod;
}

u16 APU::SquareChannel1::CalculateNewFrequency()
{
	const u8& periodLow = core->ram->Get(HWAddr::NR13);
	const u8& NR14 = core->ram->Get(HWAddr::NR14);
	const u8 periodHigh = GetBits(NR14, NRx4Bits::PeriodHigh, 0b111);
	int currentFreq = Register<11>((u16)periodLow | ((u16)periodHigh << 8));
	const u8& NR10 = core->ram->Get(HWAddr::NR14);
	int sweepShift = GetBits(NR10, NR10Bits::SweepShift, 0b111);
	int shiftedFreq = currentFreq >> sweepShift;

	if (sweepDirection)
	{
		currentFreq -= shiftedFreq;
	}
	else
	{
		currentFreq += shiftedFreq;
	}

	return currentFreq;
}

void APU::SquareChannel1::Reset()
{
	super::Reset();
	sweepPeriod = 0;
	sweepTimer = 0;
	sweepEnabled = false;
}

void APU::SquareChannel1::StepSweep()
{
	if (enabled)
	{
		if (!sweepEnabled || sweepPeriod == 0)
		{
			return;
		}

		if (--sweepTimer == 0)
		{
			sweepTimer = sweepPeriod;
		
			u16 newFreq = CalculateNewFrequency();
			if (newFreq > 2047) {
				enabled = false;
			}
			else 
			{
				const u8& NR14 = core->ram->Get(HWAddr::NR14);
				u8 newNR13 = newFreq & 0xFF;
				u8 bits3 = ((newFreq & (0b111 << 8)) >> 8);
				u8 newNR14 = (NR14 & 0b11111000) | bits3;
				core->ram->Set(HWAddr::NR13, newNR13);
				core->ram->Set(HWAddr::NR14, newNR14);
				frequencyTimer = GetFrequencyTimer();
			}
		}
	}
}

APU::SquareChannel2::SquareChannel2(const GamboCore* core)
	: super(core)
{
}

void APU::SquareChannel2::Trigger()
{
	enabled = true;
	frequencyTimer = GetFrequencyTimer();

	const u8& NR21 = core->ram->Get(HWAddr::NR21);
	dutyCycle = GetBits(NR21, NRx1Bits::DutyCycleIndex);
	dutyStep = 0;
	lengthTimerEnabled = GetBits(core->ram->Get(HWAddr::NR24), NRx4Bits::LengthEnable);
	if (lengthTimer == 0)
	{
		lengthTimer = GetBits(NR21, NRx1Bits::InitialLengthTimer, 0b111111);
	}

	const u8& NR22 = core->ram->Get(HWAddr::NR12);
	envelopePeriod = GetBits(NR22, NRx2Bits::EnvelopePeriod, 0b111);
	envelopeDirection = GetBits(NR22, NRx2Bits::EnvelopeDirection);
	volume = GetBits(NR22, NRx2Bits::InitialVolume, 0b1111);
	envelopeTimer = envelopePeriod;
}

u16 APU::SquareChannel2::GetFrequencyTimer()
{
	const u8& periodLow = core->ram->Get(HWAddr::NR23);
	const u8& NR24 = core->ram->Get(HWAddr::NR24);
	const u8 periodHigh = GetBits(NR24, NRx4Bits::PeriodHigh, 0b111);
	int currentFreq = (u16)periodLow | ((u16)periodHigh << 8);
	return (2048 - currentFreq) * squareFrequencyPeriod;
}

APU::NoiseChannel::NoiseChannel(const GamboCore* core)
	: super(core)
	, lfsr(0)
	, clockShift(0)
	, widthMode(0)
	, frequencyDivider(0)
{
}

void APU::NoiseChannel::Reset() 
{
	super::Reset();
	lfsr = 0;
	clockShift = 0;
	widthMode = 0;
	frequencyDivider = 0;
}

void APU::NoiseChannel::Trigger() 
{
	enabled = true;

	const u8& NR41 = core->ram->Get(HWAddr::NR41);
	lengthTimer = GetBits(NR41, NRx1Bits::InitialLengthTimer, 0b111111);
	lengthTimerEnabled = GetBits(core->ram->Get(HWAddr::NR44), NRx4Bits::LengthEnable);

	const u8& NR42 = core->ram->Get(HWAddr::NR42);
	envelopePeriod = GetBits(NR42, NRx2Bits::EnvelopePeriod, 0b111);
	envelopeDirection = GetBits(NR42, NRx2Bits::EnvelopeDirection);
	volume = GetBits(NR42, NRx2Bits::InitialVolume, 0b1111);
	envelopeTimer = envelopePeriod;

	const u8& NR43 = core->ram->Get(HWAddr::NR43);
	clockShift = GetBits(NR43, NR43Bits::ClockShift, 0b1111);
	widthMode = GetBits(NR43, NR43Bits::LFSRWidth);
	frequencyDivider = GetBits(NR43, NR43Bits::ClockDivider, 0b111);
	lfsr = 0;
}

u16 APU::NoiseChannel::GetFrequencyTimer()
{
	float divider = frequencyDivider == 0 ? 0.5f : frequencyDivider;

	return (divider * (int)std::pow(2, clockShift)) * noiseFrequencyPeriod;
}

void APU::NoiseChannel::StepFrequency() 
{
	if (enabled)
	{
		if (--frequencyTimer <= 0) 
		{
			frequencyTimer = GetFrequencyTimer(); 

			// Advance the LFSR
			bool xnorResult = lfsr.GetBits(0) == lfsr.GetBits(1);
			lfsr.SetBit(15, xnorResult);
			if (GetBits(core->ram->Get(HWAddr::NR43), NR43Bits::LFSRWidth)) 
			{
				lfsr.SetBit(7, xnorResult);
			}
			lfsr >>= 1;
		}
	}
}

void APU::NoiseChannel::GenerateSample(int channel) 
{
	if (!enabled) {
		samples.push_back(0);
		samples.push_back(0);
		return;
	}

	float waveformOutput = lfsr.GetBits(0) ? 0 : 1;
	waveformOutput *= (float)volume / 15.0f; // Apply volume

	const u8& NR51 = core->ram->Get(HWAddr::NR51);
	bool panLeft = GetBits(NR51, NR51Bits::CH4_Left);
	bool panRight = GetBits(NR51, NR51Bits::CH4_Right);

	float leftChannel = panLeft ? waveformOutput : 0.0f;
	float rightChannel = panRight ? waveformOutput : 0.0f;

	samples.push_back(leftChannel);
	samples.push_back(rightChannel);
}

APU::WaveChannel::WaveChannel(const GamboCore* core)
	: AudioChannel(core)
	, waveIndex(0)
	, volumeScale(0)
{
}

void APU::WaveChannel::Reset()
{
	super::Reset();
	waveIndex = 0;
	volumeScale = 0;
}

void APU::WaveChannel::Trigger()
{
	enabled = GetBits(core->ram->Get(HWAddr::NR30), NR30Bits::DACEnabled); // Check NR30 enable flag

	if (!enabled)
		return;

	lengthTimer = core->ram->Get(HWAddr::NR31);
	lengthTimerEnabled = GetBits(core->ram->Get(HWAddr::NR34), NRx4Bits::LengthEnable);

	const u8& NR32 = core->ram->Get(HWAddr::NR32);
	volumeScale = GetBits(NR32, NR32Bits::VolumeScale, 0b11); // Volume scaling (00 = mute, 01 = 100%, 10 = 50%, 11 = 25%)

	frequencyTimer = GetFrequencyTimer();
	waveIndex = 0; // Start at the first sample
}

void APU::WaveChannel::StepFrequency() 
{
	if (enabled)
	{
		if (--frequencyTimer <= 0)
		{
			frequencyTimer = GetFrequencyTimer();
			++waveIndex;
		}
	}
}

void APU::WaveChannel::StepLength() 
{
	if (enabled) 
	{
		if (lengthTimerEnabled) 
		{
			if (++lengthTimer == 256)
			{
				enabled = false;
			}
		}
	}
}

void APU::WaveChannel::GenerateSample(int channel) 
{
	if (!enabled) 
	{
		samples.push_back(0);
		samples.push_back(0);
		return;
	}

	// Fetch 4-bit sample from Wave RAM (NR30�NR3F)
	u8 waveByte = core->ram->Get(HWAddr::WAVE + (waveIndex / 2)); // Each byte holds 2 samples
	u8 waveSample = (waveIndex % 2 == 0) ? (waveByte >> 4) : (waveByte & 0x0F); // Upper 4 bits or lower 4 bits

	// Apply volume scaling
	float sampleOutput = (float)waveSample / 15.0f; // Normalize sample to range [0.0, 1.0]
	sampleOutput *= waveVolumeScales[volumeScale]; // Apply volume scaling

	const u8& NR51 = core->ram->Get(HWAddr::NR51);
	bool panLeft = GetBits(NR51, NR51Bits::CH3_Left);
	bool panRight = GetBits(NR51, NR51Bits::CH3_Right);

	float leftChannel = panLeft ? sampleOutput : 0.0f;
	float rightChannel = panRight ? sampleOutput : 0.0f;

	samples.push_back(leftChannel);
	samples.push_back(rightChannel);
}

u16 APU::WaveChannel::GetFrequencyTimer()
{
	const u8& periodLow = core->ram->Get(HWAddr::NR33);
	const u8& NR34 = core->ram->Get(HWAddr::NR34);
	const u8 periodHigh = GetBits(NR34, NRx4Bits::PeriodHigh, 0b111);
	int currentFreq = (u16)periodLow | ((u16)periodHigh << 8);
	return (2048 - currentFreq) * waveFrequencyPeriod;
}