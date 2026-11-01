#include "PluginEditor.h"
#include "BrandPaths.h"

using namespace ae8;

// AE-8 brand palette (Space theme)
namespace
{
const juce::Colour kBg      { 0xff050608 };
const juce::Colour kPanel   { 0xff111319 };
const juce::Colour kRaised  { 0xff1a1d25 };
const juce::Colour kLine    { 0xff2a2e38 };
const juce::Colour kInk     { 0xfff2f2ee };
const juce::Colour kMuted   { 0xff9b9fa8 };
const juce::Colour kFlux[7] = { juce::Colour (0xff252466), juce::Colour (0xff2b46c8), juce::Colour (0xff4fd0e0), juce::Colour (0xff6fbd46),
                                juce::Colour (0xfff7ec1e), juce::Colour (0xfff5901e), juce::Colour (0xffed1c22) };
constexpr int kHeader = 64, kRowH = 118, kGap = 8, kKeysH = 62, kW = 1240;

juce::Font sansBold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
juce::Font mono (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain));
}

// dipole field line r = L cos^2(lat), in units of the planet radius
juce::Path fieldBand (float Lc, float w, float latDeg, float side, juce::Point<float> c, float R)
{
    juce::Path p;
    const float lm = juce::degreesToRadians (latDeg);
    auto pt = [&] (float L, float la) { const float r = L * std::cos (la) * std::cos (la); return juce::Point<float> (c.x + side * r * std::cos (la) * R, c.y - r * std::sin (la) * R); };
    for (int i = 0; i <= 40; ++i) { const float la = -lm + 2 * lm * (float) i / 40.0f; auto q = pt (Lc + w, la); if (i == 0) p.startNewSubPath (q); else p.lineTo (q); }
    for (int i = 40; i >= 0; --i) { const float la = -lm + 2 * lm * (float) i / 40.0f; p.lineTo (pt (Lc - w, la)); }
    p.closeSubPath();
    return p;
}
} // namespace

//==============================================================================
AE8LookAndFeel::AE8LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, kInk);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId, kBg);
    setColour (juce::ComboBox::textColourId, kInk);
    setColour (juce::ComboBox::outlineColourId, kLine);
    setColour (juce::ComboBox::arrowColourId, kMuted);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::textColourId, kInk);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kFlux[1]);
    setColour (juce::PopupMenu::highlightedTextColourId, kInk);
    setColour (juce::Label::textColourId, kInk);
    setColour (juce::TextButton::buttonColourId, kRaised);
    setColour (juce::TextButton::textColourOffId, kInk);
    setColour (juce::TextEditor::backgroundColourId, kBg);
    setColour (juce::TextEditor::textColourId, kInk);
    setColour (juce::TextEditor::highlightColourId, kFlux[1]);
    setColour (juce::CaretComponent::caretColourId, kInk);
}

juce::Font AE8LookAndFeel::getComboBoxFont (juce::ComboBox&) { return mono (12.0f, true); }
juce::Font AE8LookAndFeel::getPopupMenuFont() { return mono (13.0f); }
juce::Font AE8LookAndFeel::getTextButtonFont (juce::TextButton&, int) { return mono (14.0f, true); }

juce::Label* AE8LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (mono (11.5f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::textColourId, kInk);
    return l;
}

void AE8LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s)
{
    const auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
    const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float ang = start + pos * (end - start);
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);

    juce::Path track; track.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, end, true);
    g.setColour (kLine); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
    const float from = bipolar ? start + (float) ((0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) * (end - start) : start;
    juce::Path arc; arc.addCentredArc (c.x, c.y, r - 2, r - 2, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
    g.setColour (accent); g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float kr = r - 8;
    g.setColour (kBg); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    g.setColour (kLine); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);

    juce::Path ptr; ptr.addRoundedRectangle (-1.5f, -kr + 3, 3.0f, kr * 0.6f, 1.5f);
    g.setColour (kInk); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
}

void AE8LookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().withSizeKeepingCentre (40, 22);
    const bool on = b.getToggleState();
    const auto col = b.findColour (juce::ToggleButton::tickColourId);
    g.setColour (on ? col : kRaised);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (kLine); if (! on) g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (on ? (col.getPerceivedBrightness() < 0.45f ? kInk : kBg) : kMuted);
    g.setFont (mono (11.0f, true));
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

void AE8LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? kLine : (hi ? kRaised.brighter (0.15f) : kRaised)); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (kLine); g.drawRoundedRectangle (r, 4.0f, 1.0f);
}

//==============================================================================
AE8Editor::AE8Editor (AE8Processor& p)
    : AudioProcessorEditor (&p), proc (p),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lnf);
    wordmark = juce::Drawable::parseSVGPath (kWordmarkPath);

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
    keyboard.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::transparentBlack);
    keyboard.setWantsKeyboardFocus (false);
    addAndMakeVisible (keyboard);

    refreshPresetBox();
    setSize (kW, kHeader + kGap + 4 * (kRowH + kGap) + kKeysH + kGap);
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

void AE8Editor::drawBeltMark (juce::Graphics& g, juce::Rectangle<float> a) const
{
    const auto c = a.getCentre();
    const float R = a.getHeight() * 0.085f;
    const float outer[][3] = { { 1.35f, 46, 1 }, { 1.1f, 40, 2 }, { 0.85f, 34, 3 }, { 0.62f, 28, 4 }, { 0.4f, 22, 5 }, { 0.2f, 16, 6 } };
    const float inner[][3] = { { 0.32f, 36, 2 }, { 0.2f, 28, 4 }, { 0.09f, 20, 6 } };
    for (float side : { 1.0f, -1.0f })
    {
        for (auto& b : outer) { g.setColour (kFlux[(int) b[2]]); g.fillPath (fieldBand (4.6f, b[0], b[1], side, c, R)); }
        for (auto& b : inner) { g.setColour (kFlux[(int) b[2]]); g.fillPath (fieldBand (1.75f, b[0], b[1], side, c, R)); }
    }
    g.setColour (kInk); g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);
}

void AE8Editor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    // ---- header plate with faint field lines behind it
    auto hb = headerArea.toFloat();
    g.setColour (juce::Colour (0xff0b0c10)); g.fillRoundedRectangle (hb, 6.0f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (headerArea);
        const juce::Point<float> c (hb.getCentreX(), hb.getCentreY() + 4);
        const float R = 9.0f;
        g.setColour (kInk.withAlpha (0.07f));
        for (float L : { 3.0f, 5.0f, 8.0f, 13.0f, 21.0f, 34.0f, 55.0f, 90.0f })
            for (float side : { 1.0f, -1.0f })
            {
                juce::Path p;
                const float lm = std::acos (std::sqrt (1.0f / L));
                for (int i = 0; i <= 120; ++i)
                {
                    const float la = -lm + 2 * lm * (float) i / 120.0f, r = L * std::cos (la) * std::cos (la);
                    const juce::Point<float> q (c.x + side * r * std::cos (la) * R, c.y - r * std::sin (la) * R);
                    if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
                }
                g.strokePath (p, juce::PathStrokeType (1.0f));
            }
    }
    g.setColour (kLine); g.drawRoundedRectangle (hb.reduced (0.5f), 6.0f, 1.0f);

    // flux colorbar along the bottom edge of the header
    {
        auto bar = hb.reduced (12.0f, 0).removeFromBottom (3.0f).translated (0, -1);
        const float w = bar.getWidth() / 7.0f;
        for (int i = 0; i < 7; ++i) { g.setColour (kFlux[i]); g.fillRect (bar.getX() + w * (float) i, bar.getY(), w + 0.5f, bar.getHeight()); }
    }

    drawBeltMark (g, markArea.toFloat());
    g.setColour (kInk);
    g.fillPath (wordmark, wordmark.getTransformToScaleToFit (wordArea.toFloat(), true, juce::Justification::centredLeft));
    g.setFont (sansBold (28.0f));
    g.drawText ("POLY", wordArea.getRight() + 12, headerArea.getY(), 90, headerArea.getHeight(), juce::Justification::centredLeft);
    g.setFont (mono (11.0f));
    g.setColour (kMuted);
    g.drawText ("P-01 / POLYSYNTH", wordArea.getRight() + 98, headerArea.getY(), 200, headerArea.getHeight(), juce::Justification::centredLeft);
    g.setFont (mono (10.5f, true));
    g.drawText ("PROGRAM", presetBox.getBounds().translated (-78, 0).withWidth (70), juce::Justification::centredRight);

    // ---- sections
    for (auto& s : sections)
    {
        auto b = s.bounds.toFloat();
        g.setColour (kPanel); g.fillRoundedRectangle (b, 6.0f);
        auto strip = b.removeFromTop (20.0f);
        juce::Path p; p.addRoundedRectangle (strip.getX(), strip.getY(), strip.getWidth(), strip.getHeight(), 6.0f, 6.0f, true, true, false, false);
        g.setColour (s.colour); g.fillPath (p);
        g.setColour (s.lightText ? kInk : kBg);
        g.setFont (sansBold (11.5f));
        g.drawText (s.title, strip.reduced (10, 0), juce::Justification::centredLeft);
        g.setFont (mono (10.0f));
        g.setColour ((s.lightText ? kInk : kBg).withAlpha (0.75f));
        g.drawText (s.tag, strip.reduced (10, 0), juce::Justification::centredRight);
    }
}

void AE8Editor::resized()
{
    auto area = getLocalBounds().reduced (kGap, 0);
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
    keyboard.setBounds (area.removeFromTop (kKeysH));
    keyboard.setKeyWidth ((float) keyboard.getWidth() / 50.0f);
}
