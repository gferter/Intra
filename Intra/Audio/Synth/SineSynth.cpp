﻿#include "Audio/Synth/SineSynth.h"
#include "Audio/Synth/PeriodicSynth.h"

#include "Cpp/Warnings.h"

#include "Math/SineRange.h"

#include "Utils/Span.h"
#include "Funal/Bind.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Transform.h"
#include "Range/Mutation/Fill.h"

#include "Random/FastUniform.h"

#include "Container/Sequential/Array.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

struct SineParams
{
	float scale;
	ushort harmonics;
	float freqMultiplyer;
};

void PerfectSine(float volume, float freq, uint sampleRate, Span<float> inOutSamples, bool add)
{
	Math::SineRange<float> sineRange(volume, 0, float(2*Math::PI*freq/sampleRate));
	if(!add) ReadWrite(sineRange, inOutSamples.Length(), inOutSamples);
	else Add(inOutSamples, sineRange);
}

void FastSine(float volume, float freq, uint sampleRate, Span<float> inOutSamples, bool add)
{
	const double samplesPerPeriod = float(sampleRate)/freq;
	const uint count = GetGoodSignalPeriod(samplesPerPeriod, Math::Max(uint(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	Array<float> sineFragment;
	const uint sampleCount = uint(Math::Round(samplesPerPeriod*count));
	sineFragment.SetCountUninitialized(sampleCount);
	PerfectSine(volume, freq, sampleRate, sineFragment, false);

	RepeatFragmentInBuffer(sineFragment, inOutSamples, add);
}

static void SineSynthPassFunction(const SineParams& params,
	float freq, float volume, Span<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples == null) return;
	const float newFreq = freq*params.freqMultiplyer;
	const float maxValue = 2.0f-2.0f/float(1 << params.harmonics);
	float newVolume = volume*params.scale/maxValue;

	FastSine(newVolume, newFreq, sampleRate, inOutSamples, add);

	Random::FastUniform<float> frandom(1278328923);
	for(ushort h=1; h<params.harmonics; h++)
	{
		newVolume /= 2;
		float frequency = newFreq*float(1 << h);
		frequency += frandom()*frequency*0.002f;
		FastSine(newVolume, frequency, sampleRate, inOutSamples.Drop(uint(frandom(20))), true);
	}
}

SynthPass CreateSineSynthPass(float scale, ushort harmonics, float freqMultiplyer)
{return Funal::Bind(SineSynthPassFunction, SineParams{scale, harmonics, freqMultiplyer});}


struct MultiSineParams
{
	byte Len;
	SineHarmonic Harmonics[20];
};

static void MultiSineSynthPassFunction(const MultiSineParams& params,
		float freq, float volume, Span<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	auto rand = Random::FastUniform<ushort>(uint((inOutSamples.Length() << 5) | params.Len));
	const size_t start = rand(20);
	if(!add) FillZeros(inOutSamples.Take(start));
	for(ushort h = 0; h < params.Len; h++)
	{
		auto& harm = params.Harmonics[h];
		FastSine(volume*float(harm.Scale),
			freq*float(harm.FreqMultiplyer), sampleRate,
			inOutSamples,
			add || h>0);
	}
}

SynthPass CreateMultiSineSynthPass(CSpan<SineHarmonic> harmonics)
{
	MultiSineParams params;
	const auto src = harmonics.Take(Concepts::LengthOf(params.Harmonics));
	params.Len = byte(src.Length());
	CopyTo(src, params.Harmonics);
	return Funal::Bind(MultiSineSynthPassFunction, params);
}

}}}

INTRA_WARNING_POP
