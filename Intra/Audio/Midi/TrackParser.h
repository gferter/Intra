#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"
#include "Cpp/Fundamental.h"

#include "Utils/Debug.h"

#include "Range/Polymorphic/InputRange.h"

#include "Messages.h"
#include "RawEvent.h"
#include "DeviceState.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Midi {

struct TrackParser
{
	InputRange<RawEvent> Events;
	double Time = 0;
	uint DelayTicksPassed = 0;

	TrackParser(InputRange<RawEvent> events, double time=0);

	TrackParser(const TrackParser&) = delete;
	TrackParser& operator=(const TrackParser&) = delete;
	TrackParser(TrackParser&&) = default;
	TrackParser& operator=(TrackParser&&) = default;

	double NextEventTime(const DeviceState& state) const
	{
		INTRA_DEBUG_ASSERT(!Events.Empty());
		uint delay = Events.First().Delay();
		if(delay > DelayTicksPassed) delay -= DelayTicksPassed;
		else delay = 0;
		return Time + delay * state.TickDuration;
	}

	void OnTempoChange(double time, double prevTickDuration)
	{
		const uint ticksPassed = uint(Math::Round((time - Time) / prevTickDuration));
		DelayTicksPassed += ticksPassed;
		Time += ticksPassed * prevTickDuration;
	}

	void ProcessEvent(DeviceState& state, IDevice& device);
	forceinline bool Empty() const {return Events.Empty();}

	void processSystemEvent(DeviceState& state, const RawEvent& event);
};

}}}

INTRA_WARNING_POP
