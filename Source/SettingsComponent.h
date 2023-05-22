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

class SettingsComponent : public juce::Component, public juce::LookAndFeel_V4
{
public:
    SettingsComponent(Direct2DDemoProcessor& processor_);
    ~SettingsComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void paintDiagram(juce::Graphics& g);

    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox& box, juce::Label& label) override;
    juce::Rectangle<int> getPropertyComponentContentPosition(juce::PropertyComponent&) override;

    juce::Rectangle<int> cornerBounds;
    juce::ComponentAnimator animator;
    juce::PropertyPanel panel;

private:
    Direct2DDemoProcessor& processor;

    struct DiagramText
    {
        juce::String upperText;
        juce::String lowerText;
    };

    using DiagramEntry = juce::Array<DiagramText>;
    juce::Array<DiagramEntry> const diagrams
    {
        DiagramEntry
        {
            DiagramText
            {
                "VSyncThread", "VSync"
            },
            {
                "VBlankAttachment", "repaint()"
            },
            {
                "WM_PAINT", "Software paint()"
            }
        },

        DiagramEntry
        {
            DiagramText
            {
                "VSyncThread", "VSync"
            },
            {
                "VBlankAttachment", "Direct2D paint()"
            }
        },

        DiagramEntry
        {
            DiagramText
            {
                "AudioIODevice callback", "Signal paint event"
            },
            {
                "Dedicated render thread", "Direct2D paint()"
            }
        }
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)
};
