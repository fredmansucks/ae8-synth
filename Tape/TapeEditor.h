#pragma once
#include <JuceHeader.h>
#include "TapeProcessor.h"
#include "../Shared/Brand.h"

// Brand look plus vertical faders and a lit toggle.
class TapeLookAndFeel : public ae8brand::LookAndFeel
{
public:
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

// Two spinning reels, the tape path across the head block, and the speed readout.
class ReelsView : public juce::Component
{
public:
    explicit ReelsView (TapeProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
private:
    void drawReel (juce::Graphics&, juce::Point<float> c, float R, float packR, float angle);
    TapeProcessor& proc;
};

class TapeEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TapeEditor (TapeProcessor&);
    ~TapeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawMeter (juce::Graphics&, juce::Rectangle<int>, float l, float r, const juce::String& label);

    struct Ctl
    {
        int param;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ToggleButton> toggle;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bAtt;
    };
    struct Section { juce::String title, tag; juce::Colour colour; bool light; std::vector<int> params; juce::Rectangle<int> bounds; };

    Ctl& add (int param, juce::Colour col, bool fader, const juce::String& name);
    Ctl* find (int param);

    TapeProcessor& proc;
    TapeLookAndFeel lnf;
    ae8brand::Scaler scaler;
    std::vector<std::unique_ptr<Ctl>> ctls;
    std::vector<Section> bottom;
    ReelsView reels;
    juce::ComboBox presetBox;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::Rectangle<int> headerArea, recArea, transArea, outArea, inMeter, outMeter;
    float inL = 0, inR = 0, outL = 0, outR = 0;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeEditor)
};
