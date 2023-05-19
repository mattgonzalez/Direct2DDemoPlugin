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
