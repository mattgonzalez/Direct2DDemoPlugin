#include "Spectrum.h"

#if RUN_UNIT_TESTS
template <>
float SpectrumTest::TypeTest<float, float>::makeRampValue(int channel, int index)
{
    return (channel + 1) * 10000000.0f + index;
}

template <>
std::complex<float> SpectrumTest::TypeTest<float, std::complex<float>>::makeRampValue(int channel, int index)
{
    float radius = (channel + 1) * 10000000.0f + index;
    return std::polar(radius, index * juce::MathConstants<float>::twoPi * 0.01f);
}
#endif
