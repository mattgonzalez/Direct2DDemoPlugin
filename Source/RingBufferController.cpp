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
