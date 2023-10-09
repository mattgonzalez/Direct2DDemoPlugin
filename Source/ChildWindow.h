#pragma once

class ChildWindow : public juce::HWNDComponent
{
public:
    enum class Mode
    {
        gdi,
        direct2D,
        openGL
    };

    ChildWindow(Mode mode_) :

        inner(*this)
    {
        switch (mode_)
        {
        case Mode::gdi:
        case Mode::direct2D:
        {
            inner.getProperties().set("Direct2D", Mode::direct2D == mode_);
            inner.addToDesktop(0);
            inner.setVisible(true);

            if (auto* innerPeer = inner.getPeer())
                setHWND(innerPeer->getNativeHandle()); //, HWNDComponent::Type::child);

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
    }

    ~ChildWindow() override
    {
        if (glContext)
        {
            glContext->detach();
        }
    }

    void paint(juce::Graphics&g) override
    {
        g.fillAll(juce::Colours::aquamarine);
        g.setColour(juce::Colours::black);
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

    std::unique_ptr<juce::OpenGLContext> glContext;
    struct Inner : public Component
    {
        Inner(ChildWindow& owner_) :
            owner(owner_)
        {
            setOpaque(true);
        }
    
        void paint(juce::Graphics& g) override 
        {
            auto ticks = juce::Time::getHighResolutionTicks();
            auto elapsed = juce::Time::highResolutionTicksToSeconds(ticks - lastPaintTicks);
            lastPaintTicks = ticks;
            phase.advance(juce::MathConstants<double>::twoPi * animationPeriodSeconds * elapsed);

            g.fillAll(juce::Colour::greyLevel(0.2f));
            g.setColour(juce::Colours::magenta);
            g.drawRect(getLocalBounds());

            if (auto peer = getPeer())
            {
                g.setColour(juce::Colours::white);
                //g.setFont(getHeight() * (0.25f * (float)std::sin(phase.phase) + 0.8f));
                g.setFont(getHeight() * 0.8f);

                juce::String text{ getPeer()->getCurrentRenderingEngine() > 0 ? "Direct2D" : "GDI" };
                if (owner.glContext)
                {
                    text = "OpenGL";
                }
                g.addTransform(juce::AffineTransform::scale(1.0f + 0.25f * (float)std::sin(phase.phase), 1.0f));
                g.drawText(text, getLocalBounds(), juce::Justification::centredLeft);
            }
        }

        ChildWindow& owner;
        juce::dsp::Phase<double> phase;
        int64_t lastPaintTicks = juce::Time::getHighResolutionTicks();
        static constexpr double animationPeriodSeconds = 0.1;
    } inner;
};