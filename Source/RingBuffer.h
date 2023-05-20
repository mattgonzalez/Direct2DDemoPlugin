#pragma once

#include <JuceHeader.h>

class RingBuffer
{
public:
    void setSize(int numChannels, int numSamples);
    void reset(int storedSamples);
    void write(juce::AudioBuffer<float> const& source);
    void read(juce::AudioBuffer<float>& destination, int numSamplesToCopy, int ringAdvanceCount);

    int getSize() const
    {
        return buffer.getNumSamples();
    }

    int getReadCount() const
    {
        return readCount;
    }

    int getWriteCount() const
    {
        return writeCount;
    }

    int getNumSamplesStored() const;
    
private:
    int readCount = 0;
    int writeCount = 0;

    juce::AudioBuffer<float> buffer;
};

#if RUN_UNIT_TESTS

class RingBufferTest : public juce::UnitTest
{
public:
    RingBufferTest();

    int getRandomCount(int count);
    void checkRead(juce::AudioBuffer<float> const& source);
    void makeRamp(juce::AudioBuffer<float>& buffer, float startValue);

    void runTest() override;

    RingBuffer ringBuffer;
    juce::Random random;
};

#endif
