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

#if _WIN32
#include <windows.h>
#include <commctrl.h>
#endif
#include "Direct2DAttachment.h"

Direct2DAttachment::Direct2DAttachment()
{
}

Direct2DAttachment::~Direct2DAttachment()
{
    detach();
}

void Direct2DAttachment::attach(juce::Component* component_, bool wmPaintEnabled_)
{
    DBG("Direct2DAttachment::attach " << component_->getName() << " WM_PAINT enabled:" << (int)wmPaintEnabled_);

    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
    jassert(component_);

    detach();

    juce::ScopedLock locker{ lock };

    component = component_;
    wmPaintEnabled = wmPaintEnabled_;
    wmPaintCount = 0;

    createDirect2DContext();

    originalComponentWatcher = std::make_unique<AttachedComponentPeerWatcher>(this);
}

void Direct2DAttachment::detach()
{
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    juce::ScopedLock locker{ lock };

    removeDirect2DContext();

    originalComponentWatcher = nullptr;
    desktopComponentWatcher = nullptr;

    component = nullptr;
}

#if JUCE_DIRECT2D

static constexpr int windowSubclassID = 0xd2d12;

static LRESULT subclassProc(HWND hwnd, UINT umsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData)
{
    Direct2DAttachment* that = reinterpret_cast<Direct2DAttachment*>(static_cast<LONG_PTR>(dwRefData));

    switch (umsg)
    {
    case WM_PAINT:
    {
        that->wmPaintCount++;
        that->paintImmediately();
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;

    case WM_SIZING:
    {
        that->handleResize();
        break;
    }
    }

    return DefSubclassProc(hwnd, umsg, wParam, lParam);
}

static void paintComponentsRecursive(juce::Graphics& g, juce::Component* component, juce::Component* desktopComponent)
{
    {
        juce::Graphics::ScopedSaveState ss(g);
        component->paint(g);
    }

    for (int i = 0; i < component->getNumChildComponents(); ++i)
    {
        auto child = component->getChildComponent(i);

        juce::Graphics::ScopedSaveState ss(g);

        g.reduceClipRegion(child->getBounds());
        g.setOrigin(child->getPosition());
        paintComponentsRecursive(g, child, desktopComponent);
    }
}
#endif

void Direct2DAttachment::paintImmediately()
{
#if JUCE_DIRECT2D
    auto startTime = juce::Time::getHighResolutionTicks();

    juce::ScopedTryLock locker{ lock };

    if (!locker.isLocked())
    {
        return;
    }

    if (direct2DLowLevelGraphicsContext && component)
    {
        juce::Graphics g{ *direct2DLowLevelGraphicsContext };

        direct2DLowLevelGraphicsContext->start();

        JUCE_TRY
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            {
                if (auto peer = component->getPeer())
                {
                    peer->handlePaint(*direct2DLowLevelGraphicsContext);
                }
            }
            else
            {
                if (desktopComponentWatcher)
                {
                    if (auto desktopComponent = desktopComponentWatcher->getComponent())
                    {
                        paintComponentsRecursive(g, desktopComponent, desktopComponent);
                    }
                }
            }
        }
        JUCE_CATCH_EXCEPTION

        direct2DLowLevelGraphicsContext->end();

        auto endTime = juce::Time::getHighResolutionTicks();
        auto duration = endTime - startTime;
        measuredFrameDurationSeconds.addValue(secondsPerTick * duration);
        auto interval = startTime - lastPaintTicks;
        if (lastPaintTicks != 0)
        {
            measuredFrameIntervalSeconds.addValue(secondsPerTick * interval);
        }
        lastPaintTicks = startTime;
    }
#endif
}

void Direct2DAttachment::handleResize()
{
#if JUCE_DIRECT2D
    {
        juce::ScopedLock locker{ lock };

        if (direct2DLowLevelGraphicsContext)
        {
            direct2DLowLevelGraphicsContext->resized();
        }
    }

    paintImmediately();
#endif
}

void Direct2DAttachment::resetStats()
{
    juce::ScopedLock locker{ lock };

    measuredFrameDurationSeconds.reset();
    measuredFrameIntervalSeconds.reset();
    wmPaintCount = 0;

    lastPaintTicks = 0;
}

void Direct2DAttachment::createDirect2DContext()
{
#if JUCE_DIRECT2D
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (component)
    {
        if (auto peer = component->getPeer())
        {
            //
            // Make sure the window is in software mode; otherise the peer will have its own Direct2D context
            // which will conflict with the one about to be created
            //
            peer->setCurrentRenderingEngine(0);

            //
            // Make a Direct2DLowLevelGraphicsContext
            //
            auto hwnd = (HWND)peer->getNativeHandle();
            direct2DLowLevelGraphicsContext = nullptr;
            direct2DLowLevelGraphicsContext = std::make_unique<juce::Direct2DLowLevelGraphicsContext>(hwnd);

            //
            // I'd like to turn WM_PAINT off completely, but it still seems to be necessary
            //
            if (!wmPaintEnabled)
            {
                SendMessage(hwnd, WM_SETREDRAW, FALSE, 0);
            }

            //
            // Subclass the window to take over painting and sizing for the window
            //
            auto ok = SetWindowSubclass(hwnd, subclassProc, windowSubclassID, (DWORD_PTR)this);
            jassert(ok);
            juce::ignoreUnused(ok);

            //
            // Make sure the window is sized properly
            //
            handleResize();

            //
            // Need to attach a watcher to the desktop component in case the size is changed internally by JUCE
            //
            desktopComponentWatcher = std::make_unique<DesktopComponentWatcher>(this, component->getTopLevelComponent());
        }
    }
#endif
}

void Direct2DAttachment::removeDirect2DContext()
{
#if JUCE_DIRECT2D
    jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());

    if (direct2DLowLevelGraphicsContext)
    {
        direct2DLowLevelGraphicsContext = nullptr;
    }

    if (desktopComponentWatcher)
    {
        if (auto desktopComponent = desktopComponentWatcher->getComponent())
        {
            if (auto peer = desktopComponent->getPeer())
            {
                auto hwnd = (HWND)peer->getNativeHandle();

                SendMessage(hwnd, WM_SETREDRAW, TRUE, 0);

                auto ok = RemoveWindowSubclass(hwnd, subclassProc, windowSubclassID);
                jassert(ok);
                juce::ignoreUnused(ok);

                InvalidateRect(hwnd, nullptr, TRUE);
            }
        }

        desktopComponentWatcher = nullptr;
    }
#endif
}

Direct2DAttachment::AttachedComponentPeerWatcher::AttachedComponentPeerWatcher(Direct2DAttachment* direct2DAttachment_) :
    ComponentMovementWatcher(direct2DAttachment_->component),
    direct2DAttachment(direct2DAttachment_)
{
}

void Direct2DAttachment::AttachedComponentPeerWatcher::componentPeerChanged()
{
#if JUCE_DIRECT2D
    juce::ScopedLock locker{ direct2DAttachment->lock };

    //
    // See if the window is on the desktop
    //
    if (auto peer = getComponent()->getPeer())
    {
        if (auto windowComponent = getComponent()->getTopLevelComponent())
        {
            if (windowComponent->isOnDesktop() && !direct2DAttachment->isAttached())
            {
                //
                // The window is on the desktop and has a window handle; OK to create the Direct2D context
                //
                direct2DAttachment->createDirect2DContext();
                return;
            }
        }
    }

    direct2DAttachment->detach();
#endif
}

Direct2DAttachment::DesktopComponentWatcher::DesktopComponentWatcher(Direct2DAttachment* direct2DAttachment_, juce::Component* desktopComponent_) :
    ComponentMovementWatcher(desktopComponent_),
    direct2DAttachment(direct2DAttachment_)
{
}

void Direct2DAttachment::DesktopComponentWatcher::componentMovedOrResized(bool /*wasMoved*/, bool wasResized)
{
#if JUCE_DIRECT2D
    if (wasResized)
    {
        juce::ScopedLock locker{ direct2DAttachment->lock };

        //
        // Need this watcher to listen to window size changes performed internally by the app
        //
        direct2DAttachment->handleResize();
    }
#endif
}
