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

#include "Direct2DDemoProcessor.h"
#include "Direct2DDemoEditor.h"

Direct2DDemoEditor::Direct2DDemoEditor(Direct2DDemoProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    threadMessages(d2dAttachment.getLock()),
    timingSource(this, audioProcessor.readyEvent, threadMessages),
    settingsComponent(p)
{
    setResizable(true, false);

    int width = 1000;
    int height = 1000;
    setSize(width, height);

    addAndMakeVisible(settingsComponent);

    audioProcessor.state.state.addListener(this);

    painter = std::make_unique<SpectrumRingDisplay>(p, d2dAttachment);

    timingSource.onPaintTimer = [this]() { paintTimerCallback(); };

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
    if (RenderMode::software == audioProcessor.parameters.renderer)
    {
        repaint();
        return;
    }

    d2dAttachment.paintImmediately();
}

void Direct2DDemoEditor::paint(juce::Graphics& g)
{
    if (getWidth() <= 0)
    {
        return;
    }

    if (audioProcessor.outputRingBuffer.getNumItemsStored() > 0)
    {
        //energyPaintSpectrum.copyFrom(processorOutput->energySpectrum);
        audioProcessor.outputRingBuffer.advanceReadPosition();
    }

    g.fillAll(juce::Colours::black);

    painter->paint(g, getLocalBounds().toFloat(), audioProcessor.outputRingBuffer.getMostRecent());

    paintModeText(g);
    paintStats(g);
}

void Direct2DDemoEditor::paintModeText(juce::Graphics& g)
{
    g.setFont(20.0f);
    g.setColour(juce::Colours::white);
    juce::String text;
    switch (audioProcessor.parameters.renderer)
    {
    case RenderMode::software:
        text = "Software renderer ";
        break;

    case RenderMode::vblankAttachmentDirect2D:
    case RenderMode::dedicatedThreadDirect2D:
        text = "Direct2D ";
        break;
    }

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

static void paintStat(juce::Graphics& g, juce::Rectangle<int> const r, juce::String name, juce::StatisticsAccumulator<double> const& stats)
{
    name << juce::String{ stats.getAverage() * 1000.0, 1 } << " avg / " << juce::String{ stats.getStandardDeviation() * 1000.0, 1 } << " std.dev.";
    g.drawText(name, r, juce::Justification::centredLeft);
}

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
            g.drawText(juce::String{ 1.0 / averageFrameInterval, 1 }, r.translated(50, 0), juce::Justification::centredLeft);
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

void Direct2DDemoEditor::paintStats(juce::Graphics& g)
{
#if JUCE_DIRECT2D
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
    d2dAttachment.paintImmediately();

    settingsComponent.setBounds(getWidth() - 30, getHeight() - 30, 500, 200);
    settingsComponent.cornerBounds = settingsComponent.getBounds();
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
#if JUCE_DIRECT2D
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
    if (auto framesPerSecond = audioProcessor.parameters.frameRate.get(); framesPerSecond > 0.0)
    {
        threadMessages.postMessage([this, framesPerSecond]()
            {
                timingSource.setFrameRate(framesPerSecond);
                resetStats();
            });
    }
}

void Direct2DDemoEditor::updateRenderer()
{
    timingSource.stopAllTimers();
    d2dAttachment.detach();

    resetStats();

    int renderMode = audioProcessor.parameters.renderer;
    switch (renderMode)
    {
    case RenderMode::software:
    {
        break;
    }

    case RenderMode::vblankAttachmentDirect2D:
    case RenderMode::dedicatedThreadDirect2D:
    {
        bool wmPaintEnabled = true;
        d2dAttachment.attach(this, wmPaintEnabled);
    }
    }

    updateFrameRate();
    timingSource.setMode(renderMode);
}
