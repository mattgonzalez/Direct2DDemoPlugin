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

class SpectrumRingDisplay
{
public:
    SpectrumRingDisplay(Direct2DDemoProcessor& processor_, Direct2DAttachment& direct2DAttachment_);
    ~SpectrumRingDisplay() = default;

    void paint(juce::Graphics& g, juce::Rectangle<float> bounds, ProcessorOutput const * const processorOutput);

protected:
    Direct2DDemoProcessor& audioProcessor;
    Direct2DAttachment& direct2DAttachment;
    
    juce::OwnedArray<juce::Path> segmentPaths;
    juce::Rectangle<float> ringBounds;
    juce::ColourGradient gradient;

    int const segmentsPerQuarterCircle = 32;
    float const segmentAngleSpacingRadians = 0.8f * juce::MathConstants<float>::halfPi / (float)segmentsPerQuarterCircle;
    float const segmentFilledAngleRadians = segmentAngleSpacingRadians * 0.8f;
    float const segmentAngleSpacingInverse = 1.0f / segmentAngleSpacingRadians;

    void makeSegmentPaths(int numRings, juce::Rectangle<float> bounds);
    void paintSegments(juce::Graphics& g, int channel, int ring, float magnitude, auto const& translateAndScale);
};
