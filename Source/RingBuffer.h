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
