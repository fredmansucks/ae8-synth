#include "ChopEditor.h"

using namespace ae8chop;
using namespace ae8brand;

namespace
{
constexpr int kW = 1180, kH = 760, kGap = 8, kHeader = 64, kSampleH = 268, kSideW = 300;
const char* kNotes[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
juce::String noteName (int midi) { return juce::String (kNotes[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 2); }   // Ableton: 36 = C1

juce::String sliceText (int k, double v)
{
    switch (k)
    {
        case S_PITCH:   return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ST";
        case S_FINE:    return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " CT";
        case S_STRETCH: return juce::String (juce::roundToInt (v * 100)) + "%";
        case S_LEVEL:   return juce::String (v, 1) + " dB";
        case S_PAN:     return std::abs (v) < 0.02 ? juce::String ("C") : (v < 0 ? "L" : "R") + juce::String (juce::roundToInt (std::abs (v) * 50));
        case S_START:   return (v > 0 ? "+" : "") + juce::String (juce::roundToInt (v)) + " ms";
        case S_LENGTH:  return juce::String (juce::roundToInt (v * 100)) + "%";
        case S_ATTACK:
        case S_RELEASE: return juce::String (juce::roundToInt (v)) + " ms";
        case S_FILTER:  return v >= 19999 ? juce::String ("OPEN") : (v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v)));
        default:        return juce::String (v, 2);
    }
}
} // namespace

//==============================================================================
void ChopLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                        juce::Slider::SliderStyle style, juce::Slider& s)
{
    if (style != juce::Slider::LinearBar) { LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, pos, 0, 0, style, s); return; }
    auto r = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (0.5f);
    drawGlass (g, r, 4.0f);
    const auto col = s.findColour (juce::Slider::trackColourId);
    auto fill = r.reduced (2.0f).withRight (juce::jmax (r.getX() + 4, pos - 2));
    g.setColour (col.withAlpha (0.16f)); g.fillRoundedRectangle (fill, 3.0f);
    g.setColour (col); g.fillRect (fill.getX(), fill.getBottom() - 1.5f, fill.getWidth(), 1.5f);
    g.setColour (kInk); g.setFont (tracked (mono (10.5f, true), 0.06f));
    g.drawText (s.getTextFromValue (s.getValue()), r, juce::Justification::centred);
}

void Chip::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    const bool on = getToggleState() && ! action;
    if (on || action)
    {
        ae8brand::LookAndFeel::drawRaised (g, r.reduced (0.5f), over, down);
        if (on)
        {
            g.setColour (onColour.withAlpha (0.16f)); g.fillRoundedRectangle (r.reduced (1.5f), 4.0f);
            auto bar = r.reduced (5.0f, 0).removeFromBottom (2.5f).translated (0, -2.0f);
            g.setColour (onColour.withAlpha (0.35f)); g.fillRoundedRectangle (bar.expanded (1.5f), 2.0f);
            g.setColour (onColour); g.fillRoundedRectangle (bar, 1.0f);
        }
    }
    else
    {
        drawGlass (g, r, 4.0f);
        if (over) { g.setColour (juce::Colours::white.withAlpha (0.04f)); g.fillRoundedRectangle (r, 4.0f); }
    }
    g.setColour (on || action ? kInk : kMuted);
    g.setFont (tracked (mono (10.0f, true), 0.06f));
    g.drawText (getButtonText(), r.translated (0, on ? -0.5f : 0.0f), juce::Justification::centred);
}

ChoiceChips::ChoiceChips (juce::RangedAudioParameter& p, juce::StringArray labels, juce::Colour on, std::vector<int> values)
    : param (p), vals (std::move (values)), att (p, [this] (float v) { update (v); })
{
    if (vals.empty()) for (int i = 0; i < labels.size(); ++i) vals.push_back (i);
    for (int i = 0; i < labels.size(); ++i)
    {
        auto c = std::make_unique<Chip> (labels[i], on);
        c->setClickingTogglesState (false);
        const int v = vals[(size_t) i];
        c->onClick = [this, v] { att.setValueAsCompleteGesture ((float) v); };
        addAndMakeVisible (*c);
        chips.push_back (std::move (c));
    }
    att.sendInitialUpdate();
}

void ChoiceChips::update (float v)
{
    for (size_t i = 0; i < chips.size(); ++i) chips[i]->setToggleState (juce::roundToInt (v) == vals[i], juce::dontSendNotification);
}

void ChoiceChips::resized()
{
    auto r = getLocalBounds();
    const int n = (int) chips.size();
    for (int i = 0; i < n; ++i)
    {
        const int w = i < (int) widths.size() ? widths[(size_t) i] : (r.getWidth() - 5 * (n - 1 - i)) / (n - i);
        chips[(size_t) i]->setBounds (r.removeFromLeft (w));
        r.removeFromLeft (5);
    }
}

void MidiDragChip::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 4) return;
    dragging = true;
    const auto f = proc.exportMidi();
    if (f.existsAsFile())
        juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}

//==============================================================================
double WaveView::xToPos (float x) const { return viewStart + (double) x / juce::jmax (1, getWidth()) * viewLen; }
float WaveView::posToX (double p) const { return viewLen > 0 ? (float) ((p - viewStart) / viewLen * getWidth()) : 0.0f; }

void WaveView::clampView()
{
    auto s = proc.currentSample();
    const double len = s ? (double) s->size() : 0;
    if (len <= 0) { viewStart = 0; viewLen = 0; return; }
    if (viewLen <= 0 || viewLen > len) viewLen = len;
    viewLen = juce::jmax (viewLen, juce::jmin (len, 512.0));
    viewStart = juce::jlimit (0.0, len - viewLen, viewStart);
}

int WaveView::sliceAt (double pos) const
{
    const auto m = proc.markers();
    int si = -1;
    for (size_t i = 0; i < m.size(); ++i) if ((double) m[i] <= pos) si = (int) i;
    return si;
}

int WaveView::flagAt (juce::Point<float> p) const
{
    if (p.y > 16) return -1;
    const auto m = proc.markers();
    for (int i = (int) m.size() - 1; i >= 0; --i)
    {
        const float x = posToX ((double) m[(size_t) i]);
        if (p.x >= x - 2 && p.x <= x + 20) return i;
    }
    return -1;
}

void WaveView::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    drawGlass (g, r, 6.0f);
    auto s = proc.currentSample();
    if (! s || s->size() == 0)
    {
        g.setColour (dragHover ? flux (2) : kLine); g.drawRoundedRectangle (r.reduced (6), 4.0f, 1.5f);
        g.setColour (kMuted); g.setFont (mono (13.0f, true));
        g.drawText ("DROP A SAMPLE HERE  |  OR CLICK TO BROWSE", r, juce::Justification::centred);
        g.setFont (mono (10.5f)); g.setColour (kMuted.withAlpha (0.6f));
        g.drawText ("WAV  AIFF  FLAC  MP3  OGG", r.translated (0, 22), juce::Justification::centred);
        return;
    }
    clampView();
    const double len = (double) s->size();
    const auto& ov = proc.overview;
    float norm = 1e-6f;
    for (auto& b : ov) norm = std::max ({ norm, b.lo, b.mid, b.hi });
    const auto m = proc.markers();
    const int sel = proc.engine.selected.load();

    // selected slice background
    if (sel >= 0 && sel < (int) m.size())
    {
        const float x0 = posToX ((double) m[(size_t) sel]), x1 = posToX (sel + 1 < (int) m.size() ? (double) m[(size_t) sel + 1] : len);
        g.setColour (kInk.withAlpha (0.08f)); g.fillRect (juce::Rectangle<float> (x0, 0, x1 - x0, r.getHeight()));
    }

    const float mid = r.getHeight() * 0.5f + 6, H = r.getHeight() * 0.5f - 12;
    const int W = getWidth();
    const double spp = viewLen / W;          // samples per pixel
    const bool fine = spp < len / ChopProcessor::kOverview;
    for (int x = 0; x < W; x += 2)
    {
        const double a = viewStart + x * spp, b = a + 2 * spp;
        const int c0 = (int) (a / len * ov.size()), c1 = juce::jmax (c0 + 1, (int) (b / len * ov.size()));
        ChopProcessor::Band band { 0, 0, 0 };
        for (int c = c0; c < c1 && c < (int) ov.size(); ++c) { band.lo = std::max (band.lo, ov[(size_t) c].lo); band.mid = std::max (band.mid, ov[(size_t) c].mid); band.hi = std::max (band.hi, ov[(size_t) c].hi); }
        float amp = std::max ({ band.lo, band.mid, band.hi }) / norm;
        if (fine)
        {
            float pk = 0;
            for (int64_t i = (int64_t) a; i < (int64_t) b && i < (int64_t) len; ++i) pk = std::max (pk, std::fabs (0.5f * (s->L[(size_t) i] + s->R[(size_t) i])));
            const float scale = amp > 0 ? (pk / norm) / amp : 0.0f;
            band.lo *= scale; band.mid *= scale; band.hi *= scale; amp = pk / norm;
        }
        const float lay[4] = { std::min (1.0f, amp), std::min (1.0f, (band.mid * 0.75f + band.hi * 0.45f) / norm), std::min (1.0f, (band.mid * 0.35f + band.hi * 0.7f) / norm), std::min (1.0f, band.hi * 0.6f / norm) };
        const juce::Colour col[4] = { flux (6), flux (5), flux (2), flux (1) };
        for (int k = 0; k < 4; ++k)
        {
            const float h = lay[k] * H;
            g.setColour (col[k]); g.fillRect ((float) x, mid - h, 1.4f, h * 2);
        }
    }

    // markers
    for (size_t i = 0; i < m.size(); ++i)
    {
        const float x = posToX ((double) m[i]);
        if (x < -20 || x > W) continue;
        const auto c = sliceColour ((int) i);
        g.setColour (c); g.fillRect (x - ((int) i == sel ? 1.0f : 0.5f), 0.0f, (int) i == sel ? 2.0f : 1.2f, r.getHeight());
        g.fillRect (x, 0.0f, 18.0f, 13.0f);
        g.setColour (kBg); g.setFont (mono (9.0f, true));
        g.drawText (juce::String ((int) i + 1), juce::Rectangle<float> (x, 0, 18, 13), juce::Justification::centred);
    }
    if (dragMarker >= 0 && dragMarker < (int) dragCopy.size())
    {
        const float x = posToX ((double) dragCopy[(size_t) dragMarker]);
        g.setColour (kInk); g.fillRect (x - 1, 0.0f, 2.0f, r.getHeight());
    }

    // playhead
    const float ph = proc.engine.guiPlayhead.load();
    if (proc.engine.guiPlaying.load() != 0 && ph >= 0)
    {
        g.setColour (kInk); g.fillRect (posToX (ph * len) - 0.75f, 0.0f, 1.5f, r.getHeight());
    }
    if (dragHover) { g.setColour (flux (2)); g.drawRoundedRectangle (r.reduced (2), 4.0f, 2.0f); }
}

void WaveView::mouseDown (const juce::MouseEvent& e)
{
    auto s = proc.currentSample();
    if (! s || s->size() == 0) { browse(); return; }
    const int f = flagAt (e.position);
    if (f >= 0 && e.mods.isPopupMenu())
    {
        auto m = proc.markers();
        if (m.size() > 1) { m.erase (m.begin() + f); proc.setManualMarkers (m); }
        return;
    }
    if (f >= 0) { dragMarker = f; dragCopy = proc.markers(); return; }
    const int si = sliceAt (xToPos (e.position.x));
    if (si >= 0)
    {
        proc.engine.selected = si;
        if (onSelect) onSelect();
        auditionNote = proc.apvts.getRawParameterValue ("layout")->load() > 0.5f ? 60 : 36 + si;
        proc.keyboardState.noteOn (1, auditionNote, 0.85f);
    }
}

void WaveView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragMarker < 0) return;
    auto s = proc.currentSample(); if (! s) return;
    const double lo = dragMarker > 0 ? (double) dragCopy[(size_t) dragMarker - 1] + 64 : 0;
    const double hi = dragMarker + 1 < (int) dragCopy.size() ? (double) dragCopy[(size_t) dragMarker + 1] - 64 : (double) s->size() - 64;
    dragCopy[(size_t) dragMarker] = (int64_t) juce::jlimit (lo, hi, xToPos (e.position.x));
    repaint();
}

void WaveView::mouseUp (const juce::MouseEvent&)
{
    if (dragMarker >= 0) { proc.setManualMarkers (dragCopy); dragMarker = -1; }
    if (auditionNote >= 0) { proc.keyboardState.noteOff (1, auditionNote, 0.0f); auditionNote = -1; }
}

void WaveView::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto s = proc.currentSample();
    if (! s || s->size() == 0 || flagAt (e.position) >= 0) return;
    if (auditionNote >= 0) { proc.keyboardState.noteOff (1, auditionNote, 0.0f); auditionNote = -1; }
    auto m = proc.markers();
    if ((int) m.size() >= kMaxSlices) return;
    m.push_back ((int64_t) xToPos (e.position.x));
    proc.setManualMarkers (m);
}

void WaveView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    auto s = proc.currentSample(); if (! s) return;
    const double anchor = xToPos (e.position.x);
    const double f = std::pow (0.85, (w.deltaY != 0 ? w.deltaY : w.deltaX) * 4.0);
    const double newLen = viewLen * f;
    viewStart = anchor - (anchor - viewStart) * (newLen / viewLen);
    viewLen = newLen;
    clampView();
    repaint();
    if (onViewChanged) onViewChanged();
}

void WaveView::browse()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a sample", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [this] (const juce::FileChooser& fc)
                          {
                              if (fc.getResult().existsAsFile() && proc.loadFile (fc.getResult())) { viewStart = 0; viewLen = 0; repaint(); if (onSelect) onSelect(); }
                          });
}

void WaveView::filesDropped (const juce::StringArray& files, int, int)
{
    dragHover = false;
    for (auto& f : files)
        if (proc.loadFile (juce::File (f))) { viewStart = 0; viewLen = 0; if (onSelect) onSelect(); break; }
    repaint();
}

//==============================================================================
void OverviewStrip::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    drawGlass (g, r, 4.0f);
    const auto& ov = proc.overview;
    auto s = proc.currentSample();
    if (ov.empty() || ! s) return;
    float norm = 1e-6f; for (auto& b : ov) norm = std::max ({ norm, b.lo, b.mid, b.hi });
    const int W = getWidth();
    g.setColour (kMuted.withAlpha (0.7f));
    for (int x = 0; x < W; x += 3)
    {
        const int c = (int) ((double) x / W * ov.size());
        const float a = std::max ({ ov[(size_t) c].lo, ov[(size_t) c].mid, ov[(size_t) c].hi }) / norm;
        const float h = juce::jmax (1.0f, a * (r.getHeight() * 0.5f - 1));
        g.fillRect ((float) x, r.getCentreY() - h, 2.0f, h * 2);
    }
    const double len = (double) s->size();
    const float x0 = (float) (wave.viewStart / len * W), w = (float) (wave.viewLen / len * W);
    g.setColour (flux (2)); g.drawRect (juce::Rectangle<float> (x0, 0.5f, juce::jmax (4.0f, w), r.getHeight() - 1), 1.5f);
}

void OverviewStrip::mouseDrag (const juce::MouseEvent& e)
{
    auto s = proc.currentSample(); if (! s) return;
    const double len = (double) s->size();
    wave.viewStart = e.position.x / getWidth() * len - wave.viewLen * 0.5;
    wave.viewStart = juce::jlimit (0.0, juce::jmax (0.0, len - wave.viewLen), wave.viewStart);
    wave.repaint(); repaint();
}

//==============================================================================
int PadGrid::padAt (juce::Point<float> p) const
{
    const float w = (float) getWidth() / 4, h = (float) getHeight() / 4;
    const int col = juce::jlimit (0, 3, (int) (p.x / w)), row = juce::jlimit (0, 3, (int) (p.y / h));
    return (3 - row) * 4 + col;        // pad 1 bottom-left, like an MPC
}

int PadGrid::noteFor (int pad) const
{
    const bool pitch = proc.apvts.getRawParameterValue ("layout")->load() > 0.5f;
    return pitch ? 60 + pad : 36 + bank * 16 + pad;
}

void PadGrid::paint (juce::Graphics& g)
{
    const bool pitch = proc.apvts.getRawParameterValue ("layout")->load() > 0.5f;
    const int ns = proc.engine.numSlices.load(), sel = proc.engine.selected.load();
    const uint32_t playing = proc.engine.guiPlaying.load();
    const float w = (float) getWidth() / 4, h = (float) getHeight() / 4;
    for (int pad = 0; pad < 16; ++pad)
    {
        const int row = 3 - pad / 4, col = pad % 4;
        auto cell = juce::Rectangle<float> (w * col, h * row, w, h).reduced (3.0f);
        const int si = pitch ? sel : bank * 16 + pad;
        const bool exists = si < ns;
        const auto c = exists ? sliceColour (si) : kLine;
        const int note = noteFor (pad);
        const bool lit = pitch ? proc.keyboardState.isNoteOn (1, note) : (exists && (playing >> si) & 1u);
        const bool isSel = ! pitch && si == sel;
        // soft-touch pad: shadow, rubber body, coloured ring that lights up when hit
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (cell.translated (0, 1.5f), 7.0f);
        if (lit)
        {
            g.setColour (c.withAlpha (0.30f)); g.fillRoundedRectangle (cell.expanded (3.0f), 9.0f);
            g.setGradientFill (juce::ColourGradient (c.brighter (0.35f), cell.getX(), cell.getY(), c.darker (0.2f), cell.getX(), cell.getBottom(), false));
        }
        else
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff23262e), cell.getX(), cell.getY(), juce::Colour (0xff121419), cell.getX(), cell.getBottom(), false));
        g.fillRoundedRectangle (cell, 7.0f);
        if (isSel && ! lit) { g.setColour (c.withAlpha (0.12f)); g.fillRoundedRectangle (cell, 7.0f); }
        g.setColour (exists ? c.withAlpha (lit ? 1.0f : (isSel ? 0.95f : 0.55f)) : kLine);
        g.drawRoundedRectangle (cell.reduced (0.75f), 6.5f, isSel ? 2.0f : 1.2f);
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.35f : 0.07f));
        g.drawLine (cell.getX() + 7, cell.getY() + 1.5f, cell.getRight() - 7, cell.getY() + 1.5f, 1.0f);
        const auto textCol = lit ? kBg : (exists ? c : kMuted.withAlpha (0.4f));
        g.setColour (textCol);
        g.setFont (mono (12.0f, true));
        const juce::String main = pitch ? (pad == 0 ? juce::String ("ROOT") : "+" + juce::String (pad)) : juce::String (si + 1).paddedLeft ('0', 2);
        g.drawText (main, cell.reduced (7, 5), juce::Justification::topLeft);
        g.setFont (mono (9.0f)); g.setColour (textCol.withAlpha (0.75f));
        g.drawText (noteName (note), cell.reduced (7, 5), juce::Justification::topRight);
    }
}

void PadGrid::mouseDown (const juce::MouseEvent& e)
{
    const int pad = padAt (e.position);
    const bool pitch = proc.apvts.getRawParameterValue ("layout")->load() > 0.5f;
    if (! pitch)
    {
        const int si = bank * 16 + pad;
        if (si >= proc.engine.numSlices.load()) return;
        proc.engine.selected = si;
        if (onSelect) onSelect();
    }
    const float h = (float) getHeight() / 4;
    const float yIn = std::fmod (e.position.y, h) / h;             // top of the pad = harder hit
    downNote = noteFor (pad);
    proc.keyboardState.noteOn (1, downNote, juce::jlimit (0.3f, 1.0f, 1.05f - 0.6f * yIn));
    repaint();
}

void PadGrid::mouseUp (const juce::MouseEvent&)
{
    if (downNote >= 0) proc.keyboardState.noteOff (1, downNote, 0.0f);
    downNote = -1;
    repaint();
}

//==============================================================================
juce::Slider& ChopEditor::knob (const char* id, juce::Colour col, const juce::String& label)
{
    auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 15);
    s->setColour (juce::Slider::rotarySliderFillColourId, col);
    s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (*s);
    sAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *s));
    for (auto& d : paramDefs()) if (juce::String (d.id) == id) s->setDoubleClickReturnValue (true, d.def);
    auto l = std::make_unique<juce::Label> (juce::String(), label);
    l->setJustificationType (juce::Justification::centred); l->setFont (mono (10.0f, true)); l->setColour (juce::Label::textColourId, kMuted);
    addAndMakeVisible (*l);
    knobLabels[s.get()] = l.get();
    labels.push_back (std::move (l));
    sliders.push_back (std::move (s));
    return *sliders.back();
}

juce::Slider& ChopEditor::sliceKnob (int k, juce::Colour col, const juce::String& label)
{
    auto s = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow);
    s->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 15);
    s->setColour (juce::Slider::rotarySliderFillColourId, col);
    s->setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    s->setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    const auto& info = sliceInfo (k);
    s->setRange (info.min, info.max, k == S_PITCH ? 1.0 : 0.0);
    if (k == S_FILTER) s->setSkewFactorFromMidPoint (2000.0);
    if (k == S_ATTACK || k == S_RELEASE) s->setSkewFactorFromMidPoint (k == S_ATTACK ? 40.0 : 250.0);
    if (k == S_START) s->setSkewFactorFromMidPoint (30.0);
    if (k == S_STRETCH) s->setSkewFactorFromMidPoint (1.0);
    s->setDoubleClickReturnValue (true, info.def);
    s->textFromValueFunction = [k] (double v) { return sliceText (k, v); };
    s->valueFromTextFunction = [] (const juce::String& t) { return t.retainCharacters ("-0123456789.").getDoubleValue(); };
    s->onValueChange = [this, k, sp = s.get()]
    {
        const int sel = proc.engine.selected.load();
        if (juce::isPositiveAndBelow (sel, kMaxSlices)) proc.engine.slice[(size_t) sel].v[(size_t) k] = (float) sp->getValue();
    };
    addAndMakeVisible (*s);
    auto l = std::make_unique<juce::Label> (juce::String(), label);
    l->setJustificationType (juce::Justification::centred); l->setFont (mono (10.0f, true)); l->setColour (juce::Label::textColourId, kMuted);
    addAndMakeVisible (*l);
    knobLabels[s.get()] = l.get();
    labels.push_back (std::move (l));
    sliceKnobs[(size_t) k] = s.get();
    sliders.push_back (std::move (s));
    return *sliders.back();
}

Chip& ChopEditor::chip (const juce::String& text, juce::Colour col, const char* boolParam)
{
    auto c = std::make_unique<Chip> (text, col);
    addAndMakeVisible (*c);
    if (boolParam) bAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, boolParam, *c));
    chips.push_back (std::move (c));
    return *chips.back();
}

juce::Slider& ChopEditor::barSlider (const char* id)
{
    auto s = std::make_unique<juce::Slider> (juce::Slider::LinearBar, juce::Slider::NoTextBox);
    s->setColour (juce::Slider::trackColourId, flux (2));
    addAndMakeVisible (*s);
    sAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *s));
    sliders.push_back (std::move (s));
    return *sliders.back();
}

ChopEditor::ChopEditor (ChopProcessor& p)
    : AudioProcessorEditor (&p), proc (p), wave (p), overview (p, wave), pads (p)
{
    setLookAndFeel (&lnf);
    auto param = [this] (const char* id) -> juce::RangedAudioParameter& { return *proc.apvts.getParameter (id); };
    auto group = [this] (ChoiceChips* g) { addAndMakeVisible (*g); groups.emplace_back (g); return g; };

    // ---- sample toolbar
    chopModes = group (new ChoiceChips (param ("chopmode"), { "TRANSIENT", "BEAT", "EQUAL", "MANUAL" }, flux (2)));
    chopModes->widths = { 80, 48, 54, 62 };
    sensBar = &barSlider ("sens");
    sensBar->textFromValueFunction = [] (double v) { return "SENS " + juce::String (juce::roundToInt (v * 100)) + "%"; };
    slicesBar = &barSlider ("slices");
    slicesBar->textFromValueFunction = [] (double v) { return "SLICES " + juce::String (juce::roundToInt (v)); };
    sensBar->updateText(); slicesBar->updateText();
    for (auto* c : paramDefs()[DIVISION].choices) divisionBox.addItem (juce::String ("DIV ") + c, divisionBox.getNumItems() + 1);
    addAndMakeVisible (divisionBox);
    cAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "division", divisionBox));
    findBtn = &chip ("FIND", kInk); findBtn->setClickingTogglesState (false); findBtn->action = true;
    findBtn->onClick = [this] { proc.find(); };
    findBtn->setTooltip ("Drop cue points on the strongest hits across the whole sample");
    randomBtn = &chip ("RANDOM", kInk); randomBtn->setClickingTogglesState (false); randomBtn->action = true;
    randomBtn->onClick = [this] { proc.randomise(); };

    bpmLabel.setEditable (false, true);
    bpmLabel.setFont (mono (13.0f, true)); bpmLabel.setJustificationType (juce::Justification::centredRight);
    bpmLabel.setColour (juce::Label::textColourId, kInk);
    bpmLabel.setColour (juce::Label::backgroundWhenEditingColourId, kBg);
    bpmLabel.setTooltip ("Detected tempo. Double-click to type the right one.");
    bpmLabel.onTextChange = [this] { const double b = bpmLabel.getText().getDoubleValue(); if (b > 20) proc.setDetectedBpm (b); };
    addAndMakeVisible (bpmLabel);
    bpmSync = &chip ("SYNC", flux (3), "bpmsync");
    for (auto* k : keyNames24()) keyBox.addItem (k, keyBox.getNumItems() + 1);
    keyBox.setTooltip ("Detected key. Change it if the detection is off.");
    keyBox.onChange = [this] { const int i = keyBox.getSelectedId() - 1; if (i >= 0) proc.setDetectedKey (i % 12, i >= 12); };
    addAndMakeVisible (keyBox);
    keySync = &chip ("SYNC", flux (3), "keysync");
    for (auto* k : keyNames24()) targetBox.addItem (juce::String ("TO ") + k, targetBox.getNumItems() + 1);
    targetBox.setTooltip ("Your song's key. Ableton doesn't tell plugins the key, so set it here.");
    addAndMakeVisible (targetBox);
    cAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "targetkey", targetBox));
    warpBtn = &chip ("WARP", flux (5), "warp");
    warpBtn->setTooltip ("On: pitch changes keep the tempo (Serato style). Off: pitch changes speed too (classic MPC/SP style).");

    addAndMakeVisible (wave);
    addAndMakeVisible (overview);
    wave.onSelect = [this] { refreshSlice(); pads.repaint(); overview.repaint(); };
    wave.onViewChanged = [this] { overview.repaint(); };

    // ---- pads
    layoutChips = group (new ChoiceChips (param ("layout"), { "SLICES", "PITCH" }, flux (2)));
    bankA = &chip ("A", kInk); bankB = &chip ("B", kInk);
    bankA->setRadioGroupId (77); bankB->setRadioGroupId (77);
    bankA->setToggleState (true, juce::dontSendNotification);
    bankA->onClick = [this] { pads.bank = 0; pads.repaint(); };
    bankB->onClick = [this] { pads.bank = 1; pads.repaint(); };
    addAndMakeVisible (pads);
    pads.onSelect = [this] { refreshSlice(); wave.repaint(); };

    // ---- slice editor
    const auto so = flux (5);
    sliceKnob (S_PITCH, so, "PITCH"); sliceKnob (S_FINE, so, "FINE"); sliceKnob (S_STRETCH, so, "STRETCH"); sliceKnob (S_LEVEL, so, "LEVEL"); sliceKnob (S_PAN, so, "PAN");
    sliceKnob (S_START, so, "START"); sliceKnob (S_LENGTH, so, "LENGTH"); sliceKnob (S_ATTACK, so, "ATTACK"); sliceKnob (S_RELEASE, so, "RELEASE"); sliceKnob (S_FILTER, so, "FILTER");
    auto sliceToggle = [this, so] (const juce::String& t, int k)
    {
        auto& c = chip (t, so);
        c.onClick = [this, k, cp = &c] { const int sel = proc.engine.selected.load(); proc.engine.slice[(size_t) sel].v[(size_t) k] = cp->getToggleState() ? 1.0f : 0.0f; };
        return &c;
    };
    revBtn = sliceToggle ("REVERSE", S_REVERSE);
    oneBtn = sliceToggle ("ONE-SHOT", S_ONESHOT);
    loopBtn = sliceToggle ("LOOP", S_LOOP);
    oneBtn->setTooltip ("On: the chop plays to the end. Off: it stops when you let go.");
    for (int i = 0; i < 4; ++i)
    {
        auto& c = chip (i < 3 ? juce::String (i + 1) : juce::String ("OFF"), so);
        c.setClickingTogglesState (false);
        const int val = i < 3 ? i + 1 : 0;
        c.onClick = [this, val] { proc.engine.slice[(size_t) proc.engine.selected.load()].v[S_CHOKE] = (float) val; refreshSlice(); };
        chokeBtns[(size_t) i] = &c;
    }
    copyBtn = &chip ("COPY", kInk); copyBtn->setClickingTogglesState (false); copyBtn->action = true;
    copyBtn->onClick = [this] { const auto& s = proc.engine.slice[(size_t) proc.engine.selected.load()]; for (int k = 0; k < NUM_SP; ++k) clipboard[(size_t) k] = s[k]; hasClip = true; };
    pasteBtn = &chip ("PASTE", kInk); pasteBtn->setClickingTogglesState (false); pasteBtn->action = true;
    pasteBtn->onClick = [this] { if (! hasClip) return; auto& s = proc.engine.slice[(size_t) proc.engine.selected.load()]; for (int k = 0; k < NUM_SP; ++k) s.v[(size_t) k] = clipboard[(size_t) k]; refreshSlice(); };
    midiBtn = std::make_unique<MidiDragChip> (proc);
    midiBtn->setButtonText ("MIDI");
    midiBtn->setTooltip ("Drag onto a MIDI track: your chops in their original order");
    addAndMakeVisible (*midiBtn);

    // ---- play
    voiceChips = group (new ChoiceChips (param ("voice"), { "MONO", "POLY", "GATE" }, flux (3)));
    repeatBtn = &chip ("ON", flux (3), "repeat");
    rateChips = group (new ChoiceChips (param ("reprate"), { "1/8", "1/16", "1/32" }, flux (3), { 1, 2, 3 }));
    tripBtn = &chip ("T", flux (3), "triplet");
    knob ("glide", flux (3), "GLIDE"); knob ("swing", flux (3), "SWING"); knob ("vel", flux (3), "VEL");

    // ---- color
    knob ("cutoff", flux (4), "CUTOFF"); knob ("res", flux (4), "RES"); knob ("bits", flux (4), "BITS"); knob ("rate", flux (4), "RATE");
    ftypeChips = group (new ChoiceChips (param ("ftype"), { "LP", "HP", "BP" }, flux (4)));
    knob ("drive", flux (4), "DRIVE");

    // ---- programs
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i).toUpperCase(), i + 1);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::black);
    presetBox.setColour (juce::ComboBox::textColourId, flux (2));
    presetBox.setColour (juce::ComboBox::arrowColourId, flux (2));
    presetBox.onChange = [this] { const int i = presetBox.getSelectedId() - 1; if (i >= 0 && i != proc.getCurrentProgram()) { proc.setCurrentProgram (i); shownProgram = i; } };
    prevBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + proc.getNumPrograms() - 1) % proc.getNumPrograms()); };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (presetBox); addAndMakeVisible (prevBtn); addAndMakeVisible (nextBtn);
#ifdef AE8_FLIP
    for (auto* m : paramDefs()[FLIP_MODE].choices) flipBox.addItem (juce::String (m).toUpperCase(), flipBox.getNumItems() + 1);
    flipBox.setColour (juce::ComboBox::textColourId, flux (5));
    flipBox.setColour (juce::ComboBox::arrowColourId, flux (5));
    flipBox.setTooltip ("Rearranges which slice each pad plays. Your sound settings stay as they are.");
    addAndMakeVisible (flipBox);
    cAtts.push_back (std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "flipmode", flipBox));
#endif

    scaler.init (*this, kW, kH);
    refreshSlice();
    startTimerHz (30);
#ifdef AE8_CHOP_DEMO
    if (! proc.currentSample())
    {
        extern std::shared_ptr<ae8chop::Sample> ae8chopDemoSample();
        proc.setSampleData (ae8chopDemoSample(), "SOULFUL_KEYS_92.WAV", {});
        proc.engine.selected = 6;
        refreshSlice();
    }
#endif
}

ChopEditor::~ChopEditor() { setLookAndFeel (nullptr); }

void ChopEditor::refreshSlice()
{
    const int sel = juce::jlimit (0, kMaxSlices - 1, proc.engine.selected.load());
    const auto& s = proc.engine.slice[(size_t) sel];
    for (int k = 0; k < NUM_SP; ++k)
        if (auto* kn = sliceKnobs[(size_t) k]) { kn->setValue (s[k], juce::dontSendNotification); kn->updateText(); }
    revBtn->setToggleState (s[S_REVERSE] > 0.5f, juce::dontSendNotification);
    oneBtn->setToggleState (s[S_ONESHOT] > 0.5f, juce::dontSendNotification);
    loopBtn->setToggleState (s[S_LOOP] > 0.5f, juce::dontSendNotification);
    const int ch = (int) s[S_CHOKE];
    for (int i = 0; i < 4; ++i) chokeBtns[(size_t) i]->setToggleState ((i < 3 ? i + 1 : 0) == ch, juce::dontSendNotification);
    keyBox.setSelectedId (proc.keyRoot + (proc.keyMinor ? 12 : 0) + 1, juce::dontSendNotification);
    bpmLabel.setText (juce::String (proc.detectedBpm, 1), juce::dontSendNotification);
    shownSelected = sel;
    repaint (scaler.toWindow (*this, sliceArea.withHeight (22)));
}

juce::String ChopEditor::keyShiftText() const
{
    const int s = proc.keyShift();
    return (s > 0 ? "+" : "") + juce::String (s) + " ST";
}

void ChopEditor::timerCallback()
{
    if (proc.engine.selected.load() != shownSelected) { refreshSlice(); pads.repaint(); }
    if (proc.changeCounter.load() != shownChange) { shownChange = proc.changeCounter.load(); refreshSlice(); wave.repaint(); overview.repaint(); pads.repaint(); repaint (scaler.toWindow (*this, sampleArea.withHeight (22))); }
    if (proc.getCurrentProgram() != shownProgram) { shownProgram = proc.getCurrentProgram(); presetBox.setSelectedId (shownProgram + 1, juce::dontSendNotification); }
    const int mode = (int) proc.apvts.getRawParameterValue ("chopmode")->load();
    sensBar->setVisible (mode == CHOP_TRANSIENT);
    divisionBox.setVisible (mode == CHOP_BEAT);
    static uint32_t lastPlaying = 0;
    const uint32_t pl = proc.engine.guiPlaying.load();
    if (pl != 0 || lastPlaying != 0) { wave.repaint(); pads.repaint(); }
    lastPlaying = pl;
    repaint (scaler.toWindow (*this, toolbar.withLeft (toolbar.getRight() - 470).withWidth (60)));
}

void ChopEditor::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (scaler.scale (*this)));
    drawWindow (g, scaler.design());
#ifdef AE8_FLIP
    drawHeader (g, headerArea, "FLIP", "F-01 / SAMPLE REARRANGEMENT");
#else
    drawHeader (g, headerArea, "CHOP", "C-04 / SAMPLER");
#endif
    g.setFont (tracked (mono (9.5f, true), 0.12f)); g.setColour (kMuted);
    g.drawText ("PROGRAM", presetBox.getBounds().translated (-80, 0).withWidth (72), juce::Justification::centredRight);
#ifdef AE8_FLIP
    g.drawText ("FLIP", flipBox.getBounds().translated (-52, 0).withWidth (44), juce::Justification::centredRight);
#endif

    auto s = proc.currentSample();
    const juce::String tag = s ? proc.sampleName.toUpperCase() + "  |  " + juce::String ((double) s->size() / s->sr, 1) + " S" : juce::String ("NO SAMPLE");
    drawSection (g, sampleArea, "SAMPLE", tag, flux (2), false);
    drawSection (g, padArea, "PADS", "4 X 4  |  BANK A / B", flux (3), false);
    drawSection (g, sliceArea, "SLICE " + juce::String (proc.engine.selected.load() + 1).paddedLeft ('0', 2), "PER-PAD", flux (5), false);
    drawSection (g, playArea, "PLAY", "VOICE / REPEAT", flux (1), true);
    drawSection (g, colorArea, "COLOR", "FILTER / CRUSH", flux (4), false);

    g.setFont (mono (10.0f, true)); g.setColour (kMuted);
    auto lbl = [&] (juce::Component& c, const juce::String& t, int w) { g.drawText (t, c.getX() - w - 4, c.getY(), w, c.getHeight(), juce::Justification::centredRight); };
    lbl (*chopModes, "CHOP", 34);
    lbl (bpmLabel, "BPM", 26);
    lbl (keyBox, "KEY", 26);
    lbl (*chokeBtns[0], "CHOKE", 44);
    g.drawText ("VOICE", playArea.getX() + 12, voiceChips->getY() - 16, 80, 14, juce::Justification::centredLeft);
    g.drawText ("REPEAT", playArea.getX() + 12, repeatBtn->getY(), 60, repeatBtn->getHeight(), juce::Justification::centredLeft);
    // key shift readout
    g.setColour (proc.apvts.getRawParameterValue ("keysync")->load() > 0.5f ? flux (3) : kMuted);
    g.setFont (mono (11.0f, true));
    g.drawText (keyShiftText(), targetBox.getRight() + 6, targetBox.getY(), 50, targetBox.getHeight(), juce::Justification::centredLeft);
    // hint under the waveform
    g.setColour (kMuted.withAlpha (0.8f)); g.setFont (mono (9.5f));
    g.drawText ("DROP A FILE  |  CLICK = PLAY  |  DBL-CLICK = ADD SLICE  |  DRAG FLAG = MOVE  |  RIGHT-CLICK FLAG = DELETE  |  WHEEL = ZOOM",
                overview.getRight() - 800, overview.getBottom() + 3, 800, 12, juce::Justification::centredRight);
}

void ChopEditor::placeKnob (juce::Slider& s, juce::Rectangle<int> cell)
{
    cell.removeFromTop (14);
    auto k = cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 74), juce::jmin (cell.getHeight(), 88));
    if (auto* l = knobLabels[&s]) l->setBounds (k.getX() - 10, k.getY() - 14, k.getWidth() + 20, 14);
    s.setBounds (k);
}

void ChopEditor::resized()
{
    auto area = scaler.design().reduced (kGap, 0);
    area.removeFromTop (kGap);
    headerArea = area.removeFromTop (kHeader);
    area.removeFromTop (kGap);
    {
        auto right = headerArea.reduced (14, 15);
        nextBtn.setBounds (right.removeFromRight (34)); right.removeFromRight (6);
        prevBtn.setBounds (right.removeFromRight (34)); right.removeFromRight (8);
        presetBox.setBounds (right.removeFromRight (280));
#ifdef AE8_FLIP
        right.removeFromRight (84);
        flipBox.setBounds (right.removeFromRight (190));
#endif
    }

    // ---- sample
    sampleArea = area.removeFromTop (kSampleH);
    area.removeFromTop (kGap);
    {
        auto in = sampleArea.withTrimmedTop (20).reduced (10, 0);
        toolbar = in.removeFromTop (38).withTrimmedTop (8).withTrimmedBottom (6);
        auto t = toolbar;
        t.removeFromLeft (38);
        chopModes->setBounds (t.removeFromLeft (259)); t.removeFromLeft (10);
        auto ctx = t.removeFromLeft (96);
        sensBar->setBounds (ctx); divisionBox.setBounds (ctx);
        t.removeFromLeft (6);
        slicesBar->setBounds (t.removeFromLeft (84)); t.removeFromLeft (10);
        findBtn->setBounds (t.removeFromLeft (48)); t.removeFromLeft (5);
        randomBtn->setBounds (t.removeFromLeft (62));
        auto r = t;
        warpBtn->setBounds (r.removeFromRight (50)); r.removeFromRight (56);
        targetBox.setBounds (r.removeFromRight (104)); r.removeFromRight (5);
        keySync->setBounds (r.removeFromRight (46)); r.removeFromRight (5);
        keyBox.setBounds (r.removeFromRight (84)); r.removeFromRight (34);
        bpmSync->setBounds (r.removeFromRight (46)); r.removeFromRight (4);
        bpmLabel.setBounds (r.removeFromRight (52));

        in.removeFromBottom (18);
        overview.setBounds (in.removeFromBottom (18).reduced (2, 0));
        in.removeFromBottom (4);
        wave.setBounds (in);
    }

    // ---- bottom row
    auto row = area.withTrimmedBottom (kGap);
    padArea = row.removeFromLeft (kSideW); row.removeFromLeft (kGap);
    auto right = row.removeFromRight (kSideW); row.removeFromRight (kGap);
    sliceArea = row;
    playArea = right.removeFromTop ((right.getHeight() - kGap) / 2); right.removeFromTop (kGap);
    colorArea = right;

    {
        auto in = padArea.withTrimmedTop (20).reduced (10, 8);
        auto top = in.removeFromTop (24);
        layoutChips->setBounds (top.removeFromLeft (128));
        bankB->setBounds (top.removeFromRight (26)); top.removeFromRight (5);
        bankA->setBounds (top.removeFromRight (26));
        in.removeFromTop (6);
        pads.setBounds (in);
    }
    {
        auto in = sliceArea.withTrimmedTop (20).reduced (10, 8);
        auto bottom = in.removeFromBottom (26);
        in.removeFromBottom (6);
        const int rowH = in.getHeight() / 2, cw = in.getWidth() / 5;
        const int order[10] = { S_PITCH, S_FINE, S_STRETCH, S_LEVEL, S_PAN, S_START, S_LENGTH, S_ATTACK, S_RELEASE, S_FILTER };
        for (int i = 0; i < 10; ++i)
            placeKnob (*sliceKnobs[(size_t) order[i]], juce::Rectangle<int> (in.getX() + cw * (i % 5), in.getY() + rowH * (i / 5) + 6, cw, rowH - 8));
        revBtn->setBounds (bottom.removeFromLeft (64)); bottom.removeFromLeft (5);
        oneBtn->setBounds (bottom.removeFromLeft (70)); bottom.removeFromLeft (5);
        loopBtn->setBounds (bottom.removeFromLeft (46)); bottom.removeFromLeft (50);
        for (int i = 0; i < 4; ++i) { chokeBtns[(size_t) i]->setBounds (bottom.removeFromLeft (i < 3 ? 22 : 32)); bottom.removeFromLeft (4); }
        midiBtn->setBounds (bottom.removeFromRight (56)); bottom.removeFromRight (5);
        pasteBtn->setBounds (bottom.removeFromRight (54)); bottom.removeFromRight (5);
        copyBtn->setBounds (bottom.removeFromRight (50));
    }
    {
        auto in = playArea.withTrimmedTop (20).reduced (10, 6);
        in.removeFromTop (16);
        voiceChips->setBounds (in.removeFromTop (24).withWidth (170));
        in.removeFromTop (8);
        auto rr = in.removeFromTop (24);
        rr.removeFromLeft (56);
        repeatBtn->setBounds (rr.removeFromLeft (36)); rr.removeFromLeft (5);
        rateChips->setBounds (rr.removeFromLeft (128)); rr.removeFromLeft (5);
        tripBtn->setBounds (rr.removeFromLeft (26));
        in.removeFromTop (4);
        const int cw = in.getWidth() / 3;
        int idx = 0;
        for (auto& s : sliders)
        {
            const auto lt = knobLabels.count (s.get()) ? knobLabels[s.get()]->getText() : juce::String();
            if (lt == "GLIDE" || lt == "SWING" || lt == "VEL") placeKnob (*s, juce::Rectangle<int> (in.getX() + cw * idx++, in.getY(), cw, in.getHeight()));
        }
    }
    {
        auto in = colorArea.withTrimmedTop (20).reduced (10, 6);
        auto bottom = in.removeFromBottom (40);
        const int cw = in.getWidth() / 4;
        int idx = 0;
        juce::Slider* drive = nullptr;
        for (auto& s : sliders)
        {
            const auto lt = knobLabels.count (s.get()) ? knobLabels[s.get()]->getText() : juce::String();
            if (lt == "CUTOFF" || lt == "RES" || lt == "BITS" || lt == "RATE") placeKnob (*s, juce::Rectangle<int> (in.getX() + cw * idx++, in.getY() + 2, cw, in.getHeight() - 2));
            if (lt == "DRIVE") drive = s.get();
        }
        ftypeChips->setBounds (bottom.removeFromLeft (110).withSizeKeepingCentre (110, 24));
        if (drive)
        {
            auto dcell = bottom.removeFromRight (140);
            knobLabels[drive]->setBounds (dcell.removeFromLeft (44));
            dcell = dcell.withSizeKeepingCentre (dcell.getWidth(), 40);
            drive->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            drive->setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 15);
            drive->setBounds (dcell);
        }
    }
    scaler.apply (*this);
}

#ifdef AE8_CHOP_DEMO
#include "tests/testloop.h"
std::shared_ptr<ae8chop::Sample> ae8chopDemoSample()
{
    auto s = std::make_shared<ae8chop::Sample>(); s->sr = 44100; s->L = makeLoop (44100); s->R = s->L;
    return s;
}
#endif
