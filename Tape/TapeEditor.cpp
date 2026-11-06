#include "TapeEditor.h"

using namespace ae8tape;
using namespace ae8brand;

namespace { constexpr int kW = 1180, kHeader = 64, kGap = 8, kTopH = 300, kKnobH = 124, kSideW = 250;
            constexpr int kH = kGap + kHeader + kGap + kTopH + kGap + kKnobH + kGap; }

//==============================================================================
void TapeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearVertical) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s); return; }
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y + 8, bot = (float) (y + h) - 8;
    // scale ticks
    for (int i = 0; i <= 10; ++i)
    {
        const float ty = top + (bot - top) * (float) i / 10.0f; const float tw = i % 5 == 0 ? 9.0f : 5.0f;
        g.setColour (kInk.withAlpha (i % 5 == 0 ? 0.30f : 0.14f));
        g.fillRect (cx - 13 - tw, ty, tw, 1.0f); g.fillRect (cx + 13, ty, tw, 1.0f);
    }
    // milled slot
    drawGlass (g, { cx - 3.5f, top - 2, 7.0f, bot - top + 4 }, 3.5f);
    // lit fill from zero point (bipolar) or bottom
    const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
    const float zeroY = bipolar ? (float) s.getPositionOfValue (0.0) : bot;
    const auto col = s.findColour (juce::Slider::trackColourId);
    auto fill = juce::Rectangle<float> (cx - 1.5f, juce::jmin (zeroY, pos), 3.0f, std::fabs (zeroY - pos));
    g.setColour (col.withAlpha (0.22f)); g.fillRoundedRectangle (fill.expanded (2.5f, 1.0f), 3.0f);
    g.setColour (col); g.fillRoundedRectangle (fill, 1.5f);
    // machined cap
    auto cap = juce::Rectangle<float> (cx - 18, pos - 11, 36, 22);
    for (int i = 1; i <= 3; ++i) { g.setColour (juce::Colours::black.withAlpha (0.18f)); g.fillRoundedRectangle (cap.translated (0, (float) i).expanded (0.5f * i), 4); }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff40444e), cap.getX(), cap.getY(), juce::Colour (0xff121318), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1c22), cap.getX(), cap.getCentreY(), juce::Colour (0xff2a2d35), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap.withTrimmedTop (cap.getHeight() * 0.5f).reduced (2.0f, 1.0f), 2);
    g.setColour (juce::Colours::black); g.drawRoundedRectangle (cap, 4, 1);
    g.setColour (juce::Colours::white.withAlpha (0.18f)); g.fillRect (cap.getX() + 4, cap.getY() + 1, cap.getWidth() - 8, 1.0f);
    g.setColour (kInk); g.fillRect (cap.getX() + 5, pos - 0.75f, cap.getWidth() - 10, 1.5f);
}

void TapeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hi, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    const auto col = b.findColour (juce::ToggleButton::tickColourId);
    drawRaised (g, r, hi, on);
    if (on) { g.setColour (col.withAlpha (0.14f)); g.fillRoundedRectangle (r.reduced (1.0f), 4.0f); }
    drawLed (g, { r.getX() + 14.0f, r.getCentreY() }, 3.5f, col, on);
    g.setColour (on ? kInk : kMuted); g.setFont (tracked (mono (12.0f, true), 0.1f));
    const auto txt = b.getButtonText() == "ON" ? juce::String (on ? "ON" : "OFF") : b.getButtonText();
    g.drawText (txt, r.withTrimmedLeft (18.0f), juce::Justification::centred);
}

//==============================================================================
void ReelsView::drawReel (juce::Graphics& g, juce::Point<float> c, float R, float packR, float angle)
{
    // tape pack, rendered as flux rings
    for (int i = 6; i >= 0; --i)
    {
        const float rr = 0.3f * R + (packR - 0.3f * R) * (float) (i + 1) / 7.0f;
        g.setColour (flux (i).withAlpha (0.85f));
        g.fillEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2);
    }
    g.setColour (kBg.withAlpha (0.35f));
    for (int i = 1; i < 7; ++i) { const float rr = 0.3f * R + (packR - 0.3f * R) * (float) i / 7.0f; g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, 0.6f); }
    // flange (transparent with spokes)
    g.setGradientFill (juce::ColourGradient (kInk.withAlpha (0.16f), c.x - R * 0.5f, c.y - R * 0.7f, kInk.withAlpha (0.03f), c.x + R * 0.6f, c.y + R, true));
    g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);
    g.setColour (kInk.withAlpha (0.5f)); g.drawEllipse (c.x - R, c.y - R, R * 2, R * 2, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - R - 1.5f, c.y - R - 1.5f, R * 2 + 3, R * 2 + 3, 1.0f);
    for (int k = 0; k < 3; ++k)
    {
        juce::Path win;
        const float a0 = angle + (float) k * juce::MathConstants<float>::twoPi / 3.0f;
        win.addCentredArc (c.x, c.y, R * 0.82f, R * 0.82f, 0, a0 + 0.25f, a0 + 1.6f, true);
        win.addCentredArc (c.x, c.y, R * 0.42f, R * 0.42f, 0, a0 + 1.6f, a0 + 0.25f, false);
        win.closeSubPath();
        g.setColour (kInk.withAlpha (0.12f)); g.fillPath (win);
        g.setColour (kInk.withAlpha (0.35f)); g.strokePath (win, juce::PathStrokeType (1.0f));
    }
    // hub
    const float hr = R * 0.24f;
    g.setColour (kRaised); g.fillEllipse (c.x - hr, c.y - hr, hr * 2, hr * 2);
    g.setColour (kLine.brighter (0.4f)); g.drawEllipse (c.x - hr, c.y - hr, hr * 2, hr * 2, 1.0f);
    for (int k = 0; k < 3; ++k)
    {
        const float a = angle + (float) k * juce::MathConstants<float>::twoPi / 3.0f;
        g.setColour (kInk);
        g.fillRect (juce::Rectangle<float> (-2.0f, -hr, 4.0f, hr * 0.45f).transformedBy (juce::AffineTransform::rotation (a).translated (c.x, c.y)).getSmallestIntegerContainer().toFloat());
    }
    g.setColour (kBg); g.fillEllipse (c.x - 4, c.y - 4, 8, 8);
}

void ReelsView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    drawGlass (g, r, 6.0f);

    auto& e = proc.engine;
    const float pack = e.guiPack.load();
    const float R = juce::jmin (r.getHeight() * 0.36f, r.getWidth() * 0.2f);
    const float cy = r.getY() + R + 14;
    const juce::Point<float> cl (r.getX() + r.getWidth() * 0.27f, cy), cr (r.getX() + r.getWidth() * 0.73f, cy);
    const float pL = R * (0.32f + 0.6f * (1.0f - pack)), pR = R * (0.32f + 0.6f * pack);

    // head block and tape path
    const float headY = r.getBottom() - 30;
    const juce::Point<float> gL (r.getCentreX() - 70, headY), gR (r.getCentreX() + 70, headY);
    juce::Path tp;
    tp.startNewSubPath (cl.x - pL, cl.y);
    tp.lineTo (gL.x - 30, headY);
    tp.lineTo (gR.x + 30, headY);
    tp.lineTo (cr.x + pR, cr.y);
    g.setColour (flux (1).withAlpha (0.9f)); g.strokePath (tp, juce::PathStrokeType (2.0f));
    for (auto p : { gL.translated (-30, 0), gR.translated (30, 0) }) { g.setColour (kRaised); g.fillEllipse (p.x - 7, p.y - 7, 14, 14); g.setColour (kLine.brighter (0.4f)); g.drawEllipse (p.x - 7, p.y - 7, 14, 14, 1); }
    int idx = 0;
    for (auto lbl : { "ERASE", "REC", "PLAY" })
    {
        auto hb = juce::Rectangle<float> (r.getCentreX() - 54 + 38.0f * (float) idx, headY - 2, 32, 20);
        g.setColour (idx == 1 ? flux (6) : (idx == 2 ? flux (2) : kRaised)); g.fillRoundedRectangle (hb, 2);
        g.setColour (idx == 0 ? kMuted : kBg); g.setFont (mono (8.5f, true)); g.drawText (lbl, hb.translated (0, 2), juce::Justification::centred);
        ++idx;
    }

    drawReel (g, cl, R, pL, e.guiAngleL.load());
    drawReel (g, cr, R, pR, e.guiAngleR.load());

    // speed readout
    const float spd = e.guiSpeed.load();
    g.setFont (mono (10.0f)); g.setColour (kMuted);
    g.drawText ("SPEED", juce::Rectangle<float> (r.getX() + 10, r.getY() + 8, 80, 12), juce::Justification::centredLeft);
    g.setFont (mono (18.0f, true)); g.setColour (spd < 0.01f ? flux (6) : flux (2));
    g.drawText (spd < 0.01f ? juce::String ("STOP") : juce::String (juce::roundToInt (spd * 100.0f)) + "%",
                juce::Rectangle<float> (r.getX() + 10, r.getY() + 20, 90, 22), juce::Justification::centredLeft);
    const double ips = 15.0 * spd;
    g.setFont (mono (10.0f)); g.setColour (kMuted);
    g.drawText (juce::String (ips, 1) + " IPS", juce::Rectangle<float> (r.getRight() - 90, r.getY() + 8, 80, 12), juce::Justification::centredRight);
}

//==============================================================================
TapeEditor::Ctl& TapeEditor::add (int pi, juce::Colour col, bool fader, const juce::String& name)
{
    const auto& d = paramDefs()[(size_t) pi];
    auto k = std::make_unique<Ctl>();
    k->param = pi;
    k->label = std::make_unique<juce::Label> (juce::String(), name);
    k->label->setJustificationType (juce::Justification::centred);
    k->label->setFont (mono (10.5f, true));
    k->label->setColour (juce::Label::textColourId, kMuted);
    addAndMakeVisible (*k->label);
    if (d.kind == ae8::Bool)
    {
        k->toggle = std::make_unique<juce::ToggleButton> (name == "STOP" ? "STOP" : "ON");
        k->toggle->setColour (juce::ToggleButton::tickColourId, col);
        addAndMakeVisible (*k->toggle);
        k->bAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, d.id, *k->toggle);
    }
    else
    {
        k->slider = std::make_unique<juce::Slider> (fader ? juce::Slider::LinearVertical : juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
        k->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 16);
        k->slider->setColour (juce::Slider::rotarySliderFillColourId, col);
        k->slider->setColour (juce::Slider::trackColourId, col);
        k->slider->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
        addAndMakeVisible (*k->slider);
        k->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, *k->slider);
        k->slider->setDoubleClickReturnValue (true, d.def);
    }
    ctls.push_back (std::move (k));
    return *ctls.back();
}

TapeEditor::Ctl* TapeEditor::find (int pi) { for (auto& c : ctls) if (c->param == pi) return c.get(); return nullptr; }

TapeEditor::TapeEditor (TapeProcessor& p)
    : AudioProcessorEditor (&p), proc (p), reels (p)
{
    setLookAndFeel (&lnf);

    add (INPUT, flux (6), true, "INPUT");
    add (SATURATE, flux (6), true, "SATURATE");
    add (SPEED, flux (2), false, "SPEED");
    add (STOP, flux (6), false, "STOP");
    add (OUTPUT, flux (2), true, "OUTPUT");
    add (MIX, flux (2), true, "MIX");

    bottom =
    {
        { "TAPE",  "FORMULATION", flux (5), false, { BIAS, WOW, FLUTTER, WEAR }, {} },
        { "NOISE", "HISS / NR", flux (3), false, { HISS, CROSSTALK, NR }, {} },
        { "EQ",    "PLAYBACK", flux (4), false, { LOW, HIGH }, {} },
    };
    static const std::map<int, const char*> names = { { BIAS, "BIAS" }, { WOW, "WOW" }, { FLUTTER, "FLUTTER" }, { WEAR, "WEAR" },
        { HISS, "HISS" }, { CROSSTALK, "XTALK" }, { NR, "NR" }, { LOW, "LOW" }, { HIGH, "HIGH" } };
    for (auto& s : bottom) for (int pi : s.params) add (pi, s.colour, false, names.at (pi));

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i).toUpperCase(), i + 1);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    presetBox.setColour (juce::ComboBox::textColourId, flux (2));
    presetBox.setColour (juce::ComboBox::arrowColourId, flux (2));
    presetBox.onChange = [this] { const int i = presetBox.getSelectedId() - 1; if (i >= 0 && i != proc.getCurrentProgram()) { proc.setCurrentProgram (i); shownProgram = i; } };
    prevBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (presetBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);
    addAndMakeVisible (reels);

    scaler.init (*this, kW, kH);
    startTimerHz (30);
#ifdef AE8_TAPE_DEMO
    proc.startDemo();
#endif
}

TapeEditor::~TapeEditor() { setLookAndFeel (nullptr); }

void TapeEditor::timerCallback()
{
    auto& e = proc.engine;
    auto fall = [] (float& v, float nv) { v = juce::jmax (nv, v * 0.85f); };
    fall (inL, e.guiInL.load()); fall (inR, e.guiInR.load()); fall (outL, e.guiOutL.load()); fall (outR, e.guiOutR.load());
    reels.repaint();
    repaint (scaler.toWindow (*this, inMeter)); repaint (scaler.toWindow (*this, outMeter));
    if (proc.getCurrentProgram() != shownProgram) { shownProgram = proc.getCurrentProgram(); presetBox.setSelectedId (shownProgram + 1, juce::dontSendNotification); }
}

void TapeEditor::drawMeter (juce::Graphics& g, juce::Rectangle<int> a, float l, float r, const juce::String& label)
{
    drawGlass (g, a.toFloat(), 5.0f);
    auto body = a.reduced (5, 6).withTrimmedBottom (14);
    const int bw = (body.getWidth() - 3) / 2;
    int ci = 0;
    for (float v : { l, r })
    {
        auto bar = body.withX (body.getX() + ci * (bw + 3)).withWidth (bw);
        const float db = juce::Decibels::gainToDecibels (v, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);   // -60..+6 dB
        const int segs = 22;
        for (int s = 0; s < segs; ++s)
        {
            const float sf = (float) s / (float) segs;
            auto seg = juce::Rectangle<float> ((float) bar.getX(), (float) bar.getBottom() - (float) bar.getHeight() * (sf + 1.0f / segs) + 1, (float) bar.getWidth(), (float) bar.getHeight() / segs - 2);
            const int fi = sf > 0.9f ? 6 : sf > 0.8f ? 5 : sf > 0.7f ? 4 : sf > 0.45f ? 3 : 2;
            if (sf < frac) { g.setColour (flux (fi).withAlpha (0.25f)); g.fillRect (seg.expanded (1.0f)); g.setColour (flux (fi)); }
            else g.setColour (juce::Colour (0xff15171c));
            g.fillRoundedRectangle (seg, 1.0f);
        }
        ++ci;
    }
    g.setFont (mono (9.0f, true)); g.setColour (kMuted);
    g.drawText (label, a.removeFromBottom (16), juce::Justification::centred);
}

void TapeEditor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.scale (*this)));
    drawWindow (g, scaler.design());
    drawHeader (g, headerArea, "TAPE", "T-02 / TAPE MACHINE");
    g.setFont (tracked (mono (9.5f, true), 0.12f)); g.setColour (kMuted);
    g.drawText ("PROGRAM", presetBox.getBounds().translated (-80, 0).withWidth (72), juce::Justification::centredRight);

    drawSection (g, recArea, "RECORD", "IN / DRIVE", flux (6), false);
    drawSection (g, transArea, "TRANSPORT", "REELS / SPEED", flux (1), true);
    drawSection (g, outArea, "OUTPUT", "OUT / BLEND", flux (2), false);
    for (auto& s : bottom) drawSection (g, s.bounds, s.title, s.tag, s.colour, s.light);
    drawMeter (g, inMeter, inL, inR, "IN");
    drawMeter (g, outMeter, outL, outR, "OUT");
}

void TapeEditor::resized()
{
    auto area = scaler.design().reduced (kGap, 0);
    area.removeFromTop (kGap);
    headerArea = area.removeFromTop (kHeader);
    area.removeFromTop (kGap);
    auto right = headerArea.reduced (14, 15);
    nextBtn.setBounds (right.removeFromRight (34)); right.removeFromRight (6);
    prevBtn.setBounds (right.removeFromRight (34)); right.removeFromRight (8);
    presetBox.setBounds (right.removeFromRight (300));

    auto top = area.removeFromTop (kTopH);
    area.removeFromTop (kGap);
    recArea = top.removeFromLeft (kSideW); top.removeFromLeft (kGap);
    outArea = top.removeFromRight (kSideW); top.removeFromRight (kGap);
    transArea = top;

    auto placeFader = [this] (int pi, juce::Rectangle<int> cell)
    {
        if (auto* c = find (pi)) { c->label->setBounds (cell.removeFromTop (16)); c->slider->setBounds (cell); }
    };
    {   // record: meter + two faders
        auto in = recArea.withTrimmedTop (22).reduced (10, 8);
        inMeter = in.removeFromLeft (46);
        in.removeFromLeft (8);
        const int w = in.getWidth() / 2;
        placeFader (INPUT, in.removeFromLeft (w));
        placeFader (SATURATE, in);
    }
    {   // output: two faders + meter
        auto in = outArea.withTrimmedTop (22).reduced (10, 8);
        outMeter = in.removeFromRight (46);
        in.removeFromRight (8);
        const int w = in.getWidth() / 2;
        placeFader (OUTPUT, in.removeFromLeft (w));
        placeFader (MIX, in);
    }
    {   // transport: reels + speed knob + stop
        auto in = transArea.withTrimmedTop (22).reduced (10, 8);
        auto side = in.removeFromRight (140);
        in.removeFromRight (10);
        reels.setBounds (in);
        if (auto* c = find (SPEED))
        {
            c->label->setBounds (side.removeFromTop (16));
            c->slider->setBounds (side.removeFromTop (150).withSizeKeepingCentre (130, 150));
            c->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
        }
        side.removeFromTop (14);
        if (auto* c = find (STOP))
        {
            c->label->setBounds (side.removeFromTop (16));
            c->toggle->setBounds (side.removeFromTop (40).reduced (10, 0));
        }
    }

    auto row = area.removeFromTop (kKnobH);
    int cells = 0; for (auto& s : bottom) cells += (int) s.params.size();
    const int avail = row.getWidth() - kGap * ((int) bottom.size() - 1);
    int x = row.getX();
    for (size_t k = 0; k < bottom.size(); ++k)
    {
        auto& sec = bottom[k];
        const int n = (int) sec.params.size();
        const int w = (k == bottom.size() - 1) ? row.getRight() - x : avail * n / cells;
        sec.bounds = { x, row.getY(), w, kKnobH };
        x += w + kGap;
        auto in = sec.bounds.withTrimmedTop (22).reduced (4, 2);
        const int cw = in.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto cell = in.removeFromLeft (i == n - 1 ? in.getWidth() : cw);
            if (auto* c = find (sec.params[(size_t) i]))
            {
                c->label->setBounds (cell.removeFromTop (16));
                if (c->slider) c->slider->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 78), cell.getHeight()));
                else           c->toggle->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth() - 16, 80), 32).translated (0, -8));
            }
        }
    }
    scaler.apply (*this);
}
