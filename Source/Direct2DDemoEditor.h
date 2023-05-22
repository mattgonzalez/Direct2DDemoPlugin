#pragma once

#include <JuceHeader.h>
#include "Direct2DDemoProcessor.h"
#include "Direct2DAttachment.h"
#include "SettingsComponent.h"
#include "TimingSource.h"
#include "SpectrumRingDisplay.h"
#include "ThreadMessageQueue.h"

class Direct2DDemoEditor : public juce::AudioProcessorEditor,
    public juce::ValueTree::Listener
{
public:
    Direct2DDemoEditor(Direct2DDemoProcessor&);
    ~Direct2DDemoEditor() override;

    void paintTimerCallback();

    void paint(juce::Graphics&) override;

    void resized() override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void resetStats();

private:
    Direct2DDemoProcessor& audioProcessor;
    Direct2DAttachment d2dAttachment;
    ThreadMessageQueue threadMessages;
    TimingSource timingSource;
    SettingsComponent settingsComponent;
    RealSpectrum<float> energyPaintSpectrum;
    std::unique_ptr<SpectrumRingDisplay> painter;

    void updateFrameRate();
    void updateRenderer();

    void paintModeText(juce::Graphics& g);
    void paintFrameIntervalStats(juce::Graphics& g, juce::Rectangle<int>& r, juce::StatisticsAccumulator<double> const& frameIntervalSeconds);
    void paintFrameDurationStats(juce::Graphics& g, juce::Rectangle<int>& r, juce::StatisticsAccumulator<double> const& frameDurationSeconds);
    void paintWmPaintCount(juce::Graphics& g, juce::Rectangle<int>& r, int wmPaintCount);
    void paintStats(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Direct2DDemoEditor)
};
