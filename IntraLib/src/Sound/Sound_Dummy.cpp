﻿#include "Core/Core.h"

#if(INTRA_LIBRARY_SOUND_SYSTEM==INTRA_LIBRARY_SOUND_SYSTEM_Dummy)

#include "Sound/SoundApi.h"

namespace Intra {

using namespace Math;

namespace SoundAPI {

const ValueType::I InternalBufferType = ValueType::Void;
const int InternalChannelsInterleaved = true;

struct Buffer
{
	Buffer(uint sample_count, uint sample_rate, uint ch):
		sampleCount(sample_count), sampleRate(sample_rate), channels(ch) {}

	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	uint sampleCount;
	uint sampleRate;
	uint channels;

	void* locked_bits=null;
};

struct Instance
{
	Instance(BufferHandle myParent): parent(myParent) {}

	BufferHandle parent;

	bool deleteOnStop=false;
};


struct StreamedBuffer
{
	//StreamedBuffer(uint bufs[2], StreamingCallback callback, uint sample_count, uint sample_rate, ushort ch):
		//buffers{bufs[0], bufs[1]}, streamingCallback(callback), sampleCount(sampleCount), sampleRate(sample_rate), channels(ch) {}

	uint SizeInBytes() const {return uint(sampleCount*channels*sizeof(short));}

	uint sampleCount; //Размер половины буфера
	uint sampleRate;
	uint channels;
	bool looping=false;
};

BufferHandle BufferCreate(size_t sampleCount, uint channels, uint sampleRate)
{
	return new Buffer(uint(sampleCount), sampleRate, channels);
}

void BufferSetDataInterleaved(BufferHandle snd, const void* data, ValueType type)
{
	INTRA_ASSERT(data!=null);
	(void)data; (void)snd; (void)type;
}

void* BufferLock(BufferHandle snd)
{
	INTRA_ASSERT(snd!=null);
    snd->locked_bits = Memory::SystemHeapAllocator::Allocate(snd->SizeInBytes(), INTRA_SOURCE_INFO);
	return snd->locked_bits;
}

void BufferUnlock(BufferHandle snd)
{
	INTRA_ASSERT(snd!=null);
    Memory::SystemHeapAllocator::Free(snd->locked_bits);
    snd->locked_bits=null;
}

void BufferDelete(BufferHandle snd)
{
	if(snd==null) return;
	delete snd;
}

InstanceHandle InstanceCreate(BufferHandle snd)
{
	INTRA_ASSERT(snd!=null);
	return new Instance(snd);
}

void InstanceSetDeleteOnStop(InstanceHandle instance, bool del)
{
	if(del) delete instance;
}

void InstanceDelete(InstanceHandle instance)
{
	delete instance;
}

void InstancePlay(InstanceHandle instance, bool loop)
{
	INTRA_ASSERT(instance!=null);
	(void)instance; (void)loop;
}

bool InstanceIsPlaying(InstanceHandle instance)
{
	(void)instance;
	return false;
}

void InstanceStop(InstanceHandle instance)
{
	(void)instance;
}

StreamedBufferHandle StreamedBufferCreate(size_t sampleCount,
	uint channels, uint sampleRate, StreamingCallback callback)
{
	if(sampleCount==0 || channels==0 ||
		sampleRate==0 || callback.CallbackFunction==null)
			return null;
	StreamedBufferHandle result = new StreamedBuffer;
	result->sampleCount = uint(sampleCount);
	result->sampleRate = sampleRate;
	result->channels = channels;
	return result;
}

void StreamedBufferSetDeleteOnStop(StreamedBufferHandle snd, bool del)
{
	if(del) delete snd;
}

void StreamedBufferDelete(StreamedBufferHandle snd)
{
	(void)snd;
}

void StreamedSoundPlay(StreamedBufferHandle snd, bool loop)
{
	INTRA_ASSERT(snd!=null);
	snd->looping = loop;
}

bool StreamedSoundIsPlaying(StreamedBufferHandle si)
{
	(void)si;
	return false;
}

void StreamedSoundStop(StreamedBufferHandle snd)
{
	(void)snd;
}

void StreamedSoundUpdate(StreamedBufferHandle snd)
{
	(void)snd;
}

}}

#endif
