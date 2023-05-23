# Direct2D Demo VST3 Plugin

A JUCE-based VST3 plugin demonstrating Direct2D rendering in a JUCE Component. 

# Overview

This is a Windows VST3 plugin for demonstrating Direct2D rendering with JUCE and measuring performance. The plugin editor displays a stereo frequency spectrum and painting statistics. The plugin can switch between the standard JUCE software renderer, Direct2D rendering, or Direct2D rendering from a background thread. 

![Direct2D-big-120FPS](https://github.com/mattgonzalez/Direct2DDemoPlugin/assets/1240735/d6911be8-2081-4397-9e86-13c3a61137fa)

The statistics in the corner show the interval between each frame and how long each frame took to paint.

![Direct2D-big-120FPS-stats](https://github.com/mattgonzalez/Direct2DDemoPlugin/assets/1240735/35757947-27b0-454c-b245-b60f8caf3fcc)

# Modes

To switch modes, hover over the arrow in the corner to show the settings panel. Here you can set the frame rate or the render mode (both of which are also plugin parameters).

The diagram on the settings panel shows the sequence of events for painting a new frame.

## JUCE software renderer mode

This is the standard JUCE software-based renderer using Windows GDI (BeginPaint, etc). The plugin editor uses a JUCE VBlankAttachment to listen for the monitor's vertical blank interval and uses the standard JUCE methods to repaint the window.

Here's the sequence of events involved in painting a new frame:

- The JUCE internal VSyncThread waits for the next vertical blank, then posts a message to the message thread
- The message thread receives and delivers the message, resulting in an onVBlank callback to the VBlankAttachment for the plugin editor
- The plugin editor checks the elapsed time since the last frame. If enough time has passed, the plugin editor calls repaint() from within the VBlankAttachment onVBlank callback, which invalidates the window area.
- Windows sends a WM_PAINT message to the window
- The plugin editor paints the window from its paint callback

Note that there are two significant sources of unpredictable delay; the time between the VSyncThread posting and the onVBlank callback, and the time between calling repaint() and the paint callback. Under a light load this is probably fine, but as the message loop gets busier the animation timing will get sloppier.


## Direct2D from VBlankAttachment mode

This is using a modified JUCE Direct2DLowLevelGraphicsContext to render. Here, the plugin editor is also using a JUCE VBlankAttachment to listen for the vertical blank. Direct2D can render immediately without waiting for the repaint -> WM_PAINT cycle, so the editor paints the window directly from within the onVBlank callback.

In this mode:

- The JUCE internal VSyncThread waits for the next vertical blank, then posts a message to the message thread
- The message thread receives and delivers the message, resulting in an onVBlank callback to the VBlankAttachment for the plugin editor
- The plugin editor checks the elapsed time since the last frame. If enough time has passed, the plugin editor paints the window from the onVBlank callback.

This cuts out two steps, removes one source of timing jitter, and cuts down on message thread traffic.

## "Direct2D from dedicated thread" mode

This is using the same modified Direct2DLowLevelGraphicsContext as the previous case. In this mode, the editor launches a dedicated paint thread and paints from that thread instead of message thread. The VSyncThread really isn't necessary with Direct2D anyhow; Direct2D can handle VSync when renders.

This mode does not use the VBlankAttachment; instead, the plugin editor uses the audio clock as a timing source. The sequence of events for this mode is a little different:

- The dedicated paint thread waits on a WaitableEvent shared between the editor and processor
- The plugin's processBlock callback fires
- The processBlock callback signals the shared WaitableEvent
- The paint thread wakes up and immediately paints from the paint thread

This removes the potential delay from notifying the VBlankAttachment. There will still be delay between signaling the shared event and the paint thread waking up, but that should be shorter and more reliable than posting a message.

Of course, painting off the message thread is risky! The JUCE component hierarchy is definitely not meant to be thread-safe. This mode is here primarily to demonstrate that this is possible and to show the timing benefits.

# Building the plugin

This plugin requires the direct2d branch of my fork of JUCE: https://github.com/mattgonzalez/JUCE/tree/direct2d

You'll need to clone both this repository and the JUCE fork, switch to the direct2d branch, and then run the Projucer. Point the Projucer to the JUCE modules in the Direct2D fork, then use the Projucer to save the project and create the Visual Studio solution. 


# Using Direct2D in your own application

If you'd like to try Direct2D, the simplest approach is to clone the JUCE fork shown above. You'll need to set a couple of preprocessor flags in your project:

| Flag                          | Default | Description                                                                                      |
|-------------------------------|---------|--------------------------------------------------------------------------------------------------|
| JUCE_DIRECT2D                 | 0       | Enables Direct2D support                                                                         |
|                               |         |                                                                                                  |
| JUCE_DIRECT2D_PARTIAL_REPAINT | 1       | If enabled, the Direct2DLowLevelGraphicsContext will only redraw the invalid part of the window. Otherwise, it redraws the entire window every time. |
|                               |         |                                               |
| JUCE_WAIT_FOR_VBLANK          | 1       | If disabled, turns off the JUCE VSync thread; Direct2D can also handle waiting for VSync         |

Be sure to define JUCE_DIRECT2D=1. The other flags are optional; partial repainting seems to work well, but if you're seeing painting artifacts or visual tearing you could try disalbing partial repainting or VSync.

Direct2D is off by default. To switch to Direct2Dmode, enable it in the constructor of your main window:

```
            //
            // Turn on Direct2D mode; make sure to do this after the window has been added to the desktop
            //
            #if JUCE_DIRECT2D
            jassert(getPeer() && isOnDesktop());
            if (auto peer = getPeer())
            {
                peer->setCurrentRenderingEngine(1); // Enable Direct2D
            }
            #endif
```

Calling repaint(), paint() functions, and the Graphics class should all work just as they did before. However, there's no support for on-demand painting; you'll still need to call repaint() and wait for the WM_PAINT message as before.

# Direct2DAttachment

The Direct2DAttachment class sets up Direct2D rendering and enables on-demand painting.

All you have to do is create one as a member of any component and then call Direct2DAttachment::attach(); Direct2DAttachment will find the desktop parent of that component and set up Direct2D for the whole window. It then turns off the redirection surface for the window and subclasses the window in order to intercept paint and sizing messages.

```
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
```

You can continue to use repaint() and paint() as before. Or - call paintImmediately() to paint the entire window from a timer callback, or from any thread Once again - painting off the message thread is tricky! Be sure to use the Direct2DAttachment lock to synchronize between the message thread and the painting thread.

Painting off the message thread also doesn't support Component effects like drop shadows or transformed Components; for now it's really just a proof-of-concept.

The plugin demonstrates how to use Direct2DAttachment to render both on and off the message thread.
