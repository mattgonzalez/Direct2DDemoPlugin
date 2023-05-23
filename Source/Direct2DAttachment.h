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

class Direct2DAttachment
{
public:
    Direct2DAttachment(juce::Component* owner_);
    ~Direct2DAttachment();

    void attach(bool wmPaintEnabled_ = true);
    void detach();
    bool isAttachmentPending() const noexcept
    {
        return attached && direct2DLowLevelGraphicsContext == nullptr;
    }
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
    juce::Component::SafePointer<juce::Component> owner;
    bool attached = false;
    bool wmPaintEnabled = false;
    const int windowSubclassID = (int)juce::Time::getHighResolutionTicks();
    
    struct AttachedComponentPeerWatcher : public juce::ComponentMovementWatcher
    {
        AttachedComponentPeerWatcher(Direct2DAttachment* direct2DAttachment_, juce::Component* component_);
        ~AttachedComponentPeerWatcher() override;

        void componentMovedOrResized(bool /*wasMoved*/, bool /*wasResized*/) override {}
        void componentPeerChanged() override;
        void componentVisibilityChanged() override {}

        Direct2DAttachment* direct2DAttachment = nullptr;
    } originalComponentWatcher;

    struct DesktopComponentWatcher : public juce::ComponentMovementWatcher
    {
        DesktopComponentWatcher(Direct2DAttachment* direct2DAttachment_, juce::Component* desktopComponent_);
        ~DesktopComponentWatcher() override = default;

        void componentMovedOrResized(bool /*wasMoved*/, bool wasResized) override;
        void componentPeerChanged() override {}
        void componentVisibilityChanged() override {}
        void componentParentHierarchyChanged(juce::Component&) override {}

        Direct2DAttachment* direct2DAttachment = nullptr;
    };

    int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
    double const secondsPerTick = 1.0 / (double)juce::Time::getHighResolutionTicksPerSecond();
   
    std::unique_ptr<DesktopComponentWatcher> desktopComponentWatcher;
#if JUCE_DIRECT2D
    std::unique_ptr<juce::Direct2DLowLevelGraphicsContext> direct2DLowLevelGraphicsContext;
#endif

    void createDirect2DContext();
    void removeDirect2DContext();
};
