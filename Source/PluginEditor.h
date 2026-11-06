#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../Shared/Brand.h"

// Premium keyboard: ivory keys with a soft gradient, gloss black keys, lit key-down.
class AE8Keyboard : public juce::MidiKeyboardComponent
{
public:
    using juce::MidiKeyboardComponent::MidiKeyboardComponent;
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour, juce::Colour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour) override;
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
        juce::String title, tag;
        juce::Colour colour;
        bool lightText;
        std::vector<int> params;
        juce::Rectangle<int> bounds;
    };

    AE8Processor& proc;
    ae8brand::LookAndFeel lnf;
    ae8brand::Scaler scaler;
    std::vector<Section> sections;
    std::vector<std::unique_ptr<Control>> controls;
    juce::Path wordmark;
    juce::Rectangle<int> headerArea, markArea, wordArea, displayArea;

    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    AE8Keyboard keyboard;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AE8Editor)
};
