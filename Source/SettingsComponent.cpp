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

#include <JuceHeader.h>
#include "Direct2DDemoProcessor.h"
#include "SettingsComponent.h"
#include "TimingSource.h"

SettingsComponent::SettingsComponent(Direct2DDemoProcessor& processor_) :
    processor(processor_)
{
    auto& parameters = processor_.parameters;

    juce::Array<juce::PropertyComponent*> propertyComponents;
    {
        auto range = processor_.state.getParameterRange(processor_.frameRateID).getRange();
        auto c = std::make_unique<juce::SliderPropertyComponent>(parameters.frameRate.getPropertyAsValue(), "Frame rate", range.getStart(), range.getEnd(), 0.1);
        propertyComponents.add(c.release());
    }

    {
        juce::StringArray strings
    {
            "JUCE software renderer", 
            "Direct2D from VBlankAttachment callback", 
            "Direct2D from dedicated thread" 
        };
        juce::Array<juce::var> values{ RenderMode::software, RenderMode::vblankAttachmentDirect2D, RenderMode::dedicatedThreadDirect2D };

        auto c = std::make_unique<juce::ChoicePropertyComponent>(parameters.renderer.getPropertyAsValue(),
            "Render mode",
            strings,
            values);
        propertyComponents.add(c.release());
    }

    panel.addProperties(propertyComponents);
    addAndMakeVisible(panel);

    setLookAndFeel(this);
}

SettingsComponent::~SettingsComponent()
{
    setLookAndFeel(nullptr);
}

void SettingsComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colour{ 0xff7CDFF7 }.withAlpha(0.8f));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), getHeight() * 0.1f);

    g.setColour(juce::Colours::black);
    g.drawText("Copyright " + juce::String{ juce::CharPointer_UTF8("\xc2\xa9") } + " 2023 Matthew Gonzalez", getLocalBounds().removeFromTop(25), juce::Justification::centred);

    g.setColour(juce::Colours::white);
    if (getBoundsInParent().getX() < getParentComponent()->getWidth() - panel.getX())
    {
        paintDiagram(g);
    }
    else
    {
        g.drawArrow({ 25.0f, 25.0f, 8.0f, 8.0f }, 4.0f, 8.0f, 8.0f);
    }
}

void SettingsComponent::resized()
{
    panel.setBounds(getLocalBounds().reduced(30, 30).withHeight(panel.getTotalContentHeight()));
}

void SettingsComponent::mouseEnter(const juce::MouseEvent& /*event*/)
{
   animator.animateComponent(this, { getParentComponent()->getWidth() - getWidth(), getParentComponent()->getHeight() - getHeight(), getWidth(), getHeight() }, 1.0f, 250, false, 1.0, 1.0);
}

void SettingsComponent::mouseExit(const juce::MouseEvent& /*event*/)
{
    if (!isMouseOver(true))
    {
        animator.animateComponent(this, cornerBounds, 1.0f, 250, false, 1.0, 1.0);
    }
}

void SettingsComponent::paintDiagram(juce::Graphics& g)
{
    juce::String const arrow = juce::CharPointer_UTF8("\xe2\x9e\x9c");

    auto const& diagram = diagrams.getReference(processor.parameters.renderer);
    
    juce::Font lowerFont({ "Segoe UI Symbol", 19.0f, juce::Font::plain });
    juce::Font upperFont = lowerFont.withHeight(21.0f).boldened();

    juce::Rectangle<float> blobR = { 160.0f, 60.0f };
    float xGap = 5.0f;
    float extraWidth = panel.getWidth() - blobR.getWidth() * diagram.size() - xGap * (diagram.size() - 1);
    blobR.setX(extraWidth * 0.5f + panel.getX());
    float upperY = (float)panel.getBottom() - 10.0f;
    float lowerY = upperY + blobR.getHeight() - 10.0f;
    for (int i = 0; i < diagram.size(); ++i)
    {
        auto const& diagramEntry = diagram[i];

        g.setFont(upperFont);
        g.setColour(juce::Colours::black);
        g.drawText(diagramEntry.upperText, blobR.withY(upperY), juce::Justification::centred);

        auto lowerR = blobR.withY(lowerY);
        g.setColour(juce::Colour::greyLevel(0.05f));
        auto roundRectR = lowerR.reduced(20.0f, 0.0f);
        g.fillRoundedRectangle(roundRectR, lowerR.getHeight() * 0.1f);

        if (i < diagram.size() - 1)
        {
            float arrowX = lowerR.getRight() + xGap * 0.1f;
            g.drawArrow({ arrowX - 5.0f, lowerR.getCentreY(), blobR.getRight() + xGap * 0.8f + 10.0f, lowerR.getCentreY() }, 4.0f, 8.0f, 8.0f);
        }

        g.setFont(lowerFont);
        g.setColour(juce::Colours::lightcyan);
        g.drawText(diagramEntry.lowerText, lowerR, juce::Justification::centred);

        blobR.translate(blobR.getWidth() + xGap, 0.0f);
    }
}

juce::PopupMenu::Options SettingsComponent::getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label)
{
    auto options = juce::LookAndFeel_V4::getOptionsForComboBoxPopupMenu(box, label);
    return options.withParentComponent(this);
}

juce::Rectangle<int> SettingsComponent::getPropertyComponentContentPosition(juce::PropertyComponent& component)
{
    return juce::LookAndFeel_V4::getPropertyComponentContentPosition(component).expanded(38, 0).translated(-38, 0);
}
