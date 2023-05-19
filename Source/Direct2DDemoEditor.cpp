#include <windows.h>
#include "Direct2DDemoProcessor.h"
#include "Direct2DDemoEditor.h"

Direct2DDemoEditor::Direct2DDemoEditor(Direct2DDemoProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    timingSource(this, audioProcessor.readyEvent),
    settingsComponent(p),
    energyPaintBuffer(2, p.getNumBins())
{
    setResizable(true, false);

    int width = 1000;
    int height = 1000;
    setSize(width, height);

    addAndMakeVisible(settingsComponent);

    audioProcessor.state.state.addListener(this);

#if JUCE_DEBUG
    valueTreeDisplay.addTree(audioProcessor.state.state);
#endif

    painter = std::make_unique<SpectrumRingDisplay>(p, d2dAttachment, energyPaintBuffer);

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

static juce::String printStats(juce::String name, juce::StatisticsAccumulator<double> const& stats)
{
    return name << juce::String{ stats.getAverage() * 1000.0, 1 } << " avg / " << juce::String{ stats.getStandardDeviation() * 1000.0, 1 } << " std.dev.";
}

void Direct2DDemoEditor::paint(juce::Graphics& g)
{
    if (getWidth() <= 0)
    {
        return;
    }

    g.fillAll(juce::Colours::black);

    for (int channel = 0; channel < energyPaintBuffer.getNumChannels(); ++channel)
    {
        energyPaintBuffer.copyFrom(channel, 0, audioProcessor.getEnergySpectrum(), channel, 0, energyPaintBuffer.getNumSamples());
    }

    painter->paint(g, getLocalBounds());

    paintWatermark(g);
    paintStats(g);
}

void Direct2DDemoEditor::paintWatermark(juce::Graphics& g)
{
    g.setFont({ 24.0f });
    g.setColour(juce::Colours::grey);
    juce::String text;
    switch (audioProcessor.parameters.renderer)
    {
        case RenderMode::software:
            text = "Software render";
            break;

        case RenderMode::vblankAttachmentDirect2D:
        case RenderMode::dedicatedThreadDirect2D:
            text = "Direct2D";
            break;
    }

#if JUCE_DEBUG
    text << " (debug build)";
#endif

    g.drawText(text, getLocalBounds(), juce::Justification::topLeft);
}

static void printStats(juce::Graphics& g,
    juce::Rectangle<int> r,
    juce::StatisticsAccumulator<double> const& frameIntervalSeconds,
    juce::StatisticsAccumulator<double> const& frameDurationSeconds)
{
    if (auto average = frameIntervalSeconds.getAverage(); average > 0.0)
    {
        g.drawText("FPS: " + juce::String{ 1.0 / average, 1 }, r, juce::Justification::centredLeft, false);
    }
    r.translate(0, 25);
    g.drawText(printStats("Paint interval (ms): ", frameIntervalSeconds), r, juce::Justification::centredLeft, false);
    r.translate(0, 25);
    g.drawText(printStats("Paint duration (ms): ", frameDurationSeconds), r, juce::Justification::centredLeft, false);
}

void Direct2DDemoEditor::paintStats(juce::Graphics& g)
{
    g.setFont({ 14.0f });
    g.setColour(juce::Colours::white);

    juce::Rectangle<int> r{ 0, getHeight() - 125, getWidth(), 25 };

    if (d2dAttachment.isAttached())
    {
        g.drawText("# WM_PAINT: " + juce::String{ d2dAttachment.wmPaintCount }, r, juce::Justification::centredLeft, false);
    }
    r.translate(0, 25);
    g.drawText(printStats("Timer interval (ms): ", timingSource.measuredTimerIntervalSeconds), r, juce::Justification::centredLeft, false);
    r.translate(0, 25);

    switch (audioProcessor.parameters.renderer)
    {
    case RenderMode::software:
        if (auto peer = getPeer())
        {
            printStats(g, r, peer->measuredPaintIntervalSeconds, peer->measuredPaintDurationSeconds);
        }
        break;

    case RenderMode::vblankAttachmentDirect2D:
    case RenderMode::dedicatedThreadDirect2D:
        printStats(g, r, d2dAttachment.measuredFrameIntervalSeconds, d2dAttachment.measuredFrameDurationSeconds);
        break;
    }
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
    if (auto peer = getPeer())
    {
        peer->resetStats();
    }

    timingSource.resetStats();
    d2dAttachment.resetStats();
}

void Direct2DDemoEditor::updateFrameRate()
{
    if (auto framesPerSecond = audioProcessor.parameters.frameRate.get(); framesPerSecond > 0.0)
    {
        timingSource.update(framesPerSecond, audioProcessor.parameters.renderer);
    }
}

void Direct2DDemoEditor::updateRenderer()
{
    timingSource.stopAllTimers();
    d2dAttachment.detach();

    resetStats();

    switch (audioProcessor.parameters.renderer)
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
}
