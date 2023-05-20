#pragma once

#include <JuceHeader.h>

template <typename floatType> class Spectrum
{
public:
    Spectrum()
    {
    }

    Spectrum(int numChannels, int numNonNegativeFrequencyBins) :
        spectrumBuffer(numChannels, numNonNegativeFrequencyBins)
    {
    }

    Spectrum& operator= (const Spectrum& other) noexcept
    {
        spectrumBuffer = other.spectrumBuffer;
        return *this;
    }

    Spectrum& withChannels(int numChannels)
    {
        spectrumBuffer.setSize(numChannels, spectrumBuffer.getNumSamples());
        return *this;
    }

    Spectrum& withFFTSize(int fftSize)
    {
        spectrumBuffer.setSize(spectrumBuffer.getNumChannels(), fftSize / 2 + 1);
        return *this;
    }

    void clear();

    int getNumChannels() const
    {
        return spectrumBuffer.getNumChannels();
    }

    int getNumBins() const // actually the number of bins where the frequency >= 0; same as (FFT size / 2) + 1
    {
        return spectrumBuffer.getNumSamples();
    }

    void copyFrom(juce::AudioBuffer<floatType> const& source, int numSamples)
    {
        int numChannels = juce::jmin(source.getNumChannels(), spectrumBuffer.getNumSamples());
        numSamples = juce::jmin(numSamples, spectrumBuffer.getNumSamples());
        for (int channel = 0; channel < numChannels; ++channel)
        {
            spectrumBuffer.copyFrom(channel, 0, source.getReadPointer(channel), numSamples);
        }
    }

    void copyFrom(juce::AudioBuffer<floatType> const& source, int numSamples, float gain)
    {
        int numChannels = juce::jmin(source.getNumChannels(), spectrumBuffer.getNumSamples());
        numSamples = juce::jmin(numSamples, spectrumBuffer.getNumSamples());
        for (int channel = 0; channel < numChannels; ++channel)
        {
            spectrumBuffer.copyFrom(channel, 0, source.getReadPointer(channel), numSamples, gain);
        }
    }

    void copyFrom(Spectrum<floatType> const& source)
    {
        int numChannels = juce::jmin(source.getNumChannels(), spectrumBuffer.getNumSamples());
        int numSamples = juce::jmin(source.getNumBins(), spectrumBuffer.getNumSamples());
        for (int channel = 0; channel < numChannels; ++channel)
        {
            spectrumBuffer.copyFrom(channel, 0, source.getReadPointer(channel), numSamples);
        }
    }

    floatType getBinMagnitude(int channel, int bin) const;

    auto getReadPointer(int channel) const
    {
        return spectrumBuffer.getReadPointer(channel);
    }

    auto getWritePointer(int channel)
    {
        return getWritePointer(channel);
    }


#if 0
    template<> float getBinMagnitude<float>(int channel, int bin) const
    {
        return spectrumBuffer.getSample(channel, bin);
    }

    template<> double getBinMagnitude<double>(int channel, int bin) const
    {
        return spectrumBuffer.getSample(channel, bin);
    }
#endif

protected:
    juce::AudioBuffer<floatType> spectrumBuffer;
};

