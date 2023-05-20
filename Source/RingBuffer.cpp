#include "RingBuffer.h"

void RingBuffer::setSize(int numChannels, int numSamples)
{
    numSamples = juce::nextPowerOfTwo(numSamples);
    buffer.setSize(numChannels, numSamples, false, true, true);
}

void RingBuffer::reset(int storedSamples)
{
    buffer.clear();
    readCount = 0;
    writeCount = storedSamples;
}

void RingBuffer::write(juce::AudioBuffer<float> const& source)
{
    int numChannels = juce::jmin(buffer.getNumChannels(), source.getNumChannels());
    int samplesRemaining = source.getNumSamples();
    int sourceIndex = 0;
    int bufferMask = buffer.getNumSamples() - 1;
    int destinationIndex = writeCount & bufferMask;
    while (samplesRemaining > 0)
    {
        int copyCount = juce::jmin(samplesRemaining, source.getNumSamples() - sourceIndex);
        copyCount = juce::jmin(copyCount, buffer.getNumSamples() - destinationIndex);
        if (copyCount <= 0)
        {
            break;
        }

        for (int channel = 0; channel < numChannels; ++channel)
        {
            buffer.copyFrom(channel, destinationIndex, source, channel, sourceIndex, copyCount);
        }

        destinationIndex = (destinationIndex + copyCount) & bufferMask;
        sourceIndex += copyCount;
        samplesRemaining -= copyCount;
    }

    writeCount += source.getNumSamples();
}

void RingBuffer::read(juce::AudioBuffer<float>& destination, int numSamplesToCopy, int ringAdvanceCount)
{
    jassert(destination.getNumSamples() >= numSamplesToCopy);

    int numChannels = juce::jmin(buffer.getNumChannels(), destination.getNumChannels());
    int samplesRemaining = numSamplesToCopy;
    int destinationIndex = 0;
    int bufferMask = buffer.getNumSamples() - 1;
    int sourceIndex = readCount & bufferMask;
    while (samplesRemaining > 0)
    {
        int copyCount = juce::jmin(samplesRemaining, destination.getNumSamples() - destinationIndex);
        copyCount = juce::jmin(copyCount, buffer.getNumSamples() - sourceIndex);
        if (copyCount <= 0)
        {
            break;
        }

        for (int channel = 0; channel < numChannels; ++channel)
        {
            destination.copyFrom(channel, destinationIndex, buffer, channel, sourceIndex, copyCount);
        }

        sourceIndex = (sourceIndex + copyCount) & bufferMask;
        destinationIndex += copyCount;
        samplesRemaining -= copyCount;
    }

    readCount += ringAdvanceCount;
}

int RingBuffer::getNumSamplesStored() const
{
    return (writeCount - readCount) & (buffer.getNumSamples() - 1);
}

#if RUN_UNIT_TESTS

RingBufferTest::RingBufferTest() : UnitTest("RingBufferTest")
{
}

int RingBufferTest::getRandomCount(int count)
{
    return (count > 2) ? random.nextInt(juce::Range<int>(1, count)) : count;
}

void RingBufferTest::checkRead(juce::AudioBuffer<float> const& source)
{
    juce::AudioBuffer<float> destination{ source.getNumChannels(), source.getNumSamples() };

    int samplesRemaining = source.getNumSamples();
    int sourceIndex = 0;
    while (samplesRemaining > 0)
    {
        int copyCount = getRandomCount(samplesRemaining);
        int advance = getRandomCount(copyCount);

        destination.clear();

        int previousReadPosition = ringBuffer.getReadCount();
        ringBuffer.read(destination, copyCount, advance);
        expect(ringBuffer.getReadCount() == previousReadPosition + advance);

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

void RingBufferTest::makeRamp(juce::AudioBuffer<float>& buffer, float startValue)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        for (int index = 0; index < buffer.getNumSamples(); ++index)
        {
            buffer.setSample(channel, index, channel * 1000.0f + startValue + (float)index);
        }
    }
}

void RingBufferTest::runTest()
{
    beginTest("RingBuffer");

    int minimumRingSamples = 30;
    ringBuffer.setSize(2, minimumRingSamples);

    expect(juce::isPowerOfTwo(ringBuffer.getSize()) && ringBuffer.getSize() >= minimumRingSamples);

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
