#include "Spectrum.h"

template<> float Spectrum<float>::getBinMagnitude(int channel, int bin) const
{
    return spectrumBuffer.getSample(channel, bin);
}

template<> void Spectrum<float>::clear()
{
    spectrumBuffer.clear();
}

template<> void Spectrum<double>::clear()
{
    spectrumBuffer.clear();
}

template<> double Spectrum<double>::getBinMagnitude(int channel, int bin) const
{
    return spectrumBuffer.getSample(channel, bin);
}

template<> void Spectrum<std::complex<float>>::clear()
{
    for (int channel = 0; channel < spectrumBuffer.getNumChannels(); ++channel)
    {
        memset(spectrumBuffer.getWritePointer(channel), 0, spectrumBuffer.getNumSamples() * sizeof(std::complex<float>));
    }
}

template<> std::complex<double> Spectrum<std::complex<double>>::getBinMagnitude(int channel, int bin) const
{
    return std::abs(spectrumBuffer.getSample(channel, bin));
}

template<> std::complex<float> Spectrum<std::complex<float>>::getBinMagnitude(int channel, int bin) const
{
    return std::abs(spectrumBuffer.getSample(channel, bin));
}
