#pragma once

#include <JuceHeader.h>

#if RUN_UNIT_TESTS

#include "AudioRingBuffer.h"
#include "Spectrum.h"

struct UnitTests
{
    std::unique_ptr<AudioRingBufferTest> ringBufferTest = std::make_unique<AudioRingBufferTest>();
    std::unique_ptr<SpectrumTest> realFloatSpectrumTest = std::make_unique<SpectrumTest>();
};

#endif
