#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AE8LookAndFeel : public juce::LookAndFeel_V4
{
public:
    AE8LookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f, juce::Font::bold)); }
    juce::Font getPopupMenuFont() override { return juce::Font (juce::FontOptions (14.0f)); }
};

class AE8Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AE8Editor (AE8Processor&);
    ~AE8Editor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refreshPresetBox();

    struct Control
    {
        int param;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
    };
    struct Section
    {
        juce::String title;
        juce::Colour colour;
        std::vector<int> params;
        juce::Rectangle<int> bounds;
    };

    AE8Processor& proc;
    AE8LookAndFeel lnf;
    std::vector<Section> sections;
    std::vector<std::unique_ptr<Control>> controls;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::MidiKeyboardComponent keyboard;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AE8Editor)
};
