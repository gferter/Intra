#include "WaveTableGeneration.h"
#include "Audio/AudioProcessing.h"

#include "Random/FastUniform.h"

#include "Math/SineRange.h"

#include "Range/Mutation/Copy.h"
#include "Range/Mutation/Transform.h"
#include "Range/Reduction.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

void AddSineHarmonicGaussianProfile(Span<float> wavetableAmplitudes, float freqSampleRateRatio,
	float harmFreqMultiplier, float harmBandwidthScale, float amplitude, float bandwidthCents)
{
	const size_t N = wavetableAmplitudes.Length()*2;
	const float bwi = (Math::Pow2(bandwidthCents/1200 - 1) - 0.5f)*freqSampleRateRatio*Math::Pow(harmFreqMultiplier, harmBandwidthScale);
	float rw = -freqSampleRateRatio*harmFreqMultiplier/bwi;
	const float rdw = 1.0f/(float(N)*bwi);
	float range = 2;
	if(rdw > 1) range = 3*rdw;
	if(-range > rw)
	{
		float elementsToSkip = Math::Round((-range - rw) / rdw);
		wavetableAmplitudes.PopFirstN(size_t(elementsToSkip));
		rw += elementsToSkip*rdw;
	}
	if(rw < range) wavetableAmplitudes = wavetableAmplitudes.Take(size_t(Math::Round((range - rw) / rdw)));
	amplitude /= bwi;
	while(!wavetableAmplitudes.Empty())
	{
		wavetableAmplitudes.Next() += amplitude * Math::Exp(-Math::Sqr(rw));
		rw += rdw;
	}
}


static void GenerateRandomPhases(Span<float> inOutRealAmplitudes, Span<float> outImagAmplitudes)
{
	INTRA_DEBUG_ASSERT(inOutRealAmplitudes.Length() <= outImagAmplitudes.Length());
	Random::FastUniform<ushort> rand(uint(inOutRealAmplitudes.Length()^1633529523u));
	enum: uint {TABLE_SIZE = 1024};
	float sineTable[TABLE_SIZE];
	Math::SineRange<float> oscillator(1, 0, float(2*Math::PI/TABLE_SIZE));
	ReadTo(oscillator, sineTable);
	inOutRealAmplitudes.PopFirst();
	outImagAmplitudes.PopFirst();
	while(!inOutRealAmplitudes.Empty())
	{
		float& real = inOutRealAmplitudes.Next();
		float& imag = outImagAmplitudes.Next();
		const size_t phaseIndex = rand(TABLE_SIZE);
		const float cosine = sineTable[(phaseIndex + TABLE_SIZE/4) & (TABLE_SIZE-1)];
		const float sine = sineTable[phaseIndex];
		imag = real*sine;
		real *= cosine;
	}
}

static void ReflectComplexAmplitudes(Span<float> inOutRealAmplitudesX2, Span<float> inOutImagAmplitudesX2)
{
	inOutImagAmplitudesX2[inOutImagAmplitudesX2.Length() / 2] = 0;
	inOutRealAmplitudesX2.PopFirst();
	inOutImagAmplitudesX2.PopFirst();
	while(!inOutRealAmplitudesX2.Empty())
	{
		inOutRealAmplitudesX2.Last() = inOutRealAmplitudesX2.First();
		inOutImagAmplitudesX2.Last() = -inOutImagAmplitudesX2.First();
		inOutRealAmplitudesX2.PopFirst();
		inOutImagAmplitudesX2.PopFirst();
		if(inOutRealAmplitudesX2.Empty()) break;
		inOutRealAmplitudesX2.PopLast();
		inOutImagAmplitudesX2.PopLast();
	}
}

static void NormalizeSamples(Span<float> inOutSamples, float volume = 1)
{
	const auto minimax = MiniMax(inOutSamples.AsConstRange());
	const float multiplier = 2 / (minimax.second - minimax.first);
	const float add = -minimax.first*multiplier - 1;
	MulAdd(inOutSamples, multiplier*volume, add*volume);
}

void ConvertAmplutudesToSamples(Span<float> inAmplitudesX2OutSamples, Span<float> tempBuffer, float volume)
{
	INTRA_DEBUG_ASSERT(tempBuffer.Length() >= inAmplitudesX2OutSamples.Length());
	INTRA_DEBUG_ASSERT(inAmplitudesX2OutSamples.Length() % 2 == 0);
	GenerateRandomPhases(
		inAmplitudesX2OutSamples.Take(inAmplitudesX2OutSamples.Length() / 2),
		tempBuffer.Take(inAmplitudesX2OutSamples.Length() / 2));
	ReflectComplexAmplitudes(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
	InplaceInverseFFTNonNormalized(inAmplitudesX2OutSamples, tempBuffer.Take(inAmplitudesX2OutSamples.Length()));
	NormalizeSamples(inAmplitudesX2OutSamples, volume);
}

void ConvertAmplitudesToSamples(WaveTable& table, float volume, bool genMipmaps)
{
	INTRA_DEBUG_ASSERT(table.LevelCount == 1);
	INTRA_DEBUG_ASSERT(table.Data.Length() == table.BaseLevelLength/2); //Data �������� �� ������, � ��������� ������, ������� � 2 ���� ������
	table.Data.SetCount(table.BaseLevelLength * 2); //� ������ �������� ����� ��������� ������, � ������ ����� ��������� ������� ��� ���������
	auto amplitudesThenSamples = table.Data.Take(table.BaseLevelLength); //�������� ���� ����� ������ �������� �������� ������, ������������ �������� ���� ��������� ������
	auto tempBuffer = table.Data.Drop(table.BaseLevelLength);
	ConvertAmplutudesToSamples(amplitudesThenSamples, tempBuffer, volume);
	table.Data.SetCount(table.BaseLevelLength); //��������� ����� �������. �����, ���������� �����������������, ����� �������������� ��� �������� ������� �����������
	if(genMipmaps) table.GenerateAllNextLevels();
	else table.Data.TrimExcessCapacity();
}

WaveTableCache CreateWaveTablesFromHarmonics(CSpan<SineHarmonicWithBandwidthDesc> harmonics,
	float bandwidthScale, size_t tableSize, bool allowMipmaps)
{
	WaveTableCache result;
	Array<SineHarmonicWithBandwidthDesc> harmonicsArr = harmonics;
	result.Generator = [harmonicsArr, bandwidthScale, tableSize, allowMipmaps](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(auto& harm: harmonicsArr)
		{
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio,
				harm.FreqMultiplier, bandwidthScale, harm.Amplitude, harm.Bandwidth);
		}
		ConvertAmplitudesToSamples(tbl, 1, allowMipmaps);
		return tbl;
	};
	result.AllowMipmaps = allowMipmaps;
	return result;
}

Array<SineHarmonicWithBandwidthDesc> CreateHarmonicArray(float bandwidth, float bandwidthStep,
	float harmonicAttenuationPower, float freqMultStep, size_t numHarmonics, bool alternatingSigns)
{
	Array<SineHarmonicWithBandwidthDesc> harmonics;
	harmonics.Reserve(numHarmonics);
	float freqMult = 1;
	float sign = 1;
	while(numHarmonics --> 0)
	{
		auto& harm = harmonics.EmplaceLast();
		harm.Amplitude = sign*Math::Pow(freqMult, -harmonicAttenuationPower);
		harm.FreqMultiplier = freqMult;
		harm.Bandwidth = bandwidth;
		freqMult += freqMultStep;
		bandwidth += bandwidthStep;
		if(alternatingSigns) sign = -sign;
	}
	return harmonics;
}


WaveTableCache CreateWaveTablesFromFormants(CSpan<FormantDesc> formants, uint numHarmonics,
	float harmonicAttenuationPower, float bandwidth, float bandwidthScale, size_t tableSize)
{
	WaveTableCache result;
	Array<FormantDesc> formantsArr = formants;
	result.Generator = [=](float freq, uint sampleRate)
	{
		WaveTable tbl;
		tbl.BaseLevelLength = tableSize;
		tbl.Data.Reserve(tbl.BaseLevelLength * 2);
		tbl.Data.SetCount(tbl.BaseLevelLength / 2);
		tbl.BaseLevelRatio = freq/float(sampleRate);
		for(size_t i = 1; i <= numHarmonics; i++)
		{
			const float ifreq = float(i)*freq;
			float amplitude = 0;
			for(auto& formant: formantsArr) amplitude += formant.Scale*Math::Exp(-Math::Sqr((ifreq - formant.Frequency) * formant.Coeff));
			amplitude /= Math::Pow(float(i), harmonicAttenuationPower);
			Synth::AddSineHarmonicGaussianProfile(tbl.Data, tbl.BaseLevelRatio,
				float(i), bandwidthScale, amplitude, bandwidth);
		}
		ConvertAmplitudesToSamples(tbl, 1, false);
		return tbl;
	};
	result.AllowMipmaps = false;
	return result;
}



}}}

INTRA_WARNING_POP
