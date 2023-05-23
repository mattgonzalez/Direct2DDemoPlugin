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
#include "friz/friz.h"

class AnimationDisplay
{
public:
    AnimationDisplay(Direct2DDemoProcessor& processor_, Direct2DAttachment& direct2DAttachment_);
    ~AnimationDisplay() = default;

    void setFrameRate(int frameRateHz);
    void paint(juce::Graphics& g, juce::Rectangle<float> bounds, ProcessorOutput const* const processorOutput, double timeSeconds);

protected:
    Direct2DDemoProcessor& audioProcessor;
    Direct2DAttachment& direct2DAttachment;
    friz::Animator frizAnimator;

    struct Sprite
    {
        void paint(juce::Graphics& g, juce::Rectangle<float> paintArea);

        juce::Rectangle<float> bounds{ 10.f, 10.0f, 50.0f, 50.0f };
        float position = 0.0f;
    } sprite;
};
