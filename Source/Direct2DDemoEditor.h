#pragma once

#include <JuceHeader.h>
#include "Direct2DDemoProcessor.h"
#include "Direct2DAttachment.h"
#include "SettingsComponent.h"
#include "ValueTreeDisplay.h"
#include "TimingSource.h"
#include "SpectrumRingDisplay.h"

class Direct2DDemoEditor : public juce::AudioProcessorEditor,
    public juce::ValueTree::Listener
{
public:
    Direct2DDemoEditor(Direct2DDemoProcessor&);
    ~Direct2DDemoEditor() override;

    void paintTimerCallback();

    void paint(juce::Graphics&) override;
    void paintWatermark(juce::Graphics& g);
    void paintStats(juce::Graphics& g);

    void resized() override;
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;

    void resetStats();

private:
    Direct2DDemoProcessor& audioProcessor;
    Direct2DAttachment d2dAttachment;
    TimingSource timingSource;
    SettingsComponent settingsComponent;
#if JUCE_DEBUG
    ValueTreeDisplayWindow valueTreeDisplay;
#endif
    juce::AudioBuffer<float> energyPaintBuffer;
    std::unique_ptr<SpectrumRingDisplay> painter;

    void updateFrameRate();
    void updateRenderer();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Direct2DDemoEditor)
};
