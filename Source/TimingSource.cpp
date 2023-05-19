#include "TimingSource.h"
#include "Direct2DDemoEditor.h"

TimingSource::TimingSource(juce::Component* const component_, juce::WaitableEvent& audioInputEvent_) :
    Thread("TimingSource"),
    component(component_),
    audioInputEvent(audioInputEvent_)
{
}

TimingSource::~TimingSource()
{
    stopAllTimers();
}

void TimingSource::update(double framesPerSecond, int renderMode)
{
    //
    // Update the ticks per frame & next frame time
    //
    // Not thread safe; room for improvement here...
    //
    ticksPerFrame = juce::roundToInt((double)juce::Time::getHighResolutionTicksPerSecond() / framesPerSecond);
    
    resetStats();

    switch (renderMode)
    {
    case RenderMode::software:
    case RenderMode::vblankAttachmentDirect2D:
    {
        vblankAttachment = std::make_unique<juce::VBlankAttachment>(component, [this]() { onVBlank(); });
        break;
    }

    case RenderMode::dedicatedThreadDirect2D:
    {
        startThread(juce::Thread::Priority::normal);
        break;
    }
    }
}

void TimingSource::resetStats()
{
    //
    // Not thread safe...
    //
    lastPaintTicks = 0;
    lastTimerTicks = 0;
    nextPaintTicks = juce::Time::getHighResolutionTicks() + ticksPerFrame;

    measuredTimerIntervalSeconds.reset();
}

void TimingSource::run()
{
    while (false == threadShouldExit())
    {
        audioInputEvent.wait(100);

        servicePaintTimer();
    }
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
    stopThread(1000);
}
