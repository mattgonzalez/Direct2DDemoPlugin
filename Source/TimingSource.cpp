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

#include "TimingSource.h"
#include "Direct2DDemoEditor.h"

TimingSource::TimingSource(juce::Component* const component_) :
    component(component_)
{
}

TimingSource::~TimingSource()
{
    stopAllTimers();
}

void TimingSource::setFrameRate(double framesPerSecond)
{
    nominalFrameIntervalSeconds = 1.0 / framesPerSecond;
    ticksPerFrame = juce::roundToInt((double)juce::Time::getHighResolutionTicksPerSecond() / framesPerSecond);
}

void TimingSource::setMode(int renderMode)
{
    switch (renderMode)
    {
    case RenderMode::software:
    case RenderMode::vblankAttachmentDirect2D:
    {
        vblankAttachment = std::make_unique<juce::VBlankAttachment>(component, [this]() { onVBlank(); });
        break;
    }
    }
}

void TimingSource::resetStats()
{
    lastPaintTicks = 0;
    lastTimerTicks = 0;
    nextPaintTicks = juce::Time::getHighResolutionTicks() + ticksPerFrame;

    measuredTimerIntervalSeconds.reset();
}

void TimingSource::onVBlank()
{
    servicePaintTimer();
}

void TimingSource::servicePaintTimer()
{
    auto now = juce::Time::getHighResolutionTicks();
    auto ticksSinceLastTimer = now - lastTimerTicks;
    lastTimerTicks = now;

    //
    // Too long since the last timer service?
    //
    if (ticksSinceLastTimer > 4 * ticksPerFrame)
    {
        nextPaintTicks = now + ticksPerFrame;
        return;
    }

    measuredTimerIntervalSeconds.addValue(ticksSinceLastTimer * secondsPerTick);

    if (now >= nextPaintTicks)
    {
        //
        // Update nextPaintTicks
        //
        nextPaintTicks += ticksPerFrame;

        //
        // Painting time!
        //
        if (onPaintTimer)
        {
            onPaintTimer();
        }
    }

    lastTimerTicks = now;
}

void TimingSource::stopAllTimers()
{
    vblankAttachment = nullptr;
}
