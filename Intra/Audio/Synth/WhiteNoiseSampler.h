﻿#pragma once

#include "Cpp/Warnings.h"

#include "Utils/Span.h"

#include "Random/FastUniformNoise.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

class WhiteNoiseSampler
{
	float mT, mDT;
	float mAmplitude;
public:
	WhiteNoiseSampler(float dt, float amplitude):
		mT(0), mDT(dt), mAmplitude(amplitude) {}

	Span<float> operator()(Span<float> inOutSamples, bool add);
};

struct WhiteNoiseInstrument
{
	float VolumeScale;
	float FreqMultiplier;

	forceinline WhiteNoiseInstrument(null_t=null): VolumeScale(0), FreqMultiplier(1) {}
	forceinline WhiteNoiseInstrument(float volumeScale, float freqMult = 1):
		VolumeScale(volumeScale), FreqMultiplier(freqMult) {}

	WhiteNoiseSampler operator()(float freq, float volume, uint sampleRate) const
	{return WhiteNoiseSampler(freq*FreqMultiplier/float(sampleRate), volume*VolumeScale);}

	forceinline explicit operator bool() const noexcept {return VolumeScale > 0;}
	forceinline bool operator==(null_t) const noexcept {return VolumeScale <= 0;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
};

}}}

INTRA_WARNING_POP
