#pragma once

#include <JuceHeader.h>

class RingBufferController
{
public:
    RingBufferController();

    struct Block
    {
        int position;
        int count;
    };

    int setRingSize(int numItems);
    int getRingSize() const
    {
        return ringSize;
    }
    void reset(int numStoredItems);
    int getNumItemsStored() const;

    Block getWriteBlock(int numItemsWanted);
    Block getReadBlock(int numItemsWanted, int numItemsAlreadyRead);

    int getReadPosition(int offset = 0) const;
    int getWritePosition(int offset = 0) const;
    int getSafeTransferCount(int numItemsWanted, int position) const;
    void advanceReadPosition(int count);
    void advanceWritePosition(int count);
    void flush();

private:
    int readCount = 0;
    int writeCount = 0;
    int ringSize = 0;
};
