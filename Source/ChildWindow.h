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

class ChildWindow : public juce::HWNDComponent
{
public:
    enum class Mode
    {
        softwareRenderer,
        direct2D,
        openGL
    };

    ChildWindow(Direct2DDemoProcessor& processor_, Mode mode_) :
        inner(*this),
        processor(processor_),
        mode(mode_)
    {
        setOpaque(true);
        inner.addToDesktop(0, nullptr);
        inner.setVisible(true);
    }

    ~ChildWindow() override
    {
#if JUCE_OPENGL
        if (glContext)
        {
            glContext->detach();
        }
#endif
    }

    void parentHierarchyChanged() override
    {
        if (auto ancestorPeer = getPeer())
        {
            switch (mode)
            {
            case Mode::softwareRenderer:
            case Mode::direct2D:
            {
                if (auto* innerPeer = inner.getPeer())
                {
                    if (innerPeer->getNativeHandle() != getHWND())
                    {
                        innerPeer->setCurrentRenderingEngine((int)mode);
                        setHWND(innerPeer->getNativeHandle());
                    }
                }
                break;
            }

#if JUCE_OPENGL
            case Mode::openGL:
            {
                addAndMakeVisible(inner);

                glContext = std::make_unique<juce::OpenGLContext>();
                glContext->setComponentPaintingEnabled(true);
                glContext->setContinuousRepainting(true);
                glContext->attachTo(*this);
                break;
            }
#endif
            }

            resized();
        }
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText(getName(), getLocalBounds(), juce::Justification::topLeft);
    }

    Direct2DDemoProcessor& processor;
    Mode const mode;

    static void paintSpectrum(juce::Graphics& g, juce::Rectangle<float> area, juce::StringRef bigText, juce::StringRef smallText, RealSpectrum<float> const& spectrum)
    {
        g.setColour(juce::Colours::white);
        g.setFont(20.0f);
        g.drawText(smallText, area, juce::Justification::topLeft);

        g.setColour(juce::Colours::black);
        g.setFont(area.getHeight() * 0.6f);
        g.drawText(bigText, area, juce::Justification::centredLeft);

        int numBins = juce::roundToInt(spectrum.getNumBins() * 0.4);
        float pixelsPerBin = area.getWidth() / numBins;
        float barBottom = area.getHeight() * 0.9f + area.getY();
        float maxBarHeight = area.getHeight() * 0.6f;
        float y = barBottom - maxBarHeight;
        juce::Range<float> decibelRange{ -100.0f, 0.0f };
        float yScale = maxBarHeight / decibelRange.getLength();
        juce::RectangleList<int> bars;

        if (spectrum.getNumChannels() <= 0)
        {
            return;
        }

        float const numChannelsInverse = 1.0f / (float)spectrum.getNumChannels();
        int bin = 0;
        float x = 0.0f;
        while (x < area.getWidth() && bin < numBins)
        {
            float value = 0.0f;
            for (int channel = 0; channel < spectrum.getNumChannels(); ++channel)
            {
                value += spectrum.getBinValue(channel, bin);
            }
            auto mag = juce::Decibels::gainToDecibels(std::abs(value) * numChannelsInverse);

            float h = (mag - decibelRange.getStart()) * yScale;
            bars.add(juce::Rectangle<float>{ x, y + maxBarHeight - h, pixelsPerBin, h }.getSmallestIntegerContainer());

            ++bin;
            x += pixelsPerBin;
        }

        g.reduceClipRegion(bars);
        g.setColour(juce::Colours::magenta);
        juce::ColourGradient gradient{ juce::Colours::cyan,
            0.0f, area.getBottom(),
            juce::Colours::magenta,
            0.0f, area.getY(), false };
        g.setGradientFill(gradient);
        g.drawText(bigText, area, juce::Justification::centredLeft);
    }

#if JUCE_OPENGL
    std::unique_ptr<juce::OpenGLContext> glContext;
#endif

    struct Inner : public Component
    {
        Inner(ChildWindow& owner_) :
            owner(owner_)
        {
            setName("inner");
            setOpaque(true);
        }

        void paint(juce::Graphics& g) override
        {
            //             auto ticks = juce::Time::getHighResolutionTicks();
            //             auto elapsed = juce::Time::highResolutionTicksToSeconds(ticks - lastPaintTicks);
            //             lastPaintTicks = ticks;
            //             phase.advance(juce::MathConstants<double>::twoPi * animationPeriodSeconds * elapsed);

            g.fillAll(juce::Colour::greyLevel(0.1f));
            g.setColour(juce::Colours::cyan);
            g.drawRect(getLocalBounds());

            if (auto peer = getPeer())
            {
                juce::String text{ getPeer()->getCurrentRenderingEngine() > 0 ? "Direct2D" : "Software" };
#if JUCE_OPENGL
                if (owner.glContext)
                {
                    text = "OpenGL";
                }
#endif
                if (auto output = owner.processor.outputFIFO.getMostRecent())
                {
                    paintSpectrum(g, getLocalBounds().toFloat(), text, "Owned window", output->averageSpectrum);
                }
            }
        }

        ChildWindow& owner;
        //juce::dsp::Phase<double> phase;
        //int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
        //static constexpr double animationPeriodSeconds = 0.2;
    } inner;
};