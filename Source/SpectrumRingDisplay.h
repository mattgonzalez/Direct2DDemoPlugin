#pragma once

#include <JuceHeader.h>
#include "Direct2DDemoProcessor.h"
#include "Direct2DAttachment.h"

class SpectrumRingDisplay
{
public:
    SpectrumRingDisplay(Direct2DDemoProcessor& processor_, Direct2DAttachment& direct2DAttachment_, juce::AudioBuffer<float> const& energyPaintBuffer_);
    ~SpectrumRingDisplay() = default;

    void paint(juce::Graphics& g, juce::Rectangle<int> bounds);

protected:
    Direct2DDemoProcessor& audioProcessor;
    Direct2DAttachment& direct2DAttachment;
    juce::AudioBuffer<float> const& energyPaintBuffer;
    
    juce::OwnedArray<juce::Path> segmentPaths;
    juce::Rectangle<int> ringBounds;
    juce::ColourGradient gradient;
};
