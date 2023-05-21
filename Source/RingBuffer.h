#pragma once

#include <JuceHeader.h>

class RingBufferController
{
public:
    RingBufferController();

    struct Block
    {
        int position;
        int count;
    };

    int setRingSize(int numItems);
    int getRingSize() const
    {
        return ringSize;
    }
    void reset(int numStoredItems);
    int getNumItemsStored() const;

    Block getWriteBlock(int numItemsWanted);
    Block getReadBlock(int numItemsWanted, int numItemsAlreadyRead);

    int getReadPosition(int offset) const;
    int getWritePosition() const;
    int getSafeTransferCount(int numItemsWanted, int position) const;
    void advanceReadPosition(int count);
    void advanceWritePosition(int count);

private:
    int readCount = 0;
    int writeCount = 0;
    int ringSize = 0;
};

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
