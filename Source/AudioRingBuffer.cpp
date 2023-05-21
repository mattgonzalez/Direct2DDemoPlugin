#include "AudioRingBuffer.h"

void AudioRingBuffer::setSize(int numChannels, int numSamples)
{
    int ringSize = ringController.setRingSize(numSamples);
    buffer.setSize(numChannels, ringSize, false, false, true);
    buffer.clear();
}

void AudioRingBuffer::reset(int numStoredSamples)
{
    ringController.reset(numStoredSamples);
    buffer.clear();
}

void AudioRingBuffer::write(juce::AudioBuffer<float> const& source)
{
    int numChannels = juce::jmin(buffer.getNumChannels(), source.getNumChannels());
    int samplesRemaining = source.getNumSamples();
    int sourceIndex = 0;

    while (samplesRemaining > 0)
    {
        auto block = ringController.getWriteBlock(samplesRemaining);
        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.copyFrom(channel, block.position,
                source,
                channel, sourceIndex, block.count);
        }

        samplesRemaining -= block.count;
        sourceIndex += block.count;
        ringController.advanceWritePosition(block.count);
    }
}

void AudioRingBuffer::read(juce::AudioBuffer<float>& destination, int numSamplesToCopy, int ringAdvanceCount)
{
    int numChannels = juce::jmin(buffer.getNumChannels(), destination.getNumChannels());
    int destinationIndex = 0;
    int numSamplesCopied = 0;

    while (numSamplesCopied < numSamplesToCopy)
    {
        auto block = ringController.getReadBlock(numSamplesToCopy - numSamplesCopied, numSamplesCopied);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            destination.copyFrom(channel, destinationIndex,
                buffer,
                channel, block.position, block.count);
        }

        numSamplesToCopy -= block.count;
        destinationIndex += block.count;
        numSamplesCopied += block.count;
    }

    ringController.advanceReadPosition(ringAdvanceCount);
}

#if RUN_UNIT_TESTS

AudioRingBufferTest::AudioRingBufferTest() : UnitTest("RingBufferTest")
{
}

int AudioRingBufferTest::getRandomCount(int count)
{
    return (count > 2) ? random.nextInt(juce::Range<int>(1, count)) : count;
}

void AudioRingBufferTest::checkRead(juce::AudioBuffer<float> const& source)
{
    juce::AudioBuffer<float> destination{ source.getNumChannels(), source.getNumSamples() };

    int samplesRemaining = source.getNumSamples();
    int sourceIndex = 0;
    while (samplesRemaining > 0)
    {
        int copyCount = getRandomCount(samplesRemaining);
        int advance = getRandomCount(copyCount);

        destination.clear();

        ringBuffer.read(destination, copyCount, advance);

        for (int channel = 0; channel < destination.getNumChannels(); ++channel)
        {
            for (int index = 0; index < copyCount; ++index)
            {
                float sample = destination.getSample(channel, index);
                float expectedValue = source.getSample(channel, index + sourceIndex);

                expect(sample == expectedValue);
            }
        }

        sourceIndex += advance;
        samplesRemaining -= advance;
    }
}

void AudioRingBufferTest::makeRamp(juce::AudioBuffer<float>& buffer, float startValue)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int index = 0; index < buffer.getNumSamples(); ++index)
        {
            buffer.setSample(channel, index, channel * 1000.0f + startValue + (float)index);
        }
    }
}

void AudioRingBufferTest::runTest()
{
    beginTest("AudioRingBufferTest");

    int minimumRingSamples = 30;
    ringBuffer.setSize(2, minimumRingSamples);

    expect(juce::isPowerOfTwo(ringBuffer.getNumSamplesStored()) && ringBuffer.getNumSamples() >= minimumRingSamples);

    juce::AudioBuffer<float> source1{ 2, minimumRingSamples / 2 };
    juce::AudioBuffer<float> source2{ 2, minimumRingSamples / 2 };

    makeRamp(source1, 1000000.0f);
    ringBuffer.write(source1);

    makeRamp(source2, 2000000.0f);
    ringBuffer.write(source2);

    checkRead(source1);
    checkRead(source2);
}

#endif
