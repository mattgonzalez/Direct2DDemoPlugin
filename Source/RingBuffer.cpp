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

void RingBuffer::readWithoutAdvancing(juce::AudioBuffer<float>& destination, int numSamples)
{
    int numChannels = juce::jmin(buffer.getNumChannels(), destination.getNumChannels());
    int samplesRemaining = numSamples;
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
}

void RingBuffer::readFromPosition(juce::AudioBuffer<float>& destination, int position, int numSamples)
{
    int numChannels = juce::jmin(buffer.getNumChannels(), destination.getNumChannels());
    int samplesRemaining = numSamples;
    int destinationIndex = 0;
    int bufferMask = buffer.getNumSamples() - 1;
    int sourceIndex = position & bufferMask;
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
}

void RingBuffer::advanceReadPosition(int count)
{
    readCount += count;
}

int RingBuffer::getNumSamplesStored() const
{
    return (writeCount - readCount) & (buffer.getNumSamples() - 1);
}

float RingBuffer::getMonoValue(int position) const
{
    position &= (buffer.getNumSamples() - 1);

    return buffer.getSample(0, position);// +buffer.getSample(1, position)) * 0.5f;
}

#if 0

class RingBufferTest : public juce::UnitTest
{
public:
    RingBufferTest() : UnitTest("RingBufferTest") {}

    void checkRead(juce::AudioBuffer<float>& source, RingBuffer& rb)
    {
        juce::AudioBuffer<float> destination{ source.getNumChannels(), source.getNumSamples() };
        rb.readWithoutAdvancing(destination);
        for (int channel = 0; channel < source.getNumChannels(); ++channel)
        {
            for (int index = 0; index < source.getNumSamples(); ++index)
            {
                auto s1 = source.getSample(channel, index);
                auto s2 = destination.getSample(channel, index);
                float difference = std::abs(s1 - s2);
                expect(difference <= std::numeric_limits<float>::epsilon());
            }
        }
    }

    void runTest() override
    {
        beginTest("RingBuffer");

        RingBuffer rb;
        int numBufferSamples = 12;
        rb.setSize(2, numBufferSamples);

        juce::AudioBuffer<float> source{ 2, numBufferSamples / 2 };
        for (int channel = 0; channel < source.getNumChannels(); ++channel)
        {
            for (int index = 0; index < source.getNumSamples(); ++index)
            {
                source.setSample(channel, index, channel * 1000.0f + (float)index);
            }
        }

        rb.write(source);
        rb.write(source);

        checkRead(source, rb);
        
        rb.advanceReadPosition(source.getNumSamples());
        checkRead(source, rb);

        rb.write(source);
        rb.advanceReadPosition(source.getNumSamples());
        checkRead(source, rb);
    }
};

static RingBufferTest test;

#endif
