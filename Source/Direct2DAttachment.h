#pragma once

#include <JuceHeader.h>

class Direct2DAttachment
{
public:
    Direct2DAttachment();
    ~Direct2DAttachment();

    void attach(juce::Component* owner_, bool wmPaintEnabled_);
    void detach();
    bool isAttached() const noexcept
    {
#if JUCE_DIRECT2D
        return direct2DLowLevelGraphicsContext != nullptr;
#else
        return false;
#endif
    }

    void paintImmediately();
    void handleResize();

    juce::CriticalSection& getLock()
    {
        return lock;
    }

    void resetStats();

    juce::StatisticsAccumulator<double> measuredFrameIntervalSeconds;
    juce::StatisticsAccumulator<double> measuredFrameDurationSeconds;
    int wmPaintCount = 0;

protected:
    juce::CriticalSection lock;
    juce::Component::SafePointer<juce::Component> component;
    bool wmPaintEnabled = false;
    
    struct AttachedComponentPeerWatcher : public juce::ComponentMovementWatcher
    {
        AttachedComponentPeerWatcher(Direct2DAttachment* direct2DAttachment_);

        void componentMovedOrResized(bool /*wasMoved*/, bool /*wasResized*/) override {}
        void componentPeerChanged() override;
        void componentVisibilityChanged() override {}

        Direct2DAttachment* direct2DAttachment = nullptr;
    };
    struct DesktopComponentWatcher : public juce::ComponentMovementWatcher
    {
        DesktopComponentWatcher(Direct2DAttachment* direct2DAttachment_, juce::Component* desktopComponent_);
        void componentMovedOrResized(bool /*wasMoved*/, bool wasResized) override;
        void componentPeerChanged() override {}
        void componentVisibilityChanged() override {}
        void componentParentHierarchyChanged(juce::Component&) override {}

        Direct2DAttachment* direct2DAttachment = nullptr;
    };

    int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
    double const secondsPerTick = 1.0 / (double)juce::Time::getHighResolutionTicksPerSecond();
   
    std::unique_ptr<AttachedComponentPeerWatcher> originalComponentWatcher;
    std::unique_ptr<DesktopComponentWatcher> desktopComponentWatcher;
#if JUCE_DIRECT2D
    std::unique_ptr<juce::Direct2DLowLevelGraphicsContext> direct2DLowLevelGraphicsContext;
#endif

    void createDirect2DContext();
    void removeDirect2DContext();
};
