#pragma once

#include "Audio/Sound.h"
#include "Audio/SoundTypes.h"

#include "Container/Sequential/List.h"

#include "Concurrency/Mutex.h"
#include "Concurrency/Synchronized.h"

namespace Intra { namespace Audio {

namespace detail {

struct SoundBasicContext
{
	Mutex MyMutex;
	Array<Sound::Data*> AllSounds;
	Array<StreamedSound::Data*> AllStreamedSounds;

	SoundBasicContext() = default;
	SoundBasicContext(const SoundBasicContext&) = delete;
	SoundBasicContext& operator=(const SoundBasicContext&) = delete;
};

struct SoundBasicData
{
	SoundBasicData(const SoundInfo& info): Info(info) {}

	Mutex MyMutex;
	SoundInfo Info;

	SoundBasicData(const SoundBasicData&) = delete;
	SoundBasicData& operator=(const SoundBasicData&) = delete;
};

struct SoundInstanceBasicData
{
	SoundInstanceBasicData(Shared<Sound::Data> parent): Parent(Cpp::Move(parent)) {}

	Mutex MyMutex;
	Shared<Sound::Data> Parent;
	Shared<Sound::Instance::Data> SelfRef;
	Delegate<void()> OnStop;

	SoundInstanceBasicData(const SoundInstanceBasicData&) = delete;
	SoundInstanceBasicData& operator=(const SoundInstanceBasicData&) = delete;
};

struct StreamedSoundBasicData
{
	StreamedSoundBasicData(StreamedSound::SourceRef source, size_t bufferSampleCount):
		Source(Cpp::Move(source)), BufferSampleCount(bufferSampleCount) {}

	size_t GetBufferSize() {return Source->ChannelCount() * BufferSampleCount;}

	StreamedSound::SourceRef Source;
	Shared<StreamedSound::Data> SelfRef;
	size_t BufferSampleCount;
	StreamedSound::OnStopCallback OnStop;

	StreamedSoundBasicData(const StreamedSoundBasicData&) = delete;
	StreamedSoundBasicData& operator=(const StreamedSoundBasicData&) = delete;
};

}

}}

