﻿#pragma once

#include "Range/Generators/Span.h"
#include "Audio/Synth/Types.h"
#include "Range/Generators/Span.h"

namespace Intra { namespace Audio { namespace Synth {

namespace D {

template<typename T> void ModifierPassFunction(const T& modifier,
	float freq, Span<float> inOutSamples, uint sampleRate)
{
	auto modifierCopy = modifier;
	float t = 0.0f, dt = 1.0f/float(sampleRate);
	modifierCopy.SetParams(freq, dt);
	for(auto& s: inOutSamples)
	{
		s = modifierCopy.NextSample(s);
		t += dt;
	}
}

}

template<typename T> ModifierPass CreateModifierPass(T modifier)
{return ModifierPass(D::ModifierPassFunction<T>, modifier);}

}}}
