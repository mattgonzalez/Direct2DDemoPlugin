/*

Copyright(c) 2023 Matthew Gonzalez

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#pragma once

#include "FIFOController.h"

class AudioFIFO
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
    FIFOController ringController;
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

    AudioFIFO ringBuffer;
    juce::Random random;
};

#endif
