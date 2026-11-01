#include "PluginEditor.h"

using namespace ae8;

namespace
{
const juce::Colour kBg      { 0xff17181b };
const juce::Colour kPanel   { 0xff24262b };
const juce::Colour kPanelHi { 0xff2d3036 };
const juce::Colour kText    { 0xffe6e2d8 };
const juce::Colour kDim     { 0xff9a968c };
constexpr int kHeader = 58, kRowH = 118, kGap = 8, kKeysH = 62, kW = 1240;
}

//==============================================================================
AE8LookAndFeel::AE8LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, kBg);
    setColour (juce::ComboBox::textColourId, kText);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a3d44));
    setColour (juce::ComboBox::arrowColourId, kDim);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::textColourId, kText);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffd9452b));
    setColour (juce::Label::textColourId, kText);
    setColour (juce::TextButton::buttonColourId, kPanelHi);
    setColour (juce::TextButton::textColourOffId, kText);
}

void AE8LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float ang = start + pos * (end - start);
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

    // track
    juce::Path track; track.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, end, true);
    g.setColour (juce::Colour (0xff3a3d44)); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc (bipolar params draw from the centre)
    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
    const float from = bipolar ? start + (float) ((0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) * (end - start) : start;
    juce::Path arc; arc.addCentredArc (c.x, c.y, r - 2, r - 2, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
    g.setColour (accent); g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // knob cap
    const float kr = r - 7;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4d55), c.x, c.y - kr, juce::Colour (0xff1c1d21), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (juce::Colour (0xff0e0f11)); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);

    // pointer
    juce::Path ptr; ptr.addRoundedRectangle (-1.5f, -kr + 2, 3.0f, kr * 0.55f, 1.5f);
    g.setColour (kText); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void AE8LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().withSizeKeepingCentre (34, 20);
    const bool on = b.getToggleState();
    g.setColour (on ? b.findColour (juce::ToggleButton::tickColourId) : juce::Colour (0xff3a3d44));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (on ? juce::Colours::black : kDim);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (on ? "ON" : "OFF", r, juce::Justification::centred);
}

void AE8LookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    juce::Path tri; const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    tri.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId)); g.fillPath (tri);
}

//==============================================================================
AE8Editor::AE8Editor (AE8Processor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);

    sections =
    {
        { "OSCILLATOR 1", juce::Colour (0xffd9452b), { OSC1_WAVE, OSC1_OCT, OSC1_LEVEL }, {} },
        { "OSCILLATOR 2", juce::Colour (0xffe8862a), { OSC2_WAVE, OSC2_OCT, OSC2_SEMI, OSC2_DETUNE, OSC2_LEVEL, OSC2_SYNC }, {} },
        { "MODULATION",   juce::Colour (0xffe3c033), { XMOD, PW, PWM_LFO, ENV_PITCH }, {} },
        { "MIXER",        juce::Colour (0xffb5c43a), { SUB_LEVEL, NOISE_LEVEL, HPF }, {} },
        { "FILTER",       juce::Colour (0xff5fae4f), { FLT_TYPE, CUTOFF, RESO, FLT_ENV, FLT_KEY, FLT_LFO, FLT_VEL, DRIVE }, {} },
        { "FILTER ENVELOPE", juce::Colour (0xff3aa3a0), { F_A, F_D, F_S, F_R }, {} },
        { "AMP ENVELOPE", juce::Colour (0xff3d7fd1), { A_A, A_D, A_S, A_R, AMP_VEL }, {} },
        { "LFO",          juce::Colour (0xff8a5cc7), { LFO_WAVE, LFO_RATE, LFO_DELAY, LFO_PITCH, MW_VIB }, {} },
        { "VOICE",        juce::Colour (0xffc95091), { MODE, UNI_DETUNE, GLIDE, BEND, DRIFT, SPREAD }, {} },
        { "EFFECTS",      juce::Colour (0xffd0cbc0), { CHORUS, DLY_TIME, DLY_FB, DLY_MIX, REV_SIZE, REV_MIX, VOLUME }, {} },
    };

    static const std::map<int, const char*> shortNames =
    {
        { OSC1_WAVE, "WAVE" }, { OSC1_OCT, "OCTAVE" }, { OSC1_LEVEL, "LEVEL" },
        { OSC2_WAVE, "WAVE" }, { OSC2_OCT, "OCTAVE" }, { OSC2_SEMI, "SEMI" }, { OSC2_DETUNE, "DETUNE" }, { OSC2_LEVEL, "LEVEL" }, { OSC2_SYNC, "SYNC" },
        { XMOD, "CROSS MOD" }, { PW, "PULSE W" }, { PWM_LFO, "PWM LFO" }, { ENV_PITCH, "ENV>OSC2" },
        { SUB_LEVEL, "SUB OSC" }, { NOISE_LEVEL, "NOISE" }, { HPF, "HPF" },
        { FLT_TYPE, "TYPE" }, { CUTOFF, "CUTOFF" }, { RESO, "RESO" }, { FLT_ENV, "ENV AMT" }, { FLT_KEY, "KEY TRK" },
        { FLT_LFO, "LFO AMT" }, { FLT_VEL, "VELOCITY" }, { DRIVE, "DRIVE" },
        { F_A, "ATTACK" }, { F_D, "DECAY" }, { F_S, "SUSTAIN" }, { F_R, "RELEASE" },
        { A_A, "ATTACK" }, { A_D, "DECAY" }, { A_S, "SUSTAIN" }, { A_R, "RELEASE" }, { AMP_VEL, "VELOCITY" },
        { LFO_WAVE, "WAVE" }, { LFO_RATE, "RATE" }, { LFO_DELAY, "DELAY" }, { LFO_PITCH, "VIBRATO" }, { MW_VIB, "MW VIB" },
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
            c->label->setFont (juce::FontOptions (10.5f, juce::Font::bold));
            c->label->setColour (juce::Label::textColourId, kDim);
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
                c->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
                c->slider->setColour (juce::Slider::rotarySliderFillColourId, sec.colour);
                c->slider->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
                c->slider->setColour (juce::Slider::textBoxTextColourId, kText);
                c->slider->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
                c->slider->setTooltip (d.name);
                addAndMakeVisible (*c->slider);
                c->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, *c->slider);
                c->slider->setDoubleClickReturnValue (true, d.def);
            }
            controls.push_back (std::move (c));
        }

    // preset bar
    const int np = proc.getNumPrograms();
    for (int i = 0; i < np; ++i) presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i), i + 1);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0d0e10));
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colour (0xffff7a45));
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && idx != proc.getCurrentProgram()) { proc.setCurrentProgram (idx); shownProgram = idx; }
    };
    prevBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); refreshPresetBox(); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); refreshPresetBox(); };
    addAndMakeVisible (presetBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);

    keyboard.setAvailableRange (24, 108);
    keyboard.setKeyWidth (16.0f);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, juce::Colour (0xffd9452b));
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    refreshPresetBox();
    setSize (kW, kHeader + 4 * (kRowH + kGap) + kKeysH + kGap);
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
    g.fillAll (kBg);

    // header
    auto header = getLocalBounds().removeFromTop (kHeader).reduced (kGap, 6);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2c31), 0, (float) header.getY(), juce::Colour (0xff1b1c20), 0, (float) header.getBottom(), false));
    g.fillRoundedRectangle (header.toFloat(), 6.0f);
    // rainbow stripe (a nod to the classic Roland panel)
    {
        const juce::Colour stripe[] = { juce::Colour (0xffd9452b), juce::Colour (0xffe8862a), juce::Colour (0xffe3c033), juce::Colour (0xff5fae4f), juce::Colour (0xff3d7fd1) };
        auto s = header.withTrimmedLeft (200).withWidth (110).withSizeKeepingCentre (110, 22);
        const int sw = s.getWidth() / 5;
        for (int i = 0; i < 5; ++i) { g.setColour (stripe[i]); g.fillRect (s.getX() + i * sw, s.getY(), sw - 3, s.getHeight()); }
    }
    g.setColour (kText);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("AE-8", header.withTrimmedLeft (16).withWidth (90), juce::Justification::centredLeft);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.setColour (kDim);
    g.drawText ("ANALOG POLY", header.withTrimmedLeft (92).withWidth (110), juce::Justification::centredLeft);
    g.drawText ("PROGRAM", presetBox.getBounds().translated (-70, 0).withWidth (64), juce::Justification::centredRight);

    // sections
    for (auto& s : sections)
    {
        auto b = s.bounds.toFloat();
        g.setColour (kPanel); g.fillRoundedRectangle (b, 6.0f);
        auto strip = b.removeFromTop (20.0f);
        juce::Path p; p.addRoundedRectangle (strip.getX(), strip.getY(), strip.getWidth(), strip.getHeight(), 6.0f, 6.0f, true, true, false, false);
        g.setColour (s.colour.withAlpha (0.9f)); g.fillPath (p);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (s.title, strip.reduced (10, 0), juce::Justification::centredLeft);
    }
}

void AE8Editor::resized()
{
    auto area = getLocalBounds();
    auto header = area.removeFromTop (kHeader).reduced (kGap, 6);
    nextBtn.setBounds (header.removeFromRight (34).reduced (0, 10).translated (-8, 0));
    prevBtn.setBounds (header.removeFromRight (34).reduced (0, 10).translated (-12, 0));
    presetBox.setBounds (header.removeFromRight (340).reduced (0, 10).translated (-18, 0));

    const std::vector<std::vector<int>> rows = { { 0, 1, 2, 3 }, { 4, 5 }, { 6, 7 }, { 8, 9 } };
    area.removeFromLeft (kGap); area.removeFromRight (kGap);

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
                        else if (c->combo)  c->combo->setBounds (cc.withSizeKeepingCentre (juce::jmin (cc.getWidth() - 6, 92), 24).translated (0, -6));
                        else if (c->toggle) c->toggle->setBounds (cc.withSizeKeepingCentre (cc.getWidth(), 28).translated (0, -6));
                    }
            }
        }
    }
    keyboard.setBounds (area.removeFromTop (kKeysH));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 50.0f);   // 50 white keys in C1..C8
}
