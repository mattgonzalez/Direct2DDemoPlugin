#include "ProcessorOutputRingBuffer.h"

void ProcessorOutputRingBuffer::setSize(int numItems, int numChannels, int fftSize)
{
    ringController.setRingSize(numItems);

    while (array.size() < ringController.getRingSize())
    {
        array.add(new ProcessorOutput);
    }

    for (auto entry : array)
    {
        entry->spectrum = RealSpectrum<float>{}.withChannels(numChannels).withFFTSize(fftSize);
        entry->energySpectrum = RealSpectrum<float>{}.withChannels(numChannels).withFFTSize(fftSize);
    }
}

void ProcessorOutputRingBuffer::reset()
{
    ringController.reset(0);
    for (auto entry: array)
    {
        entry->spectrum.clear();
        entry->energySpectrum.clear();
    }
}

ProcessorOutput* const ProcessorOutputRingBuffer::getWritePointer() const
{
    return array[ringController.getWritePosition()];    
}

ProcessorOutput const* const ProcessorOutputRingBuffer::getMostRecent() const
{
    return array[ringController.getWritePosition(-1)];
}

void ProcessorOutputRingBuffer::advanceWritePosition()
{
    ringController.advanceWritePosition(1);
}

void ProcessorOutputRingBuffer::advanceReadPosition()
{
    ringController.flush();
}

int ProcessorOutputRingBuffer::getNumItemsStored() const
{
    return ringController.getNumItemsStored();
}
