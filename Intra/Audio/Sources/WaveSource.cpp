﻿#include "Audio/Sources/WaveSource.h"

#include "Math/Math.h"

#include "Cpp/Intrinsics.h"
#include "Cpp/Warnings.h"
#include "Cpp/Endianess.h"


INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Sources {

#ifndef INTRA_NO_WAVE_LOADER

struct WaveHeader
{
	char RIFF[4];
	uintLE WaveformChunkSize;
	char WAVE[4];

	char fmt[4];
	uintLE FormatChunkSize;

	ushortLE FormatTag, Channels;
	uintLE SampleRate, BytesPerSec;
	ushortLE BlockAlign, BitsPerSample;
	char data[4];
	uintLE DataSize;
};


WaveSource::WaveSource(CSpan<byte> srcFileData):
	mData(null), mSampleCount(0), mCurrentDataPos(0)
{
	const WaveHeader& header = *reinterpret_cast<const WaveHeader*>(srcFileData.Begin);

	const bool isValidHeader = (C::memcmp(header.RIFF, "RIFF", sizeof(header.RIFF))==0 &&
		C::memcmp(header.WAVE, "WAVE", sizeof(header.WAVE))==0 &&
		C::memcmp(header.fmt, "fmt ", sizeof(header.fmt))==0 &&
		C::memcmp(header.data, "data", sizeof(header.data))==0);
	
	if(!isValidHeader) return;

	mData = srcFileData.Drop(sizeof(WaveHeader)).Reinterpret<const short>().Take(header.DataSize/sizeof(short));
	mChannelCount = ushort(header.Channels);
	mSampleRate = header.SampleRate;
	mSampleCount = header.DataSize/sizeof(short)/mChannelCount;
}

size_t WaveSource::GetInterleavedSamples(Span<short> outShorts)
{
	INTRA_DEBUG_ASSERT(!outShorts.Empty());
	const auto shortsToRead = Math::Min(outShorts.Length(), mSampleCount*mChannelCount-mCurrentDataPos);
	CopyTo(mData.Drop(mCurrentDataPos).Take(shortsToRead), outShorts);
	mCurrentDataPos += shortsToRead;
	if(shortsToRead<outShorts.Length()) mCurrentDataPos = 0;
	return shortsToRead/mChannelCount;
}

size_t WaveSource::GetInterleavedSamples(Span<float> outFloats)
{
	INTRA_DEBUG_ASSERT(!outFloats.Empty());
	Array<short> outShorts;
	outShorts.SetCountUninitialized(outFloats.Length());
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0; i<outFloats.Length(); i++)
		outFloats[i] = (outShorts[i] + 0.5f) / 32767.5f;
	return result;
}

size_t WaveSource::GetUninterleavedSamples(CSpan<Span<float>> outFloats)
{
	INTRA_DEBUG_ASSERT(outFloats.Length()==mChannelCount);
	const size_t outSamplesCount = outFloats.First().Length();
	for(size_t i=1; i<mChannelCount; i++)
	{
		INTRA_DEBUG_ASSERT(outFloats[i].Length()==outSamplesCount);
	}

	Array<short> outShorts;
	outShorts.SetCountUninitialized(outSamplesCount*mChannelCount);
	auto result = GetInterleavedSamples(outShorts);
	for(size_t i=0, j=0; i<outShorts.Count(); i++)
	{
		for(ushort c=0; c<mChannelCount; c++)
			outFloats[c][i] = (outShorts[j++] + 0.5f)/32767.5f;
	}
	return result;
}

FixedArray<const void*> WaveSource::GetRawSamplesData(size_t maxSamplesToRead,
	Data::ValueType* oType, bool* oInterleaved, size_t* oSamplesRead)
{
	const auto shortsToRead = Math::Min(maxSamplesToRead, mSampleCount*mChannelCount - mCurrentDataPos);
	if(oSamplesRead) *oSamplesRead = shortsToRead/mChannelCount;
	if(oInterleaved) *oInterleaved = true;
	if(oType) *oType = Data::ValueType::Short;
	FixedArray<const void*> resultPtrs = {mData.Begin + mCurrentDataPos};
	mCurrentDataPos += shortsToRead;
	if(shortsToRead<maxSamplesToRead) mCurrentDataPos = 0;
	return resultPtrs;
}

#else

INTRA_DISABLE_LNK4221

#endif

}}}

INTRA_WARNING_POP