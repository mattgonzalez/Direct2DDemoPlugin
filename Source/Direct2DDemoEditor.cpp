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

#include <windows.h>
#include "Direct2DDemoProcessor.h"
#include "Direct2DDemoEditor.h"

Direct2DDemoEditor::Direct2DDemoEditor(Direct2DDemoProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    timingSource(this),
    settingsComponent(p)
{
    setName("Direct2DDemoEditor");

    setResizable(true, false);

    audioProcessor.state.state.addListener(this);

    //painter = std::make_unique<SpectrumRingDisplay>(p);

#if 1
    {
        auto window = std::make_unique<ChildWindow>(p, ChildWindow::Mode::softwareRenderer);
        addAndMakeVisible(window.get());
        window->setName("Child A");
        childWindows.add(std::move(window));
    }
#endif

#if 1 // JUCE_DIRECT2D
    {
        auto window = std::make_unique<ChildWindow>(p, ChildWindow::Mode::direct2D);
        addAndMakeVisible(window.get());
        window->setName("Child B");
        childWindows.add(std::move(window));
    }
#endif

#if 0//JUCE_OPENGL
    {
        auto window = std::make_unique<ChildWindow>(p, ChildWindow::Mode::openGL);

        addAndMakeVisible(window.get());
        window->setName("Child C");
        childWindows.add(std::move(window));
    }
#endif

    timingSource.onPaintTimer = [this]() { paintTimerCallback(); };

    //addAndMakeVisible(settingsComponent);

    int width = 1000;
    int height = 1000;
    if (0 == childWindows.size())
    {
        width = 1400;
        height = 500;
    }
    setSize(width, height);

    updateFrameRate();
    updateRenderer();
}

Direct2DDemoEditor::~Direct2DDemoEditor()
{
    timingSource.stopAllTimers();

    audioProcessor.state.state.removeListener(this);
}

void Direct2DDemoEditor::paintTimerCallback()
{
    repaint();

    for (auto window : childWindows)
    {
        window->inner.repaint();
    }

    //
    // Throw away excess data from the processor output FIFO
    //
    if (audioProcessor.outputFIFO.getNumItemsStored() > 0)
    {
        audioProcessor.outputFIFO.advanceReadPosition();
    }
}

void Direct2DDemoEditor::parentHierarchyChanged()
{
    int renderMode = audioProcessor.parameters.renderer;
    int renderingEngine = 0;
    if (auto peer = getPeer())
    {
        switch (renderMode)
        {
        case RenderMode::software:
        case RenderMode::openGL:
            break;

        case RenderMode::vblankAttachmentDirect2D:
            renderingEngine = 1;
            break;
        }

        peer->setCurrentRenderingEngine(renderingEngine);

#if JUCE_OPENGL
        if (renderMode == RenderMode::openGL)
        {
            openGLContext = std::make_unique<juce::OpenGLContext>();
            openGLContext->attachTo(*this);
        }
#endif
    }

    updateFrameRate();
    timingSource.setMode(renderMode);
}

void Direct2DDemoEditor::paint(juce::Graphics& g)
{
    if (getWidth() <= 0)
    {
        return;
    }

    g.fillAll(juce::Colours::black);

    //painter->paint(g, getLocalBounds().toFloat(), audioProcessor.outputFIFO.getMostRecent());

    paintSpectrum(g);
    paintModeText(g);
    paintStats(g);
}

void Direct2DDemoEditor::paintSpectrum(juce::Graphics& g)
{
    if (auto output = audioProcessor.outputFIFO.getMostRecent())
    {
        auto area = getLocalBounds();
        if (auto firstOwnedWindow = childWindows.getFirst())
        {
            area = firstOwnedWindow->getBounds();
            area.translate(0, -area.getHeight() - 10);
        }

        g.setColour(juce::Colour::greyLevel(0.1f));
        g.fillRect(area);

        juce::Graphics::ScopedSaveState saveState{ g };

        ChildWindow::paintSpectrum(g, area.toFloat(), "Editor", "Editor paint()", output->averageSpectrum);
    }
}

void Direct2DDemoEditor::paintModeText(juce::Graphics& g)
{
    g.setFont(20.0f);
    g.setColour(juce::Colours::white);
    juce::String text;
    if (auto peer = getPeer())
    {
        text = peer->getCurrentRenderingEngine() > 0 ? "Direct2D " : "Software renderer ";
    }
#if 0
    switch (audioProcessor.parameters.renderer)
    {
    case RenderMode::software:
        text = "Software renderer ";
        break;

    case RenderMode::vblankAttachmentDirect2D:
        text = "Direct2D ";
        break;

    case RenderMode::openGL:
        text = "OpenGL ";
        break;
    }
#endif

    text << getWidth() << "x" << getHeight();

#if JUCE_DEBUG
    text << " (debug build)";
#endif

    g.drawText(text, getLocalBounds(), juce::Justification::topLeft);
}

static void paintStat(juce::Graphics& g, juce::Rectangle<int> const r, juce::String name, double averageSeconds, juce::Colour averageColor, double standardDeviationSeconds, juce::Colour stdDevColor)
{
    juce::AttributedString as;
    auto font = g.getCurrentFont();
    as.setJustification(juce::Justification::centredLeft);
    as.append(name, font, juce::Colours::white);
    as.append(juce::String{ averageSeconds * 1000.0, 1 } + " avg", font, averageColor);
    as.append(" / ", font, juce::Colours::white);
    as.append(juce::String{ standardDeviationSeconds * 1000.0, 1 } + " std.dev.", font, stdDevColor);
    as.draw(g, r.toFloat());
}

#if JUCE_DIRECT2D_METRICS
static void paintStat(juce::Graphics& g, juce::Rectangle<int> const r, juce::String name, juce::StatisticsAccumulator<double> const& stats)
{
    name << juce::String{ stats.getAverage() * 1000.0, 1 } << " avg / " << juce::String{ stats.getStandardDeviation() * 1000.0, 1 } << " std.dev.";
    g.drawText(name, r, juce::Justification::centredLeft);
}
#endif

void Direct2DDemoEditor::paintFrameIntervalStats(juce::Graphics& g, juce::Rectangle<int>& r, juce::StatisticsAccumulator<double> const& frameIntervalSeconds)
{
    juce::Colour averageFrameIntervalColor = juce::Colours::white;
    auto averageFrameIntervalSeconds = frameIntervalSeconds.getAverage();
    if (averageFrameIntervalSeconds > timingSource.nominalFrameIntervalSeconds * 2.0)
    {
        averageFrameIntervalColor = juce::Colours::red;
    }
    else if (averageFrameIntervalSeconds > timingSource.nominalFrameIntervalSeconds * 1.5)
    {
        averageFrameIntervalColor = juce::Colours::yellow;
    }

    {
        g.setColour(juce::Colours::white);
        g.drawText("FPS:", r, juce::Justification::centredLeft, false);
        if (auto averageFrameInterval = frameIntervalSeconds.getAverage(); averageFrameInterval > 0.0)
        {
            g.setColour(averageFrameIntervalColor);
            g.drawText(juce::String{ 1.0 / averageFrameInterval, 1 }, r.translated(30, 0), juce::Justification::centredLeft);
        }
        r.translate(0, r.getHeight());
    }

    {
        juce::Colour standardDeviationColor = juce::Colours::white;
        auto standardDeviation = frameIntervalSeconds.getStandardDeviation();
        if (standardDeviation > averageFrameIntervalSeconds)
        {
            standardDeviationColor = juce::Colours::red;
        }
        else if (standardDeviation > averageFrameIntervalSeconds * 0.5)
        {
            standardDeviationColor = juce::Colours::yellow;
        }

        paintStat(g, r, "Paint interval (ms): ", averageFrameIntervalSeconds, averageFrameIntervalColor, standardDeviation, standardDeviationColor);
        r.translate(0, r.getHeight());
    }
}

void Direct2DDemoEditor::paintFrameDurationStats(juce::Graphics& g, juce::Rectangle<int>& r, juce::StatisticsAccumulator<double> const& frameDurationSeconds)
{
    juce::Colour averageDurationColor = juce::Colours::white;
    auto averageDurationSeconds = frameDurationSeconds.getAverage();
    if (averageDurationSeconds > timingSource.nominalFrameIntervalSeconds)
    {
        averageDurationColor = juce::Colours::red;
    }
    else if (averageDurationSeconds > timingSource.nominalFrameIntervalSeconds * 0.8)
    {
        averageDurationColor = juce::Colours::yellow;
    }

    juce::Colour standardDeviationColor = juce::Colours::white;
    auto standardDeviationSeconds = frameDurationSeconds.getStandardDeviation();
    if (standardDeviationSeconds > timingSource.nominalFrameIntervalSeconds)
    {
        standardDeviationColor = juce::Colours::red;
    }
    else if (standardDeviationSeconds > timingSource.nominalFrameIntervalSeconds * 0.5)
    {
        standardDeviationColor = juce::Colours::yellow;
    }

    paintStat(g, r, "Paint duration (ms): ", averageDurationSeconds, averageDurationColor, standardDeviationSeconds, standardDeviationColor);
    r.translate(0, r.getHeight());
}

void Direct2DDemoEditor::paintWmPaintCount(juce::Graphics& g, juce::Rectangle<int>& r, int wmPaintCount)
{
    g.setColour(juce::Colours::white);
    g.drawText("# WM_PAINT: " + juce::String{ wmPaintCount }, r, juce::Justification::centredLeft);
    r.translate(0, r.getHeight());
}

void Direct2DDemoEditor::paintStats(juce::Graphics& /*g*/)
{
#if JUCE_DIRECT2D && JUCE_DIRECT2D_METRICS
    g.setFont(15.0f);
    g.setColour(juce::Colours::white);

    juce::Rectangle<int> r{ 0, getHeight() - 100, getWidth(), 20 };

    paintStat(g, r, "Timer interval (ms): ", timingSource.measuredTimerIntervalSeconds);
    r.translate(0, r.getHeight());

    switch (audioProcessor.parameters.renderer)
    {
    case RenderMode::software:
        if (auto peer = getPeer())
        {
            paintFrameIntervalStats(g, r, peer->measuredPaintIntervalSeconds);
            paintFrameDurationStats(g, r, peer->measuredPaintDurationSeconds);
            paintWmPaintCount(g, r, peer->paintCount);
        }
        break;

    case RenderMode::vblankAttachmentDirect2D:
    case RenderMode::dedicatedThreadDirect2D:
        paintFrameIntervalStats(g, r, d2dAttachment.measuredFrameIntervalSeconds);
        paintFrameDurationStats(g, r, d2dAttachment.measuredFrameDurationSeconds);
        paintWmPaintCount(g, r, d2dAttachment.wmPaintCount);
        break;
    }
#endif
}

void Direct2DDemoEditor::resized()
{
    settingsComponent.setBounds(getWidth() - 30, getHeight() - 30, 500, 200);
    settingsComponent.cornerBounds = settingsComponent.getBounds();

    juce::BorderSize borders{ 50 };
    juce::Rectangle<int> r = borders.subtractedFrom(getLocalBounds());
    r.setHeight(r.proportionOfHeight(0.8f / (float)(childWindows.size() + 1)));
    r.translate(0, r.getHeight() + 40);
    for (auto window : childWindows)
    {
        window->setBounds(r);
        r.translate(0, r.getHeight() + 10);
    }
}

void Direct2DDemoEditor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property)
{
    DBG(treeWhosePropertyHasChanged.getType().toString() << "[" << treeWhosePropertyHasChanged[property].toString() << "]");
    ignoreUnused(property);

    auto const parameterID = treeWhosePropertyHasChanged["id"].toString();

    if (parameterID == audioProcessor.frameRateID)
    {
        updateFrameRate();
        return;
    }

    if (parameterID == audioProcessor.rendererID)
    {
        updateRenderer();
        return;
    }
}

void Direct2DDemoEditor::resetStats()
{
#if JUCE_DIRECT2D && JUCE_DIRECT2D_METRICS
    if (auto peer = getPeer())
    {
        peer->resetStats();
    }

    timingSource.resetStats();
    d2dAttachment.resetStats();
#endif
}

void Direct2DDemoEditor::updateFrameRate()
{
    //
    // Frame rate changed; tell the timing source
    //
    if (auto framesPerSecond = audioProcessor.parameters.frameRate.get(); framesPerSecond > 0.0)
    {
        timingSource.setFrameRate(framesPerSecond);
        resetStats();
    }
}

void Direct2DDemoEditor::updateRenderer()
{
    //
    // Renderer change; stop the timing source
    //
    timingSource.stopAllTimers();

    resetStats();

#if JUCE_OPENGL
    openGLContext = nullptr;
#endif

    parentHierarchyChanged();
}
