#pragma once

#include <JuceHeader.h>

#if RUN_UNIT_TESTS

#include "RingBuffer.h"
#include "Spectrum.h"

struct UnitTests
{
    std::unique_ptr<RingBufferTest> ringBufferTest = std::make_unique<RingBufferTest>();
    std::unique_ptr<SpectrumTest> realFloatSpectrumTest = std::make_unique<SpectrumTest>();
};

#endif
