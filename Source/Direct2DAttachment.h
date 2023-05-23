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

//==============================================================================
/**
    Direct2DAttachment sets up Direct2D rendering for a Component and provides
    support for immediate rendering from any thread.

    To use Direct2D, add a Direct2DAttachment as a member of one of your Components and then call attach(). 


    class Direct2DAttachmentExample : public juce::DocumentWindow
    {
    public:
        Direct2DAttachmentExample() :
            DocumentWindow("D2D Desktop Window", juce::Colours::black, juce::DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(new Content, true);
            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());

            setVisible(true);
        }

        ~Direct2DAttachmentExample() override = default;

        void closeButtonPressed() override
        {
        }

        class Content : public juce::Component
        {
        public:
            Content() :
                d2dAttachment(this)
            {
                setSize(500, 500);

                d2dAttachment.attach();
            }
            ~Content() override = default;

            void paint(juce::Graphics& g) override
            {
                g.fillAll(juce::Colours::black);
                g.setColour(juce::Colours::white);

                g.setFont(40.0f);
                g.addTransform(juce::AffineTransform::rotation(animationPosition.phase, getWidth() * 0.5f, getHeight() * 0.5f));
                g.drawText("Direct2D " + juce::String{ paintCount++ }, getLocalBounds(), juce::Justification::centred);
            }

            void onVBlank()
            {
                //
                // Measure elapsed time since last paint
                //
                auto now = juce::Time::getHighResolutionTicks();
                auto elapsedSeconds = juce::Time::highResolutionTicksToSeconds(now - lastPaintTicks);

                //
                // Advance the animation position by the elapsed time
                //
                animationPosition.advance((float)(juce::MathConstants<double>::twoPi * rotationsPerSecond * elapsedSeconds));

                //
                // Paint immediately if it's been more than 20 msec
                //
                if (elapsedSeconds >= 0.02)
                {
                    d2dAttachment.paintImmediately();

                    lastPaintTicks = now;
                }
            }

            Direct2DAttachment d2dAttachment;
            juce::VBlankAttachment vblankAttachment{ this, [this]() { onVBlank(); } };

            juce::dsp::Phase<float> animationPosition;
            int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
            double const rotationsPerSecond = 0.25;
            int paintCount = 0;
        };
    };
*/

class Direct2DAttachment
{
public:
    Direct2DAttachment(juce::Component* owner_);
    ~Direct2DAttachment();

    void attach();
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
    juce::ComponentPeer* peer = nullptr;
    bool attached = false;
    int64_t windowSubclassID = juce::Time::getHighResolutionTicks();
       
    //
    // Use this component watcher to create the Direct2D low-level graphics context when there's a new ComponentPeer
    // or to free Direct2D if the ComponentPeer goes away
    //
    struct AttachedComponentPeerWatcher : public juce::ComponentMovementWatcher
    {
        AttachedComponentPeerWatcher(Direct2DAttachment* direct2DAttachment_, juce::Component* component_);
        ~AttachedComponentPeerWatcher() override;

        void componentMovedOrResized(bool /*wasMoved*/, bool /*wasResized*/) override {}
        void componentPeerChanged() override;
        void componentVisibilityChanged() override {}

        Direct2DAttachment* direct2DAttachment = nullptr;
    } originalComponentWatcher;

    //
    // This watcher just listens for window size changes on the desktop parent in case the window is resized by
    // the app internally
    //
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

    void createDirect2DContext(juce::ComponentPeer* peer_);
    void removeDirect2DContext();
};
