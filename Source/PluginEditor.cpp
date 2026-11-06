#include "PluginEditor.h"

using namespace ae8;

using namespace ae8brand;

namespace
{
const juce::Colour kFlux[7] = { flux (0), flux (1), flux (2), flux (3), flux (4), flux (5), flux (6) };
constexpr int kHeader = 64, kRowH = 118, kGap = 8, kKeysH = 62, kW = 1240;
constexpr int kH = kGap + kHeader + kGap + 4 * (kRowH + kGap) + kKeysH + kGap;
const char* kNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
} // namespace

//==============================================================================
void AE8Keyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour, juce::Colour)
{
    auto k = a.reduced (0.6f, 0).withTrimmedBottom (1.0f);
    g.setGradientFill (juce::ColourGradient (down ? flux (5).brighter (0.2f) : juce::Colour (0xfff4f2ec), k.getX(), k.getY(),
                                             down ? flux (5) : juce::Colour (over ? 0xffdcdad2 : 0xffd2cfc6), k.getX(), k.getBottom(), false));
    g.fillRoundedRectangle (k, 2.5f);
    g.setColour (juce::Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (k, 2.5f, 0.8f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.2f : 0.8f)); g.fillRect (k.getX() + 1.5f, k.getBottom() - 3.0f, k.getWidth() - 3.0f, 1.0f);
    if (note % 12 == 0)
    {
        g.setColour (juce::Colour (0xff6a6760)); g.setFont (mono (8.5f, true));
        g.drawText (juce::String (kNames[0]) + juce::String (note / 12 - 2), k.withTrimmedTop (k.getHeight() - 15).withTrimmedLeft (2), juce::Justification::centredLeft);
    }
}

void AE8Keyboard::drawBlackNote (int, juce::Graphics& g, juce::Rectangle<float> a, bool down, bool over, juce::Colour)
{
    auto k = a.reduced (1.0f, 0).withTrimmedBottom (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (k.translated (1.0f, 1.5f), 2.0f);
    g.setGradientFill (juce::ColourGradient (down ? flux (5) : juce::Colour (over ? 0xff2a2c33 : 0xff1c1d22), k.getX(), k.getY(),
                                             down ? flux (6) : juce::Colour (0xff050507), k.getX(), k.getBottom(), false));
    g.fillRoundedRectangle (k, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.25f : 0.10f));
    g.fillRoundedRectangle (k.reduced (2.5f, 0).withTrimmedTop (2.0f).withHeight (k.getHeight() * 0.62f), 1.5f);
    g.setColour (juce::Colours::black); g.drawRoundedRectangle (k, 2.0f, 0.8f);
}

//==============================================================================
AE8Editor::AE8Editor (AE8Processor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    sections =
    {
        { "OSCILLATOR 1", "VCO",  kFlux[5], false, { OSC1_WAVE, OSC1_OCT, OSC1_LEVEL }, {} },
        { "OSCILLATOR 2", "VCO",  kFlux[5], false, { OSC2_WAVE, OSC2_OCT, OSC2_SEMI, OSC2_DETUNE, OSC2_LEVEL, OSC2_SYNC }, {} },
        { "MODULATION",   "MOD",  kFlux[4], false, { XMOD, PW, PWM_LFO, ENV_PITCH }, {} },
        { "MIXER",        "MIX",  kFlux[2], false, { SUB_LEVEL, NOISE_LEVEL, HPF }, {} },
        { "FM",           "OSC2 > OSC1", kFlux[6], false, { FM_AMT, FM_RATIO, FM_ENV, FM_VEL }, {} },
        { "FILTER",       "VCF",  kFlux[3], false, { FLT_TYPE, CUTOFF, RESO, FLT_ENV, FLT_KEY, FLT_LFO, FLT_VEL, DRIVE }, {} },
        { "FILTER / FM ENVELOPE", "ENV 1", kFlux[1], true, { F_A, F_D, F_S, F_R }, {} },
        { "AMP ENVELOPE", "ENV 2", kFlux[1], true, { A_A, A_D, A_S, A_R, AMP_VEL }, {} },
        { "LFO",          "LFO",  kFlux[4], false, { LFO_WAVE, LFO_RATE, LFO_DELAY, LFO_PITCH, MW_VIB }, {} },
        { "ARPEGGIATOR",  "SYNC", kFlux[2], false, { ARP_ON, ARP_MODE, ARP_RATE, ARP_OCT, ARP_GATE, ARP_LATCH }, {} },
        { "VOICE",        "12 VOICES", kFlux[0], true, { MODE, UNI_DETUNE, GLIDE, BEND, DRIFT, SPREAD }, {} },
        { "EFFECTS",      "FX",   kFlux[1], true, { CHORUS, DLY_TIME, DLY_FB, DLY_MIX, REV_SIZE, REV_MIX, VOLUME }, {} },
    };

    static const std::map<int, const char*> shortNames =
    {
        { OSC1_WAVE, "WAVE" }, { OSC1_OCT, "OCTAVE" }, { OSC1_LEVEL, "LEVEL" },
        { OSC2_WAVE, "WAVE" }, { OSC2_OCT, "OCTAVE" }, { OSC2_SEMI, "SEMI" }, { OSC2_DETUNE, "DETUNE" }, { OSC2_LEVEL, "LEVEL" }, { OSC2_SYNC, "SYNC" },
        { XMOD, "CROSS MOD" }, { PW, "PULSE W" }, { PWM_LFO, "PWM LFO" }, { ENV_PITCH, "ENV>OSC2" },
        { SUB_LEVEL, "SUB OSC" }, { NOISE_LEVEL, "NOISE" }, { HPF, "HPF" },
        { FM_AMT, "AMOUNT" }, { FM_RATIO, "RATIO" }, { FM_ENV, "ENV" }, { FM_VEL, "VELOCITY" },
        { FLT_TYPE, "TYPE" }, { CUTOFF, "CUTOFF" }, { RESO, "RESO" }, { FLT_ENV, "ENV AMT" }, { FLT_KEY, "KEY TRK" },
        { FLT_LFO, "LFO AMT" }, { FLT_VEL, "VELOCITY" }, { DRIVE, "DRIVE" },
        { F_A, "ATTACK" }, { F_D, "DECAY" }, { F_S, "SUSTAIN" }, { F_R, "RELEASE" },
        { A_A, "ATTACK" }, { A_D, "DECAY" }, { A_S, "SUSTAIN" }, { A_R, "RELEASE" }, { AMP_VEL, "VELOCITY" },
        { LFO_WAVE, "WAVE" }, { LFO_RATE, "RATE" }, { LFO_DELAY, "DELAY" }, { LFO_PITCH, "VIBRATO" }, { MW_VIB, "MW VIB" },
        { ARP_ON, "ARP" }, { ARP_MODE, "MODE" }, { ARP_RATE, "RATE" }, { ARP_OCT, "OCTAVES" }, { ARP_GATE, "GATE" }, { ARP_LATCH, "LATCH" },
        { MODE, "MODE" }, { UNI_DETUNE, "UNI DETUNE" }, { GLIDE, "GLIDE" }, { BEND, "BEND" }, { DRIFT, "DRIFT" }, { SPREAD, "SPREAD" },
        { CHORUS, "CHORUS" }, { DLY_TIME, "DLY TIME" }, { DLY_FB, "DLY FDBK" }, { DLY_MIX, "DLY MIX" },
        { REV_SIZE, "REV SIZE" }, { REV_MIX, "REV MIX" }, { VOLUME, "VOLUME" },
    };

    for (auto& sec : sections)
        for (int pi : sec.params)
        {
            const auto& d = paramDefs()[(size_t) pi];
            auto c = std::make_unique<Control>();
            c->param = pi;
            c->label = std::make_unique<juce::Label> (juce::String(), shortNames.at (pi));
            c->label->setJustificationType (juce::Justification::centred);
            c->label->setFont (mono (10.5f, true));
            c->label->setColour (juce::Label::textColourId, kMuted);
            c->label->setTooltip (d.name);
            addAndMakeVisible (*c->label);

            if (d.kind == Choice)
            {
                c->combo = std::make_unique<juce::ComboBox>();
                juce::StringArray items; for (auto* s : d.choices) items.add (s);
                c->combo->addItemList (items, 1);
                addAndMakeVisible (*c->combo);
                c->cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, d.id, *c->combo);
            }
            else if (d.kind == Bool)
            {
                c->toggle = std::make_unique<juce::ToggleButton>();
                c->toggle->setColour (juce::ToggleButton::tickColourId, sec.colour);
                addAndMakeVisible (*c->toggle);
                c->bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, d.id, *c->toggle);
            }
            else
            {
                c->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
                c->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
                c->slider->setColour (juce::Slider::rotarySliderFillColourId, sec.colour == kFlux[0] ? kFlux[2] : sec.colour);
                c->slider->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
                c->slider->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
                c->slider->setTooltip (d.name);
                addAndMakeVisible (*c->slider);
                c->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, *c->slider);
                c->slider->setDoubleClickReturnValue (true, d.def);
            }
            controls.push_back (std::move (c));
        }

    // program display
    const int np = proc.getNumPrograms();
    for (int i = 0; i < np; ++i) presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i).toUpperCase(), i + 1);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    presetBox.setColour (juce::ComboBox::textColourId, kFlux[5]);
    presetBox.setColour (juce::ComboBox::arrowColourId, kFlux[5]);
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram()) { proc.setCurrentProgram (idx); shownProgram = idx; }
    };
    prevBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); refreshPresetBox(); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); refreshPresetBox(); };
    addAndMakeVisible (presetBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);

    keyboard.setAvailableRange (24, 108);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe9e9e4));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, kBg);
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff9b9fa8));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kFlux[5]);
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, kFlux[2].withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.35f));
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, kGlass);
    keyboard.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, kMuted);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    refreshPresetBox();
    scaler.init (*this, kW, kH);
    startTimerHz (10);
}

AE8Editor::~AE8Editor() { setLookAndFeel (nullptr); }

void AE8Editor::refreshPresetBox()
{
    shownProgram = proc.getCurrentProgram();
    presetBox.setSelectedId (shownProgram + 1, juce::dontSendNotification);
}

void AE8Editor::timerCallback()
{
    if (proc.getCurrentProgram() != shownProgram) refreshPresetBox();
}

void AE8Editor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.scale (*this)));
    drawWindow (g, scaler.design());
    drawHeader (g, headerArea, "POLY", "P-01 / POLYSYNTH");
    g.setFont (tracked (mono (9.5f, true), 0.12f)); g.setColour (kMuted);
    g.drawText ("PROGRAM", presetBox.getBounds().translated (-78, 0).withWidth (70), juce::Justification::centredRight);
    for (auto& s : sections) drawSection (g, s.bounds, s.title, s.tag, s.colour, s.lightText);
    // keyboard bed
    auto kb = keyboard.getBounds().toFloat().expanded (4.0f, 4.0f);
    drawGlass (g, kb, 6.0f);
}

void AE8Editor::resized()
{
    auto area = scaler.design().reduced (kGap, 0);
    area.removeFromTop (kGap);
    headerArea = area.removeFromTop (kHeader);
    area.removeFromTop (kGap);

    auto h = headerArea.reduced (14, 0);
    markArea = h.removeFromLeft (70).withSizeKeepingCentre (70, 70);
    h.removeFromLeft (8);
    wordArea = h.removeFromLeft (86).withSizeKeepingCentre (86, 30);

    auto right = headerArea.reduced (14, 15);
    nextBtn.setBounds (right.removeFromRight (34));
    right.removeFromRight (6);
    prevBtn.setBounds (right.removeFromRight (34));
    right.removeFromRight (8);
    presetBox.setBounds (right.removeFromRight (360));

    const std::vector<std::vector<int>> rows = { { 0, 1, 2, 3 }, { 4, 5, 6 }, { 7, 8, 9 }, { 10, 11 } };

    for (const auto& row : rows)
    {
        auto rowArea = area.removeFromTop (kRowH);
        area.removeFromTop (kGap);
        int cells = 0; for (int si : row) cells += (int) sections[(size_t) si].params.size();
        const int avail = rowArea.getWidth() - kGap * ((int) row.size() - 1);
        int x = rowArea.getX();
        for (size_t k = 0; k < row.size(); ++k)
        {
            auto& sec = sections[(size_t) row[k]];
            const int n = (int) sec.params.size();
            const int w = (k == row.size() - 1) ? rowArea.getRight() - x : avail * n / cells;
            sec.bounds = { x, rowArea.getY(), w, kRowH };
            x += w + kGap;

            auto inner = sec.bounds.withTrimmedTop (22).reduced (4, 2);
            const int cw = inner.getWidth() / n;
            for (int i = 0; i < n; ++i)
            {
                auto cell = inner.removeFromLeft (i == n - 1 ? inner.getWidth() : cw);
                for (auto& c : controls)
                    if (c->param == sec.params[(size_t) i])
                    {
                        auto cc = cell;
                        c->label->setBounds (cc.removeFromTop (16));
                        if (c->slider)      c->slider->setBounds (cc.withSizeKeepingCentre (juce::jmin (cc.getWidth(), 78), cc.getHeight()));
                        else if (c->combo)  c->combo->setBounds (cc.withSizeKeepingCentre (juce::jmin (cc.getWidth() - 6, 96), 24).translated (0, -6));
                        else if (c->toggle) c->toggle->setBounds (cc.withSizeKeepingCentre (cc.getWidth(), 28).translated (0, -6));
                    }
            }
        }
    }
    keyboard.setBounds (area.removeFromTop (kKeysH).reduced (4, 4));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 50.0f);
    scaler.apply (*this);
}
