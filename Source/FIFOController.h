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

#include <JuceHeader.h>

class FIFOController
{
public:
    FIFOController();

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
