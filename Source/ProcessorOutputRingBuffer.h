#pragma once

#include "RingBufferController.h"
#include "Spectrum.h"

struct ProcessorOutput
{
    RealSpectrum<float> spectrum;
    RealSpectrum<float> energySpectrum;
};

class ProcessorOutputRingBuffer
{
public:
    void setSize(int numItems, int numChannels, int fftSize);
    void reset();
    ProcessorOutput* const getWritePointer() const;
    ProcessorOutput const * const getMostRecent() const;
    void advanceWritePosition();
    void advanceReadPosition();

    int getNumItemsStored() const;

private:
    RingBufferController ringController;
    juce::OwnedArray<ProcessorOutput> array;
};
