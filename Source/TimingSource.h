#pragma once

#include <JuceHeader.h>
#include "ThreadMessageQueue.h"

class TimingSource : public juce::Thread
{
public:
    TimingSource(juce::Component* const component_, juce::WaitableEvent& audioInputEvent_, ThreadMessageQueue& threadMessages_);
    ~TimingSource() override;

    void setFrameRate(double framesPerSecond);
    void setMode(int renderMode);

    void resetStats();

    void run() override;
    void stopAllTimers();
    void onVBlank();
    std::function<void()> onPaintTimer;

    juce::StatisticsAccumulator<double> measuredTimerIntervalSeconds;

    int64_t const ticksPerSecond = juce::Time::getHighResolutionTicksPerSecond();
    double const secondsPerTick = 1.0 / (double)juce::Time::getHighResolutionTicksPerSecond();

private:
    juce::Component* const component;
    juce::WaitableEvent& audioInputEvent;
    ThreadMessageQueue& threadMessages;
    std::unique_ptr<juce::VBlankAttachment> vblankAttachment;
    int64_t lastTimerTicks = juce::Time::getHighResolutionTicks();
    int64_t nextPaintTicks = lastTimerTicks;
    int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
    int64_t ticksPerFrame = juce::Time::getHighResolutionTicksPerSecond();

    void servicePaintTimer();
};
