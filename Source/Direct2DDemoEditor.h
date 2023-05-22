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
