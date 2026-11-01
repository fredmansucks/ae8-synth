#pragma once
#include <JuceHeader.h>
#include "TuneProcessor.h"
#include "../Shared/Brand.h"

// Scrolling pitch display: input pitch (white dots) vs corrected pitch (flux line) over a note grid.
class PitchView : public juce::Component
{
public:
    explicit PitchView (TuneProcessor& p) : proc (p) {}
    void push();
    void paint (juce::Graphics&) override;
private:
    TuneProcessor& proc;
    static constexpr int kN = 150;            // 5 s at 30 fps
    std::array<float, kN> inH {}, outH {};
    std::array<bool, kN> vH {};
    int head = 0;
    float centre = 60.0f;
};

// Twelve key buttons: root highlighted, scale notes lit, the note being sung outlined.
class KeyStrip : public juce::Component
{
public:
    explicit KeyStrip (TuneProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    TuneProcessor& proc;
};

class TuneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TuneEditor (TuneProcessor&);
    ~TuneEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        int param;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    };
    struct Section { juce::String title, tag; juce::Colour colour; bool light; std::vector<int> params; juce::Rectangle<int> bounds; };

    TuneProcessor& proc;
    ae8brand::LookAndFeel lnf;
    std::vector<Section> sections;
    std::vector<std::unique_ptr<Knob>> knobs;
    PitchView pitchView;
    KeyStrip keyStrip;
    juce::ComboBox scaleBox, styleBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> scaleAtt;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::Rectangle<int> headerArea, tunerArea;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneEditor)
};
