#include "TuneEditor.h"

using namespace ae8tune;
using namespace ae8brand;

namespace
{
const char* kNoteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
juce::String noteName (int midi) { return juce::String (kNoteNames[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 2); }  // C3 = 60 (Ableton)
constexpr int kW = 1180, kHeader = 64, kGap = 8, kTunerH = 220, kKnobH = 118;
}

//==============================================================================
void PitchView::push()
{
    auto& e = proc.engine;
    const bool v = e.guiVoiced.load();
    inH[(size_t) head] = e.guiInMidi.load();
    outH[(size_t) head] = e.guiOutMidi.load();
    vH[(size_t) head] = v;
    head = (head + 1) % kN;
    if (v) centre += (e.guiOutMidi.load() - centre) * 0.08f;
    repaint();
}

void PitchView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (juce::Colours::black); g.fillRoundedRectangle (r, 4.0f);
    g.setColour (kLine); g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    auto plot = r.reduced (8.0f, 6.0f).withTrimmedLeft (34.0f);

    const float span = 14.0f;                         // semitones visible
    const float lo = std::round (centre) - span / 2;
    auto yOf = [&] (float m) { return plot.getBottom() - (m - lo) / span * plot.getHeight(); };

    const int key = (int) proc.apvts.getRawParameterValue ("key")->load();
    const int msk = scaleMask ((int) proc.apvts.getRawParameterValue ("scale")->load());
    for (int n = (int) std::floor (lo); n <= (int) std::ceil (lo + span); ++n)
    {
        const float y = yOf ((float) n);
        if (y < plot.getY() - 1 || y > plot.getBottom() + 1) continue;
        const bool on = inScale (n, key, msk), root = (((n - key) % 12) + 12) % 12 == 0;
        g.setColour (root ? flux (2).withAlpha (0.35f) : (on ? kInk.withAlpha (0.13f) : kInk.withAlpha (0.04f)));
        g.drawHorizontalLine ((int) y, plot.getX(), plot.getRight());
        if (on)
        {
            g.setFont (mono (9.5f, root));
            g.setColour (root ? flux (2) : kMuted);
            g.drawText (noteName (n), (int) r.getX() + 6, (int) y - 6, 32, 12, juce::Justification::centredLeft);
        }
    }

    // history: oldest on the left
    const float dx = plot.getWidth() / (float) (kN - 1);
    juce::Path outPath; bool pen = false;
    for (int i = 0; i < kN; ++i)
    {
        const int k = (head + i) % kN;
        const float x = plot.getX() + dx * (float) i;
        if (! vH[(size_t) k]) { pen = false; continue; }
        const float yi = yOf (inH[(size_t) k]), yo = yOf (outH[(size_t) k]);
        g.setColour (kInk.withAlpha (0.45f));
        g.fillEllipse (x - 1.5f, yi - 1.5f, 3.0f, 3.0f);
        if (! pen) { outPath.startNewSubPath (x, yo); pen = true; } else outPath.lineTo (x, yo);
    }
    g.setColour (flux (5));
    g.strokePath (outPath, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    g.setFont (mono (9.5f));
    g.setColour (kMuted);
    g.drawText ("IN", plot.getRight() - 90, plot.getY(), 30, 12, juce::Justification::centredRight);
    g.setColour (flux (5));
    g.drawText ("OUT", plot.getRight() - 50, plot.getY(), 40, 12, juce::Justification::centredRight);
}

//==============================================================================
void KeyStrip::paint (juce::Graphics& g)
{
    const int key = (int) proc.apvts.getRawParameterValue ("key")->load();
    const int msk = scaleMask ((int) proc.apvts.getRawParameterValue ("scale")->load());
    const int tgt = proc.engine.guiTarget.load();
    const float w = (float) getWidth() / 12.0f;
    for (int i = 0; i < 12; ++i)
    {
        auto cell = juce::Rectangle<float> (w * (float) i, 0, w, (float) getHeight()).reduced (2.0f);
        const bool sharp = juce::String (kNoteNames[i]).contains ("#");
        const bool on = (msk >> (((i - key) % 12 + 12) % 12)) & 1;
        const bool root = i == key;
        g.setColour (root ? flux (2) : (on ? kRaised : kBg));
        g.fillRoundedRectangle (cell, 4.0f);
        g.setColour (sharp && ! root ? kLine.brighter (0.2f) : kLine);
        if (! root) g.drawRoundedRectangle (cell, 4.0f, 1.0f);
        if (tgt >= 0 && ((tgt % 12) + 12) % 12 == i)
        {
            g.setColour (flux (5)); g.drawRoundedRectangle (cell.reduced (1.0f), 4.0f, 2.0f);
        }
        g.setColour (root ? kBg : (on ? kInk : kMuted.withAlpha (0.6f)));
        g.setFont (mono (12.0f, true));
        g.drawText (kNoteNames[i], cell, juce::Justification::centred);
    }
}

void KeyStrip::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, 11, (int) (e.position.x / ((float) getWidth() / 12.0f)));
    if (auto* p = proc.apvts.getParameter ("key")) p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
    repaint();
}

//==============================================================================
TuneEditor::TuneEditor (TuneProcessor& p)
    : AudioProcessorEditor (&p), proc (p), pitchView (p), keyStrip (p)
{
    setLookAndFeel (&lnf);

    sections =
    {
        { "TUNE",       "CORRECTION", flux (5), false, { SPEED, AMOUNT, HUMANIZE, SHIFT }, {} },
        { "VOICES",     "DOUBLE / HARMONY", flux (4), false, { DOUBLE, HARMONY, HARM_INT }, {} },
        { "DISTORTION", "DRIVE", flux (6), false, { DIST_DRIVE, DIST_TONE, DIST_MIX }, {} },
        { "DELAY",      "SYNC", flux (1), true, { DLY_TIME, DLY_FB, DLY_MIX }, {} },
        { "REVERB",     "SPACE", flux (0), true, { REV_SIZE, REV_MIX }, {} },
        { "OUTPUT",     "OUT", flux (2), false, { MIX, OUTPUT }, {} },
    };
    static const std::map<int, const char*> names =
    {
        { SPEED, "SPEED" }, { AMOUNT, "AMOUNT" }, { HUMANIZE, "HUMANIZE" }, { SHIFT, "SHIFT" },
        { DOUBLE, "DOUBLE" }, { HARMONY, "HARMONY" }, { HARM_INT, "INTERVAL" },
        { DIST_DRIVE, "DRIVE" }, { DIST_TONE, "TONE" }, { DIST_MIX, "MIX" },
        { DLY_TIME, "TIME" }, { DLY_FB, "FDBK" }, { DLY_MIX, "MIX" },
        { REV_SIZE, "SIZE" }, { REV_MIX, "MIX" }, { MIX, "DRY/WET" }, { OUTPUT, "OUTPUT" },
    };

    for (auto& sec : sections)
        for (int pi : sec.params)
        {
            const auto& d = paramDefs()[(size_t) pi];
            auto k = std::make_unique<Knob>();
            k->param = pi;
            k->label = std::make_unique<juce::Label> (juce::String(), names.at (pi));
            k->label->setJustificationType (juce::Justification::centred);
            k->label->setFont (mono (10.5f, true));
            k->label->setColour (juce::Label::textColourId, kMuted);
            addAndMakeVisible (*k->label);
            if (d.kind == ae8::Choice)
            {
                k->combo = std::make_unique<juce::ComboBox>();
                juce::StringArray items; for (auto* c : d.choices) items.add (c);
                k->combo->addItemList (items, 1);
                addAndMakeVisible (*k->combo);
                k->cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, d.id, *k->combo);
            }
            else
            {
                k->slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
                k->slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 16);
                k->slider->setColour (juce::Slider::rotarySliderFillColourId, sec.colour == flux (0) ? flux (2) : sec.colour);
                k->slider->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
                k->slider->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
                addAndMakeVisible (*k->slider);
                k->sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, *k->slider);
                k->slider->setDoubleClickReturnValue (true, d.def);
            }
            knobs.push_back (std::move (k));
        }

    // style display
    for (int i = 0; i < proc.getNumPrograms(); ++i) styleBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i).toUpperCase(), i + 1);
    styleBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    styleBox.setColour (juce::ComboBox::textColourId, flux (5));
    styleBox.setColour (juce::ComboBox::arrowColourId, flux (5));
    styleBox.onChange = [this] { const int i = styleBox.getSelectedId() - 1; if (i >= 0 && i != proc.getCurrentProgram()) { proc.setCurrentProgram (i); shownProgram = i; } };
    prevBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (styleBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);

    juce::StringArray scales; for (auto* c : paramDefs()[SCALE].choices) scales.add (c);
    scaleBox.addItemList (scales, 1);
    addAndMakeVisible (scaleBox);
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "scale", scaleBox);

    addAndMakeVisible (pitchView);
    addAndMakeVisible (keyStrip);

    setSize (kW, kGap + kHeader + kGap + kTunerH + kGap + kKnobH + kGap);
    startTimerHz (30);
#ifdef AE8_TUNE_DEMO
    proc.startDemo();
#endif
}

TuneEditor::~TuneEditor() { setLookAndFeel (nullptr); }

void TuneEditor::timerCallback()
{
    pitchView.push();
    keyStrip.repaint();
    repaint (tunerArea.withWidth (300));
    if (proc.getCurrentProgram() != shownProgram) { shownProgram = proc.getCurrentProgram(); styleBox.setSelectedId (shownProgram + 1, juce::dontSendNotification); }
}

void TuneEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);
    drawHeader (g, headerArea, "TUNE", "V-03 / PITCH CORRECTION");
    g.setFont (mono (10.5f, true)); g.setColour (kMuted);
    g.drawText ("STYLE", styleBox.getBounds().translated (-70, 0).withWidth (62), juce::Justification::centredRight);

    // tuner panel
    drawSection (g, tunerArea, "PITCH", "KEY / SCALE", flux (2), false);
    auto left = tunerArea.withTrimmedTop (20).reduced (14, 10).withWidth (270);
    auto& e = proc.engine;
    const bool v = e.guiVoiced.load();
    const int tgt = e.guiTarget.load();
    auto readout = left.removeFromTop (88);
    g.setColour (juce::Colours::black); g.fillRoundedRectangle (readout.toFloat(), 4.0f);
    g.setColour (kLine); g.drawRoundedRectangle (readout.toFloat().reduced (0.5f), 4.0f, 1.0f);
    g.setColour (v && tgt >= 0 ? flux (5) : kLine.brighter (0.3f));
    g.setFont (mono (46.0f, true));
    g.drawText (v && tgt >= 0 ? noteName (tgt) : "--", readout.withTrimmedBottom (26), juce::Justification::centred);
    // cents meter: how far the singer is from the target note
    auto meter = readout.removeFromBottom (24).reduced (14, 8);
    g.setColour (kLine); g.fillRect (meter);
    const float cents = v && tgt >= 0 ? juce::jlimit (-50.0f, 50.0f, (e.guiInMidi.load() - (float) tgt) * 100.0f) : 0.0f;
    const float cx = (float) meter.getCentreX(), px = cx + cents / 50.0f * (float) meter.getWidth() * 0.5f;
    const int fi = juce::jlimit (2, 6, 2 + (int) (std::fabs (cents) / 50.0f * 4.99f));
    g.setColour (flux (fi));
    g.fillRect (juce::Rectangle<float> (juce::jmin (cx, px), (float) meter.getY(), std::fabs (px - cx) + 2.0f, (float) meter.getHeight()));
    g.setColour (kInk); g.fillRect (cx - 0.5f, (float) meter.getY() - 3, 1.0f, (float) meter.getHeight() + 6);
    g.setFont (mono (9.5f)); g.setColour (kMuted);
    g.drawText (v ? (cents >= 0 ? "+" : "") + juce::String (juce::roundToInt (cents)) + " CT" : "", readout.getRight() - 64, readout.getY() + 4, 58, 12, juce::Justification::centredRight);

    for (auto& s : sections) drawSection (g, s.bounds, s.title, s.tag, s.colour, s.light);
}

void TuneEditor::resized()
{
    auto area = getLocalBounds().reduced (kGap, 0);
    area.removeFromTop (kGap);
    headerArea = area.removeFromTop (kHeader);
    area.removeFromTop (kGap);
    auto right = headerArea.reduced (14, 15);
    nextBtn.setBounds (right.removeFromRight (34)); right.removeFromRight (6);
    prevBtn.setBounds (right.removeFromRight (34)); right.removeFromRight (8);
    styleBox.setBounds (right.removeFromRight (300));

    tunerArea = area.removeFromTop (kTunerH);
    area.removeFromTop (kGap);
    auto inner = tunerArea.withTrimmedTop (20).reduced (14, 10);
    auto left = inner.removeFromLeft (270);
    left.removeFromTop (88 + 10);
    keyStrip.setBounds (left.removeFromTop (34));
    left.removeFromTop (8);
    scaleBox.setBounds (left.removeFromTop (26));
    inner.removeFromLeft (14);
    pitchView.setBounds (inner);

    auto row = area.removeFromTop (kKnobH);
    int cells = 0; for (auto& s : sections) cells += (int) s.params.size();
    const int avail = row.getWidth() - kGap * ((int) sections.size() - 1);
    int x = row.getX();
    for (size_t k = 0; k < sections.size(); ++k)
    {
        auto& sec = sections[k];
        const int n = (int) sec.params.size();
        const int w = (k == sections.size() - 1) ? row.getRight() - x : avail * n / cells;
        sec.bounds = { x, row.getY(), w, kKnobH };
        x += w + kGap;
        auto in = sec.bounds.withTrimmedTop (22).reduced (4, 2);
        const int cw = in.getWidth() / n;
        for (int i = 0; i < n; ++i)
        {
            auto cell = in.removeFromLeft (i == n - 1 ? in.getWidth() : cw);
            for (auto& kb : knobs)
                if (kb->param == sec.params[(size_t) i])
                {
                    auto cc = cell;
                    kb->label->setBounds (cc.removeFromTop (16));
                    if (kb->slider) kb->slider->setBounds (cc.withSizeKeepingCentre (juce::jmin (cc.getWidth(), 78), cc.getHeight()));
                    else            kb->combo->setBounds (cc.withSizeKeepingCentre (juce::jmin (cc.getWidth() - 2, 92), 24).translated (0, -6));
                }
        }
    }
}
