#pragma once

#include "RingBufferController.h"

class AudioRingBuffer
{
public:
    void setSize(int numChannels, int numSamples);
    void reset(int numStoredSamples);
    void write(juce::AudioBuffer<float> const& source);
    void read(juce::AudioBuffer<float>& destination, int numSamplesToCopy, int ringAdvanceCount);

    int getNumSamples() const
    {
        return buffer.getNumSamples();
    }

    int getNumSamplesStored() const
    {
        return ringController.getNumItemsStored();
    }

private:
    RingBufferController ringController;
    juce::AudioBuffer<float> buffer;
};

#if RUN_UNIT_TESTS

class AudioRingBufferTest : public juce::UnitTest
{
public:
    AudioRingBufferTest();

    int getRandomCount(int count);
    void checkRead(juce::AudioBuffer<float> const& source);
    void makeRamp(juce::AudioBuffer<float>& buffer, float startValue);

    void runTest() override;

    AudioRingBuffer ringBuffer;
    juce::Random random;
};

#endif
