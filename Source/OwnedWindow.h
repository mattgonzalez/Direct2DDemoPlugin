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

class OwnedWindow : public juce::HWNDComponent
{
public:
    enum class Mode
    {
        softwareRenderer,
        direct2D,
        openGL
    };

    OwnedWindow(Direct2DDemoProcessor& processor_, Mode mode_) :
        inner(*this),
        processor(processor_),
        mode(mode_)
    {
    }

    ~OwnedWindow() override
    {
        if (glContext)
        {
            glContext->detach();
        }
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
                inner.addToDesktop(juce::ComponentPeer::windowIsOwned, ancestorPeer->getNativeHandle());
                inner.setVisible(true);

                if (auto* innerPeer = inner.getPeer())
                {
                    innerPeer->setCurrentRenderingEngine((int)mode);
                    setHWND(innerPeer->getNativeHandle());
                }
                break;
            }

            case Mode::openGL:
            {
                addAndMakeVisible(inner);

                glContext = std::make_unique<juce::OpenGLContext>();
                glContext->setComponentPaintingEnabled(true);
                glContext->setContinuousRepainting(true);
                glContext->attachTo(*this);
                break;
            }
            }

            resized();
        }
    }

    void paint(juce::Graphics&g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::white);
        g.drawText(getName(), getLocalBounds(), juce::Justification::topLeft);
    }

    void resized() override
    {
        if (glContext)
        {
            inner.setBounds(getLocalBounds());
        }
        else
        {
            updateHWNDBounds();
        }
    }

    Direct2DDemoProcessor& processor;
    Mode const mode;

    static void paintSpectrum(juce::Graphics& g, juce::Rectangle<float> area, double phase, juce::StringRef text, RealSpectrum<float> const& spectrum)
    {
        juce::ColourGradient gradient{ juce::Colours::black,
            0.0f, 0.0f,
            juce::Colours::black,
            area.getWidth(), 0.0f, false };
        gradient.addColour(0.5f + 0.5f * (float)std::sin(phase), juce::Colour::greyLevel(0.1f));
        g.setGradientFill(gradient);

        g.setFont(area.getHeight() * 0.8f);
        g.drawText(text, area, juce::Justification::centredLeft);

        int numBins = spectrum.getNumBins();
        float pixelsPerBin = area.getWidth() / numBins;
        float maxBarHeight = area.getHeight() / 2;
        float y = 0.0f;
        juce::Range<float> decibelRange{ -100.0f, 0.0f };
        float yScale = maxBarHeight / decibelRange.getLength();
        juce::RectangleList<int> bars;
        for (int channel = 0; channel < 2; ++channel)
        {
            int bin = 0;
            float x = 0.0f;
            while (x < area.getWidth() && bin < spectrum.getNumBins())
            {
                auto value = spectrum.getBinValue(channel, bin);
                auto mag = juce::Decibels::gainToDecibels(std::abs(value));
                float h = (mag - decibelRange.getStart()) * yScale;
                bars.add(juce::Rectangle<float>{ x, y + maxBarHeight - h, pixelsPerBin, h }.getSmallestIntegerContainer());
                ++bin;
                x += pixelsPerBin;
            }

            y += maxBarHeight;
        }

        g.reduceClipRegion(bars);
        g.setColour(juce::Colours::white);
        g.drawText(text, area, juce::Justification::centredLeft);
    }

    std::unique_ptr<juce::OpenGLContext> glContext;
    struct Inner : public Component
    {
        Inner(OwnedWindow& ownedWindow_) :
            ownedWindow(ownedWindow_)
        {
            setOpaque(false);
        }
    
        void paint(juce::Graphics& g) override 
        {
            auto ticks = juce::Time::getHighResolutionTicks();
            auto elapsed = juce::Time::highResolutionTicksToSeconds(ticks - lastPaintTicks);
            lastPaintTicks = ticks;
            phase.advance(juce::MathConstants<double>::twoPi * animationPeriodSeconds * elapsed);

            g.fillAll(juce::Colour::greyLevel(0.1f));
            g.setColour(juce::Colours::cyan);
            g.drawRect(getLocalBounds());

            if (auto peer = getPeer())
            {
                juce::String text{ getPeer()->getCurrentRenderingEngine() > 0 ? "Direct2D" : "Software" };
                if (ownedWindow.glContext)
                {
                    text = "OpenGL";
                }
                if (auto output = ownedWindow.processor.outputFIFO.getMostRecent())
                {
                    paintSpectrum(g, getLocalBounds().toFloat(), phase.phase, text, output->averageSpectrum);
                }
            }


#if 0


            g.fillAll(juce::Colours::grey.withAlpha(0.5f));
            g.setColour(juce::Colours::cyan);
            g.drawRect(getLocalBounds());

#endif
        }

        OwnedWindow& ownedWindow;
        juce::dsp::Phase<double> phase;
        int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
        static constexpr double animationPeriodSeconds = 0.2;
    } inner;
};