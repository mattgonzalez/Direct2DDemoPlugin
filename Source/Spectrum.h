#pragma once

#include <JuceHeader.h>

template <typename floatType, typename binValueType> class Spectrum
{
public:
    Spectrum()
    {
    }

    Spectrum& operator= (const Spectrum& other) noexcept
    {
        spectrumBuffer = other.spectrumBuffer;
        numNonNegativeFrequencyBins = other.numNonNegativeFrequencyBins;

        return *this;
    }

    Spectrum& withChannels(int numChannels)
    {
        spectrumBuffer.setSize(numChannels, spectrumBuffer.getNumSamples());
        return *this;
    }

    Spectrum& withFFTSize(int fftSize)
    {
        jassert(juce::isPowerOfTwo(fftSize));

        numNonNegativeFrequencyBins = fftSize / 2 + 1;
        spectrumBuffer.setSize(spectrumBuffer.getNumChannels(), numNonNegativeFrequencyBins * numBinDimensions);
        return *this;
    }

    void clear()
    {   
        spectrumBuffer.clear();
    }

    int getNumChannels() const
    {
        return spectrumBuffer.getNumChannels();
    }

    int getNumBins() const // actually the number of bins where the frequency >= 0; same as (FFT size / 2) + 1
    {
        return numNonNegativeFrequencyBins;
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

    void copyFrom(Spectrum<floatType, binValueType> const& source)
    {
        int numChannels = juce::jmin(source.getNumChannels(), spectrumBuffer.getNumSamples());
        int numSamples = juce::jmin(source.getNumBins(), spectrumBuffer.getNumSamples());
        for (int channel = 0; channel < numChannels; ++channel)
        {
            spectrumBuffer.copyFrom(channel, 0, source.getReadPointer(channel), numSamples);
        }
    }

    auto getBinValue(int channel, int bin) const
    {
        return ((const binValueType*)spectrumBuffer.getReadPointer(channel))[bin];
    }

    auto getBinMagnitude(int channel, int bin) const
    {
        return std::abs(getBinValue(channel, bin));
    }

    void setBinValue(int channel, int bin, binValueType binValue)
    {
        ((binValueType*)spectrumBuffer.getWritePointer(channel))[bin] = binValue;
    }

    auto getReadPointer(int channel) const
    {
        return (const binValueType*)spectrumBuffer.getReadPointer(channel);
    }

    auto getWritePointer(int channel)
    {
        return (binValueType*)spectrumBuffer.getWritePointer(channel);
    }

protected:
    juce::AudioBuffer<floatType> spectrumBuffer;
    int numNonNegativeFrequencyBins = 0;
    int const numBinDimensions = sizeof(binValueType) / sizeof(floatType);
};

template <typename floatType> using RealSpectrum = Spectrum<floatType, floatType>;
template <typename floatType> using ComplexSpectrum = Spectrum<floatType, std::complex<floatType>>;

#if RUN_UNIT_TESTS

struct SpectrumTest : public juce::UnitTest
{
    SpectrumTest() :
        UnitTest("SpectrumTest")
    {
    }

    void runTest() override
    {
        {
            TypeTest<float, float> typeTest{ *this };
            typeTest.runTest("real float", 1, 16);
        }

        {
            TypeTest<float, std::complex<float>> typeTest{ *this };
            typeTest.runTest("complex float", 1, 8);
        }
    }

    template <typename floatType, typename binValueType> struct TypeTest
    {
        TypeTest(SpectrumTest& owner_) :
            owner(owner_)
        {
        }

        binValueType makeRampValue(int channel, int index);

        void fillRampA()
        {
            for (int channel = 0; channel < spectrum.getNumChannels(); ++channel)
            {
                for (int bin = 0; bin < spectrum.getNumBins(); ++bin)
                {
                    spectrum.setBinValue(channel, bin, makeRampValue(channel, bin));
                }
            }
        }

        void fillRampB()
        {
            for (int channel = 0; channel < spectrum.getNumChannels(); ++channel)
            {
                auto data = spectrum.getWritePointer(channel);
                for (int bin = 0; bin < spectrum.getNumBins(); ++bin)
                {
                    data[bin] = makeRampValue(channel, bin);
                }
            }
        }

        void verifyClear()
        {
            for (int channel = 0; channel < spectrum.getNumChannels(); ++channel)
            {
                auto channelData = spectrum.getReadPointer(channel);

                for (int index = 0; index < spectrum.getNumBins(); ++index)
                {
                    auto binValue = spectrum.getBinValue(channel, index);
                    owner.expect(binValue == 0.0f);
                    owner.expect(channelData[index] == 0.0f);
                }
            }
        }

        void verifyRampA()
        {
            for (int channel = 0; channel < spectrum.getNumChannels(); ++channel)
            {
                for (int bin = 0; bin < spectrum.getNumBins(); ++bin)
                {
                    auto ramp = makeRampValue(channel, bin);
                    auto binValue = spectrum.getBinValue(channel, bin);
                    owner.expect(ramp == binValue);
                }
            }
        }

        void verifyRampB()
        {
            for (int channel = 0; channel < spectrum.getNumChannels(); ++channel)
            {
                auto data = spectrum.getReadPointer(channel);
                for (int bin = 0; bin < spectrum.getNumBins(); ++bin)
                {
                    auto ramp = makeRampValue(channel, bin);
                    auto binValue = data[bin];
                    owner.expect(ramp == binValue);
                }
            }
        }

        void runTest(juce::StringRef const testName, int numChannels, int fftSize)
        {
            owner.beginTest(testName);

            spectrum = Spectrum<floatType, binValueType>{}.withChannels(numChannels).withFFTSize(fftSize);

            owner.expect(spectrum.getNumChannels() == numChannels);
            owner.expect(spectrum.getNumBins() == fftSize / 2 + 1);

            spectrum.clear();
            verifyClear();

            fillRampA();
            verifyRampA();
            verifyRampB();
            spectrum.clear();

            fillRampB();
            verifyRampA();
            verifyRampB();
        }

        SpectrumTest& owner;
        Spectrum<floatType, binValueType> spectrum;
    };
};

#endif
