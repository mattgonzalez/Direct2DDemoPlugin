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
