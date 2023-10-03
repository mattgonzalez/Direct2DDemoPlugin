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

class TimingSource
{
public:
    TimingSource(juce::Component* const component_);
    ~TimingSource();

    void setFrameRate(double framesPerSecond);
    void setMode(int renderMode);

    void resetStats();

    void stopAllTimers();
    void onVBlank();
    std::function<void()> onPaintTimer;

    juce::StatisticsAccumulator<double> measuredTimerIntervalSeconds;

    int64_t const ticksPerSecond = juce::Time::getHighResolutionTicksPerSecond();
    double const secondsPerTick = 1.0 / (double)juce::Time::getHighResolutionTicksPerSecond();
    double nominalFrameIntervalSeconds = 0.0;

private:
    juce::Component* const component;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    int64_t lastTimerTicks = juce::Time::getHighResolutionTicks();
    int64_t nextPaintTicks = lastTimerTicks;
    int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
    int64_t ticksPerFrame = juce::Time::getHighResolutionTicksPerSecond();

    void servicePaintTimer();
};
