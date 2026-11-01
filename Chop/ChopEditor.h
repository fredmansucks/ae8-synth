#pragma once
#include <JuceHeader.h>
#include "ChopProcessor.h"
#include "../Shared/Brand.h"

class ChopLookAndFeel : public ae8brand::LookAndFeel
{
public:
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ae8brand::mono (11.0f, true); }
};

// Flat toggle "chip" used across the panel.
class Chip : public juce::Button
{
public:
    Chip (const juce::String& text, juce::Colour on) : juce::Button (text), onColour (on) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
    juce::Colour onColour;
    bool action = false;      // momentary buttons (FIND, COPY…) draw with bright text
};

// A row of chips bound to a choice (or bool) parameter.
class ChoiceChips : public juce::Component
{
public:
    ChoiceChips (juce::RangedAudioParameter&, juce::StringArray labels, juce::Colour on, std::vector<int> values = {});
    void resized() override;
    std::vector<int> widths;
private:
    juce::RangedAudioParameter& param;
    std::vector<std::unique_ptr<Chip>> chips;
    std::vector<int> vals;
    juce::ParameterAttachment att;
    void update (float v);
};

class MidiDragChip : public Chip
{
public:
    explicit MidiDragChip (ChopProcessor& p) : Chip ("MIDI", ae8brand::kInk), proc (p) { setClickingTogglesState (false); action = true; }
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent& e) override { dragging = false; Chip::mouseUp (e); }
private:
    ChopProcessor& proc; bool dragging = false;
};

class WaveView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit WaveView (ChopProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void filesDropped (const juce::StringArray&, int, int) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragHover = false; repaint(); }
    void browse();
    double viewStart = 0, viewLen = 0;          // samples
    std::function<void()> onSelect, onViewChanged;
private:
    double xToPos (float x) const;
    float posToX (double p) const;
    int flagAt (juce::Point<float>) const;
    int sliceAt (double pos) const;
    void clampView();
    ChopProcessor& proc;
    std::unique_ptr<juce::FileChooser> chooser;
    int dragMarker = -1, auditionNote = -1;
    std::vector<int64_t> dragCopy;
    bool dragHover = false;
};

class OverviewStrip : public juce::Component
{
public:
    OverviewStrip (ChopProcessor& p, WaveView& w) : proc (p), wave (w) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const juce::MouseEvent&) override;
private:
    ChopProcessor& proc; WaveView& wave;
};

class PadGrid : public juce::Component
{
public:
    explicit PadGrid (ChopProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    int bank = 0;
    std::function<void()> onSelect;
private:
    int padAt (juce::Point<float>) const;
    int noteFor (int pad) const;
    ChopProcessor& proc;
    int downNote = -1;
};

class ChopEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ChopEditor (ChopProcessor&);
    ~ChopEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::Slider& knob (const char* paramId, juce::Colour, const juce::String& label);
    juce::Slider& sliceKnob (int sp, juce::Colour, const juce::String& label);
    Chip& chip (const juce::String& text, juce::Colour, const char* boolParam = nullptr);
    juce::Slider& barSlider (const char* paramId);
    void refreshSlice();
    void placeKnob (juce::Slider&, juce::Rectangle<int>);
    juce::String keyShiftText() const;

    ChopProcessor& proc;
    ChopLookAndFeel lnf;
    WaveView wave;
    OverviewStrip overview;
    PadGrid pads;

    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> labels;
    std::vector<std::unique_ptr<Chip>> chips;
    std::vector<std::unique_ptr<ChoiceChips>> groups;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> bAtts;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> cAtts;
    std::map<juce::Slider*, juce::Label*> knobLabels;

    std::array<juce::Slider*, ae8chop::NUM_SP> sliceKnobs {};
    ChoiceChips *chopModes = nullptr, *layoutChips = nullptr, *voiceChips = nullptr, *rateChips = nullptr, *ftypeChips = nullptr;
    juce::Slider *sensBar = nullptr, *slicesBar = nullptr;
    juce::ComboBox divisionBox, keyBox, targetBox, presetBox;
    juce::Label bpmLabel;
    Chip *findBtn = nullptr, *randomBtn = nullptr, *bpmSync = nullptr, *keySync = nullptr, *warpBtn = nullptr,
         *bankA = nullptr, *bankB = nullptr, *revBtn = nullptr, *oneBtn = nullptr, *loopBtn = nullptr,
         *copyBtn = nullptr, *pasteBtn = nullptr, *repeatBtn = nullptr, *tripBtn = nullptr;
    std::array<Chip*, 4> chokeBtns {};
    std::unique_ptr<MidiDragChip> midiBtn;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };

    juce::Rectangle<int> headerArea, sampleArea, padArea, sliceArea, playArea, colorArea, toolbar;
    std::array<float, ae8chop::NUM_SP> clipboard {};
    bool hasClip = false;
    int shownProgram = -1, shownSelected = -1, shownChange = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopEditor)
};
