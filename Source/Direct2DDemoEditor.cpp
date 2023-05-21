#include <windows.h>
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

#if JUCE_DEBUG
    valueTreeDisplay.addTree(audioProcessor.state.state);
#endif

    painter = std::make_unique<SpectrumRingDisplay>(p, d2dAttachment, energyPaintSpectrum);
    energyPaintSpectrum = RealSpectrum<float>{}.withChannels(2).withFFTSize(p.getFFTLength());
    energyPaintSpectrum.clear();

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
        auto processorOutput = audioProcessor.outputRingBuffer.getMostRecent();
        energyPaintSpectrum.copyFrom(processorOutput->energySpectrum);
        audioProcessor.outputRingBuffer.advanceReadPosition();
    }

    g.fillAll(juce::Colours::black);

    painter->paint(g, getLocalBounds());

    paintModeText(g);
    paintStats(g);
}

void Direct2DDemoEditor::paintModeText(juce::Graphics& g)
{
    g.setFont({ 20.0f });
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

void Direct2DDemoEditor::paintPaintStats(juce::Graphics& g,
    juce::Rectangle<int>& r,
    juce::StatisticsAccumulator<double> const& frameIntervalSeconds,
    juce::StatisticsAccumulator<double> const& frameDurationSeconds, 
    int wmPaintCount)
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

    {
        juce::Colour averageDurationColor = juce::Colours::white;
        auto averageDurationSeconds = frameDurationSeconds.getAverage();
        if (averageDurationSeconds > averageFrameIntervalSeconds * 1.0)
        {
            averageDurationColor = juce::Colours::red;
        }
        else if (averageDurationSeconds > averageFrameIntervalSeconds * 0.8)
        {
            averageFrameIntervalColor = juce::Colours::yellow;
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

    g.setColour(juce::Colours::white);
    g.drawText("# WM_PAINT: " + juce::String{ wmPaintCount }, r, juce::Justification::centredLeft);
    r.translate(0, r.getHeight());
}

void Direct2DDemoEditor::paintStats(juce::Graphics& g)
{
    g.setFont({ 18.0f });
    g.setColour(juce::Colours::white);

    juce::Rectangle<int> r{ 0, getHeight() - 125, getWidth(), 25 };

    paintStat(g, r, "Timer interval (ms): ", timingSource.measuredTimerIntervalSeconds);
    r.translate(0, r.getHeight());

    switch (audioProcessor.parameters.renderer)
    {
    case RenderMode::software:
        if (auto peer = getPeer())
        {
            paintPaintStats(g, r, peer->measuredPaintIntervalSeconds, peer->measuredPaintDurationSeconds, peer->paintCount);
        }
        break;

    case RenderMode::vblankAttachmentDirect2D:
    case RenderMode::dedicatedThreadDirect2D:
        paintPaintStats(g, r, d2dAttachment.measuredFrameIntervalSeconds, d2dAttachment.measuredFrameDurationSeconds, d2dAttachment.wmPaintCount);
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
