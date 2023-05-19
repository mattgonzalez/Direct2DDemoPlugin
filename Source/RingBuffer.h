#pragma once

#include <JuceHeader.h>

class RingBuffer
{
public:
    void setSize(int numChannels, int numSamples);
    void reset(int storedSamples);
    void write(juce::AudioBuffer<float> const& source);
    void readWithoutAdvancing(juce::AudioBuffer<float>& destination, int numSamples);
    void readFromPosition(juce::AudioBuffer<float>& destination, int position, int numSamples);
    void advanceReadPosition(int count);
    int getNumSamplesStored() const;
    float getMonoValue(int position) const;
    
    int readCount = 0;
    int writeCount = 0;

    juce::AudioBuffer<float> buffer;
};
