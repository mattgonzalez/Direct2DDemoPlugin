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

#include "AnimationDisplay.h"

AnimationDisplay::AnimationDisplay(Direct2DDemoProcessor& processor_, Direct2DAttachment& direct2DAttachment_) :
    audioProcessor(processor_),
    direct2DAttachment(direct2DAttachment_),
    frizAnimator(std::make_unique<friz::AsyncController>())
{
    auto animation = std::make_unique<friz::Animation<1>>(1);
    //animation->setValue(0, std::make_unique<friz::Spring>(0.2f, 0.8f, 0.05f, 1.02f, 0.9f));
    auto frameRate = juce::roundToInt(processor_.parameters.frameRate.get());
    animation->setValue(0, std::make_unique<friz::Linear>(0.0f, 1.0f, 3000));

    if (auto updater = dynamic_cast<friz::UpdateSource<1>*>(animation.get()))
    {
        updater->onUpdate([this](int id, const friz::Animation<1>::ValueList& values)
            {
                auto value = values.front();
                sprite.position = value;
            });
    }
        
    setFrameRate(frameRate);
    frizAnimator.addAnimation(std::move(animation));
}

void AnimationDisplay::setFrameRate(int frameRateHz)
{
    juce::ScopedLock locker{ direct2DAttachment.getLock() };

    frizAnimator.setFrameRate(frameRateHz);
}

void AnimationDisplay::paint(juce::Graphics& g, juce::Rectangle<float> bounds, ProcessorOutput const* const processorOutput, double timeSeconds)
{
    auto elapsed = juce::roundToInt(timeSeconds * 1000.0);
    frizAnimator.gotoTime(elapsed);

    sprite.paint(g, bounds);
}

void AnimationDisplay::Sprite::paint(juce::Graphics& g, juce::Rectangle<float> paintArea)
{
    g.setColour(juce::Colours::magenta);
    auto w = paintArea.getWidth() - bounds.getWidth();
    g.fillRect(bounds.withX(position * w).translated(paintArea.getX(), paintArea.getY()));
}
