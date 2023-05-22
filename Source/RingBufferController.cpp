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

#include "RingBufferController.h"

RingBufferController::RingBufferController()
{
    reset(0);
}

int RingBufferController::setRingSize(int numItems)
{
    ringSize = juce::nextPowerOfTwo(numItems);
    return ringSize;
}

void RingBufferController::reset(int numStoredItems)
{
    readCount = 0;
    writeCount = numStoredItems;
}

int RingBufferController::getNumItemsStored() const
{
    return (writeCount - readCount) & (ringSize - 1);
}

RingBufferController::Block RingBufferController::getWriteBlock(int numItemsWanted)
{
    int position = getWritePosition();

    return
    {
        position,
        juce::jmin(numItemsWanted, ringSize - position)
    };
}

RingBufferController::Block RingBufferController::getReadBlock(int numItemsWanted, int numItemsAlreadyRead)
{
    int position = getReadPosition(numItemsAlreadyRead);

    return
    {
        position,
        juce::jmin(numItemsWanted, ringSize - position)
    };
}

int RingBufferController::getReadPosition(int offset) const
{
    return (readCount + offset) & (ringSize - 1);
}

int RingBufferController::getWritePosition(int offset) const
{
    return (writeCount - offset) & (ringSize - 1);
}

int RingBufferController::getSafeTransferCount(int numItemsWanted, int position) const
{
    return juce::jmin(numItemsWanted, ringSize - (position & (ringSize - 1)));
}

void RingBufferController::advanceReadPosition(int count)
{
    readCount += count;
}

void RingBufferController::advanceWritePosition(int count)
{
    writeCount += count;
}

void RingBufferController::flush()
{
    readCount = writeCount;
}
