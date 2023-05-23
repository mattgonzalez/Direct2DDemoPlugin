# Direct2D Demo VST3 Plugin

A JUCE-based VST3 plugin demonstrating Direct2D rendering in a JUCE Component. 

# Overview

This is a Windows VST3 plugin that displays a music visualizer. The plugin can switch between the standard JUCE software renderer, Direct2D rendering, or Direct2D rendering from a background thread.

The plugin displays statistics showing the interval between each frame and how long each frame took to paint.

# Modes

To switch modes, hover over the arrow in the corner to show the settings panel. Here you can set the frame rate or the render mode (both of which are also plugin parameters).

The diagram on the settings panel shows the sequence of events for painting a new frame.

## "JUCE software renderer" mode

This is the standard JUCE software-based renderer using Windows GDI (BeginPaint, etc). The plugin editor uses a JUCE VBlankAttachment to listen for the monitor's vertical blank interval and uses the standard JUCE methods to repaint the window.

Here's the sequence of events involved in painting a new frame:

- The JUCE internal VSyncThread waits for the next vertical blank, then posts a message to the message thread
- The message thread receives and delivers the message, resulting in an onVBlank callback to the VBlankAttachment for the plugin editor
- The plugin editor checks the elapsed time since the last frame. If enough time has passed, the plugin editor calls repaint() from within the VBlankAttachment onVBlank callback, which invalidates the window area.
- Windows sends a WM_PAINT message to the window
- The plugin editor paints the window from its paint callback

Everyone's probably familiar with this approach already. It's worth noting that there are two significant sources of unpredictable delay; the time between the VSyncThread posting and the onVBlank callback, and the time between calling repaint() and the paint callback. Under a light load this is probably fine, but as the message loop gets busier the animation timing will get sloppier.


## "Direct2D from VBlankAttachment" mode

This is using a modified and rewritten JUCE Direct2DLowLevelGraphicsContext to render. Here, the plugin editor is also using a JUCE VBlankAttachment to listen for the vertical blank. Direct2D can render immediately without waiting for the repaint -> WM_PAINT cycle, so the editor paints the window directly from within the onVBlank callback.

Here's the sequence of events for this mode:

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

This removes the potential delay from notifying the VBlankAttachment. There will still be delay between signaling the shared event and the paint thread waking up, but that should be shorter and more reliable.

Of course, painting off the message thread is risky! The JUCE component hierarchy is definitely not meant to be thread-safe. This mode is here primarily to demonstrate that this is possible and to show the timing benefits.

# Building the plugin

This plugin requires the direct2d branch of my fork of JUCE: https://github.com/mattgonzalez/JUCE/tree/direct2d

You'll need to clone both this repository and the JUCE fork, and then run the Projucer. Be sure to point the Projucer to the JUCE modules in the Direct2D fork, then use the Projucer to save the project and create the Visual Studio solution. 


# Using Direct2D in your own application

If you'd like to try using Direct2D, the simplest approach is to clone the JUCE fork shown above. You'll need to set a couple of preprocessor flags in your project:

| Flag                          | Default | Description                                                                                      |
|-------------------------------|---------|--------------------------------------------------------------------------------------------------|
| JUCE_DIRECT2D                 | 0       | Enables Direct2D support                                                                         |
|                               |         |                                                                                                  |
| JUCE_DIRECT2D_PARTIAL_REPAINT | 1       | If enabled, the Direct2DLowLevelGraphicsContext will only redraw the invalid part of the window. Otherwise, it redraws the entire window every time. |
|                               |         |                                               |
| JUCE_WAIT_FOR_VBLANK          | 1       | If disabled, turns off the JUCE VSync thread; Direct2D can also handle waiting for VSync         |

Be sure to define JUCE_DIRECT2D=1. The other flags are optional; partial repainting seems to work well, but if you're seeing painting artifacts or visual tearing you could try disalbing partial repainting or VSync.

Direct2D is off by default, so you'll need to opt-in. The simplest approach is to enable it in the constructor of your main window:

```
            //
            // Turn on Direct2D mode; make sure this window is on the desktop and has a ComponentPeer
            //
            #if JUCE_DIRECT2D
            jassert(getPeer() && isOnDesktop());
            if (auto peer = getPeer())
            {
                peer->setCurrentRenderingEngine(1); // Enable Direct2D
            }
            #endif
```

That's all you need to do. Calling repaint(), paint() functions, and the Graphics class should all work just as they did before. However, there's no support for on-demand painting; you'll still need to call repaint() and wait for the WM_PAINT message as before.

# Direct2DAttachment

Check out the Direct2DAttachment class in the plugin. All you have to do is create one (probably as a member of your top level component) and then call Direct2DAttchment::attach with a pointer to any component in your window; Direct2DAttachment will find the desktop component and set up Direct2D. It then turns off the redirection surface for the window and subclasses it so it can intercept paint and sizing messages. 

You can continue to use repaint() and paint() as before. Or - call paintImmediately to paint the entire window from a timer callback, or from any thread Once again - painting off the message thread is risky! You need to be very careful about creating and destroying components on the fly, plus many more issues like that. Be sure to use the Direct2DAttachment lock to synchronize between the message thread and the painting thread.

Painting off the message thread also doesn't support Component effects like drop shadows or transformed Components; it's really just a proof-of-concept.

Happy rendering!

