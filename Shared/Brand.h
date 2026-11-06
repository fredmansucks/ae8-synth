#pragma once
// AE-8 shared brand kit for plugin UIs (v2, "premium"): palette, embedded fonts, machined knobs,
// recessed glass displays, section plates, header plate, and a whole-window scaler.
#include <JuceHeader.h>
#include "AE8Fonts.h"
#include "../Source/BrandPaths.h"

namespace ae8brand
{
// ---- palette ----------------------------------------------------------------------------------
inline const juce::Colour kBg      { 0xff050608 };
inline const juce::Colour kPanel   { 0xff111319 };
inline const juce::Colour kRaised  { 0xff1a1d25 };
inline const juce::Colour kLine    { 0xff2a2e38 };
inline const juce::Colour kInk     { 0xfff2f2ee };
inline const juce::Colour kMuted   { 0xff9b9fa8 };
inline const juce::Colour kValue   { 0xffd5d7dc };   // value read-outs
inline const juce::Colour kEdge    { 0xff20232b };   // panel outlines
inline const juce::Colour kGlass   { 0xff020304 };   // display glass
inline juce::Colour flux (int i)
{
    static const juce::uint32 c[7] = { 0xff252466, 0xff2b46c8, 0xff4fd0e0, 0xff6fbd46, 0xfff7ec1e, 0xfff5901e, 0xffed1c22 };
    return juce::Colour (c[juce::jlimit (0, 6, i)]);
}

// ---- fonts (Archivo + IBM Plex Mono, both SIL OFL, embedded) ---------------------------------
struct Fonts
{
    juce::Typeface::Ptr sansHeavy, sansSemi, mono, monoSemi;
    static const Fonts& get()
    {
        static const Fonts f;
        return f;
    }
private:
    Fonts()
    {
        sansHeavy = juce::Typeface::createSystemTypefaceFor (AE8Fonts::ArchivoExtraBold_ttf, AE8Fonts::ArchivoExtraBold_ttfSize);
        sansSemi  = juce::Typeface::createSystemTypefaceFor (AE8Fonts::ArchivoSemiBold_ttf, AE8Fonts::ArchivoSemiBold_ttfSize);
        mono      = juce::Typeface::createSystemTypefaceFor (AE8Fonts::IBMPlexMonoRegular_ttf, AE8Fonts::IBMPlexMonoRegular_ttfSize);
        monoSemi  = juce::Typeface::createSystemTypefaceFor (AE8Fonts::IBMPlexMonoSemiBold_ttf, AE8Fonts::IBMPlexMonoSemiBold_ttfSize);
    }
};

inline juce::Font fontFrom (juce::Typeface::Ptr t, float h)
{
    return juce::Font (juce::FontOptions (t).withHeight (h));
}
// Heavy display face for product names; semibold for section titles.
inline juce::Font sansBold (float h) { return fontFrom (h >= 16.0f ? Fonts::get().sansHeavy : Fonts::get().sansSemi, h); }
inline juce::Font mono (float h, bool bold = false) { return fontFrom (bold ? Fonts::get().monoSemi : Fonts::get().mono, h); }
inline juce::Font tracked (juce::Font f, float k = 0.08f) { return f.withExtraKerningFactor (k); }

// ---- belt mark -----------------------------------------------------------------------------------
inline juce::Path fieldBand (float Lc, float w, float latDeg, float side, juce::Point<float> c, float R)
{
    juce::Path p;
    const float lm = juce::degreesToRadians (latDeg);
    auto pt = [&] (float L, float la) { const float r = L * std::cos (la) * std::cos (la); return juce::Point<float> (c.x + side * r * std::cos (la) * R, c.y - r * std::sin (la) * R); };
    for (int i = 0; i <= 40; ++i) { const float la = -lm + 2 * lm * (float) i / 40.0f; auto q = pt (Lc + w, la); if (i == 0) p.startNewSubPath (q); else p.lineTo (q); }
    for (int i = 40; i >= 0; --i) { const float la = -lm + 2 * lm * (float) i / 40.0f; p.lineTo (pt (Lc - w, la)); }
    p.closeSubPath();
    return p;
}

inline void drawBeltMark (juce::Graphics& g, juce::Rectangle<float> a)
{
    const auto c = a.getCentre();
    const float R = a.getHeight() * 0.085f;
    const float outer[][3] = { { 1.35f, 46, 1 }, { 1.1f, 40, 2 }, { 0.85f, 34, 3 }, { 0.62f, 28, 4 }, { 0.4f, 22, 5 }, { 0.2f, 16, 6 } };
    const float inner[][3] = { { 0.32f, 36, 2 }, { 0.2f, 28, 4 }, { 0.09f, 20, 6 } };
    for (float side : { 1.0f, -1.0f })
    {
        for (auto& b : outer) { g.setColour (flux ((int) b[2])); g.fillPath (fieldBand (4.6f, b[0], b[1], side, c, R)); }
        for (auto& b : inner) { g.setColour (flux ((int) b[2])); g.fillPath (fieldBand (1.75f, b[0], b[1], side, c, R)); }
    }
    g.setColour (kInk); g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);
}

// ---- surfaces ------------------------------------------------------------------------------------
inline const juce::Image& grain()
{
    static const juce::Image img = []
    {
        juce::Image im (juce::Image::ARGB, 192, 192, true);
        juce::Random rnd (8);
        for (int y = 0; y < im.getHeight(); ++y)
            for (int x = 0; x < im.getWidth(); ++x)
            {
                const float v = rnd.nextFloat();
                im.setPixelAt (x, y, v > 0.5f ? juce::Colours::white.withAlpha ((v - 0.5f) * 0.05f) : juce::Colours::black.withAlpha ((0.5f - v) * 0.12f));
            }
        return im;
    }();
    return img;
}

inline void fillGrain (juce::Graphics& g, juce::Rectangle<float> r, float alpha = 1.0f)
{
    juce::Graphics::ScopedSaveState ss (g);
    g.setOpacity (alpha);
    g.setFillType (juce::FillType (grain(), juce::AffineTransform()));
    g.fillRect (r);
}

// Window: deep anodised black with grain and a soft top light.
inline void drawWindow (juce::Graphics& g, juce::Rectangle<int> bounds)
{
    auto b = bounds.toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0a0b0f), b.getX(), b.getY(), juce::Colour (0xff030405), b.getX(), b.getBottom(), false));
    g.fillRect (b);
    juce::ColourGradient light (juce::Colours::white.withAlpha (0.035f), b.getCentreX(), b.getY() - b.getHeight() * 0.2f,
                                juce::Colours::transparentWhite, b.getCentreX(), b.getY() + b.getHeight() * 0.7f, true);
    g.setGradientFill (light); g.fillRect (b);
    fillGrain (g, b);
}

// Raised plate with bevel: top highlight, crisp outline, soft drop shadow.
inline void drawPlate (juce::Graphics& g, juce::Rectangle<float> b, float radius, juce::Colour top, juce::Colour bottom)
{
    for (int i = 1; i <= 4; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillRoundedRectangle (b.translated (0, (float) i * 0.8f).expanded ((float) i * 0.4f), radius + (float) i * 0.4f);
    }
    g.setGradientFill (juce::ColourGradient (top, b.getX(), b.getY(), bottom, b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, radius);
    fillGrain (g, b.reduced (1.0f), 0.8f);
    g.setColour (kEdge); g.drawRoundedRectangle (b.reduced (0.5f), radius, 1.0f);
    juce::Path hl; hl.addRoundedRectangle (b.getX() + 1.0f, b.getY() + 1.0f, b.getWidth() - 2.0f, b.getHeight() - 2.0f, radius - 0.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.09f), b.getX(), b.getY(), juce::Colours::transparentWhite, b.getX(), b.getY() + 14.0f, false));
    g.strokePath (hl, juce::PathStrokeType (1.0f));
}

// Recessed glass display: inner shadow, faint reflection, lit lower edge.
inline void drawGlass (juce::Graphics& g, juce::Rectangle<float> r, float radius = 6.0f)
{
    g.setColour (kGlass); g.fillRoundedRectangle (r, radius);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (r, radius);
        g.reduceClipRegion (clip);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.85f), r.getX(), r.getY(), juce::Colours::transparentBlack, r.getX(), r.getY() + 10.0f, false));
        g.fillRect (r.withHeight (10.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.035f), r.getX(), r.getY(), juce::Colours::transparentWhite, r.getX(), r.getCentreY(), false));
        g.fillRect (r.withHeight (r.getHeight() * 0.5f));
    }
    g.setColour (juce::Colours::black); g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentWhite, r.getX(), r.getCentreY(), juce::Colours::white.withAlpha (0.08f), r.getX(), r.getBottom(), false));
    g.drawRoundedRectangle (r.expanded (0.5f), radius + 0.5f, 1.0f);
}

inline void drawLed (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, bool on)
{
    if (on)
    {
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.45f), c.x, c.y, col.withAlpha (0.0f), c.x + r * 3.2f, c.y, true));
        g.fillEllipse (c.x - r * 3.2f, c.y - r * 3.2f, r * 6.4f, r * 6.4f);
        g.setColour (col); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - r * 0.45f, c.y - r * 0.55f, r * 0.7f, r * 0.6f);
    }
    else
    {
        g.setColour (juce::Colour (0xff15171c)); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour (juce::Colours::black); g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
    }
}

// Text with a soft halo, for lit display read-outs.
inline void drawGlowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour col)
{
    g.setColour (col.withAlpha (0.10f));
    for (auto d : { juce::Point<float> (-1.2f, 0), { 1.2f, 0 }, { 0, -1.2f }, { 0, 1.2f } }) g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (col); g.drawText (t, r, j, false);
}

// ---- header plate --------------------------------------------------------------------------------
inline void drawHeader (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& product, const juce::String& code)
{
    auto hb = area.toFloat();
    drawPlate (g, hb, 8.0f, juce::Colour (0xff181a21), juce::Colour (0xff0c0d11));
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addRoundedRectangle (hb.reduced (1.0f), 8.0f);
        g.reduceClipRegion (clip);
        // brushed finish
        for (float y = hb.getY() + 2; y < hb.getBottom(); y += 2.0f)
        {
            g.setColour (juce::Colours::white.withAlpha (std::fmod (y * 7.31f, 1.0f) * 0.012f));
            g.drawHorizontalLine ((int) y, hb.getX(), hb.getRight());
        }
        // faint field lines behind the plate
        const juce::Point<float> c (hb.getCentreX() + 60, hb.getCentreY() + 4);
        g.setColour (kInk.withAlpha (0.045f));
        for (float L : { 3.0f, 5.0f, 8.0f, 13.0f, 21.0f, 34.0f, 55.0f, 90.0f })
            for (float side : { 1.0f, -1.0f })
            {
                juce::Path p;
                const float lm = std::acos (std::sqrt (1.0f / L));
                for (int i = 0; i <= 120; ++i)
                {
                    const float la = -lm + 2 * lm * (float) i / 120.0f, r = L * std::cos (la) * std::cos (la);
                    const juce::Point<float> q (c.x + side * r * std::cos (la) * 9.0f, c.y - r * std::sin (la) * 9.0f);
                    if (i == 0) p.startNewSubPath (q); else p.lineTo (q);
                }
                g.strokePath (p, juce::PathStrokeType (0.8f));
            }
    }
    // flux hairline with a glow
    {
        auto bar = hb.reduced (16.0f, 0).removeFromBottom (2.0f).translated (0, -4);
        const float w = bar.getWidth() / 7.0f;
        for (int i = 0; i < 7; ++i)
        {
            const auto seg = juce::Rectangle<float> (bar.getX() + w * (float) i, bar.getY(), w + 0.5f, bar.getHeight());
            g.setColour (flux (i).withAlpha (0.22f)); g.fillRect (seg.expanded (0, 2.0f));
            g.setColour (flux (i)); g.fillRect (seg);
        }
    }
    auto h = area.reduced (16, 0);
    drawBeltMark (g, h.removeFromLeft (64).withSizeKeepingCentre (64, 64).toFloat().translated (0, -2));
    h.removeFromLeft (10);
    auto word = h.removeFromLeft (82).withSizeKeepingCentre (82, 28).translated (0, -2);
    static const juce::Path wm = juce::Drawable::parseSVGPath (ae8::kWordmarkPath);
    g.setColour (kInk);
    g.fillPath (wm, wm.getTransformToScaleToFit (word.toFloat(), true, juce::Justification::centredLeft));
    // thin divider between brand and product
    g.setColour (kInk.withAlpha (0.18f));
    g.fillRect ((float) word.getRight() + 14, (float) area.getCentreY() - 13, 1.0f, 22.0f);
    g.setColour (kInk);
    g.setFont (tracked (sansBold (27.0f), 0.04f));
    g.drawText (product, word.getRight() + 28, area.getY() - 2, 110, area.getHeight(), juce::Justification::centredLeft);
    g.setFont (tracked (mono (10.5f), 0.12f)); g.setColour (kMuted);
    const int px = word.getRight() + 28 + (int) std::ceil (juce::GlyphArrangement::getStringWidth (sansBold (27.0f), product) * 1.04f) + 14;
    g.drawText (code, px, area.getY() - 2, 260, area.getHeight(), juce::Justification::centredLeft);
}

// ---- section plate -------------------------------------------------------------------------------
// Machined panel; the section colour lives in a lit index pip, the knob arcs and the LEDs.
inline void drawSection (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title, const juce::String& tag, juce::Colour col, bool /*lightText*/)
{
    auto b = bounds.toFloat();
    drawPlate (g, b, 8.0f, juce::Colour (0xff171a21), juce::Colour (0xff0e1015));
    auto strip = b.removeFromTop (21.0f).reduced (11.0f, 0);
    const auto pip = juce::Rectangle<float> (strip.getX(), strip.getCentreY() - 4.5f + 1.0f, 3.0f, 9.0f);
    g.setColour (col.withAlpha (0.28f)); g.fillRoundedRectangle (pip.expanded (2.0f), 2.5f);
    g.setColour (col); g.fillRoundedRectangle (pip, 1.5f);
    g.setColour (kInk);
    g.setFont (tracked (sansBold (11.0f), 0.10f));
    g.drawText (title, strip.withTrimmedLeft (11.0f).translated (0, 1.0f), juce::Justification::centredLeft);
    g.setFont (tracked (mono (9.0f), 0.12f));
    g.setColour (kMuted.withAlpha (0.8f));
    g.drawText (tag, strip.translated (0, 1.0f), juce::Justification::centredRight);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (strip.getX(), strip.getBottom() + 1.0f, strip.getWidth(), 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.04f));
    g.fillRect (strip.getX(), strip.getBottom() + 2.0f, strip.getWidth(), 1.0f);
}

// ---- machined knob (shared by all look-and-feels) -----------------------------------------------
inline void drawKnob (juce::Graphics& g, juce::Rectangle<float> area, float pos, float start, float end, juce::Colour accent, bool bipolar, float bipolarZero)
{
    const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 1.0f;
    const auto c = area.getCentre();
    const float ang = start + pos * (end - start);
    auto at = [&] (float a, float rad) { return juce::Point<float> (c.x + rad * std::sin (a), c.y - rad * std::cos (a)); };

    // scale ticks
    for (int i = 0; i <= 10; ++i)
    {
        const float a = start + (end - start) * (float) i / 10.0f;
        g.setColour (kInk.withAlpha (i % 5 == 0 ? 0.30f : 0.14f));
        g.drawLine (juce::Line<float> (at (a, r - 0.5f), at (a, r - (i % 5 == 0 ? 3.5f : 2.5f))), 1.0f);
    }
    // value ring: dark groove, then the lit arc with a halo
    const float ar = r - 6.0f;
    juce::Path track; track.addCentredArc (c.x, c.y, ar, ar, 0, start, end, true);
    g.setColour (juce::Colour (0xff07080a)); g.strokePath (track, juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float from = bipolar ? start + bipolarZero * (end - start) : start;
    if (std::abs (ang - from) > 0.001f)
    {
        juce::Path arc; arc.addCentredArc (c.x, c.y, ar, ar, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
        g.setColour (accent.withAlpha (0.18f)); g.strokePath (arc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent); g.strokePath (arc, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    // body: shadow, knurled skirt, machined cap
    const float kr = ar - 5.0f;
    if (kr < 4.0f) return;
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillEllipse (c.x - kr - (float) i * 0.5f, c.y - kr + (float) i * 0.9f, (kr + (float) i * 0.5f) * 2, (kr + (float) i * 0.5f) * 2);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3e48), c.x, c.y - kr, juce::Colour (0xff0b0c0f), c.x, c.y + kr, false));
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
    for (int i = 0; i < 40; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / 40.0f + ang;
        g.setColour ((i & 1) ? juce::Colours::black.withAlpha (0.35f) : juce::Colours::white.withAlpha (0.05f));
        g.drawLine (juce::Line<float> (at (a, kr - 0.5f), at (a, kr * 0.84f)), 1.0f);
    }
    const float cr = kr * 0.78f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15171c), c.x, c.y - cr, juce::Colour (0xff272a32), c.x, c.y + cr, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.10f), c.x - cr * 0.3f, c.y - cr * 0.6f, juce::Colours::transparentWhite, c.x, c.y + cr * 0.3f, true);
    g.setGradientFill (sheen); g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 0.8f);
    juce::Path rim; rim.addCentredArc (c.x, c.y, kr - 0.5f, kr - 0.5f, 0, -1.1f, 1.1f, true);
    g.setColour (juce::Colours::white.withAlpha (0.22f)); g.strokePath (rim, juce::PathStrokeType (0.8f));
    // pointer
    g.setColour (kInk);
    g.drawLine (juce::Line<float> (at (ang, cr * 0.28f), at (ang, kr - 1.5f)), 2.2f);
    g.setColour (accent);
    const auto tip = at (ang, kr - 1.8f);
    g.fillEllipse (tip.x - 1.3f, tip.y - 1.3f, 2.6f, 2.6f);
}

// ---- look and feel -------------------------------------------------------------------------------
class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, kValue);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, flux (1));
        setColour (juce::ComboBox::backgroundColourId, kGlass);
        setColour (juce::ComboBox::textColourId, kInk);
        setColour (juce::ComboBox::outlineColourId, kEdge);
        setColour (juce::ComboBox::arrowColourId, kMuted);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff101217));
        setColour (juce::PopupMenu::textColourId, kInk);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff22262f));
        setColour (juce::PopupMenu::highlightedTextColourId, kInk);
        setColour (juce::Label::textColourId, kInk);
        setColour (juce::TextButton::buttonColourId, kRaised);
        setColour (juce::TextButton::textColourOffId, kInk);
        setColour (juce::TextEditor::backgroundColourId, kGlass);
        setColour (juce::TextEditor::textColourId, kInk);
        setColour (juce::TextEditor::highlightColourId, flux (1));
        setColour (juce::CaretComponent::caretColourId, kInk);
        setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff101217));
        setColour (juce::TooltipWindow::textColourId, kInk);
        setColour (juce::TooltipWindow::outlineColourId, kEdge);
    }

    juce::Font getComboBoxFont (juce::ComboBox& b) override { return mono (b.getHeight() > 30 ? 13.0f : (b.getWidth() < 80 ? 10.5f : 11.5f), true); }
    juce::Font getPopupMenuFont() override { return mono (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return mono (13.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) override
    {
        const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
        const float zero = bipolar ? (float) s.valueToProportionOfLength (0.0) : 0.0f;
        drawKnob (g, juce::Rectangle<int> (x, y, w, h).toFloat().reduced (2.0f), pos, start, end,
                  s.findColour (juce::Slider::rotarySliderFillColourId), bipolar, zero);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
        drawGlass (g, r, 5.0f);
        const auto arrow = box.findColour (juce::ComboBox::arrowColourId);
        juce::Path chev; const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
        chev.startNewSubPath (ax - 3.5f, ay - 1.8f); chev.lineTo (ax, ay + 1.8f); chev.lineTo (ax + 3.5f, ay - 1.8f);
        g.setColour (arrow.withAlpha (0.9f)); g.strokePath (chev, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 26, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }
    void drawLabel (juce::Graphics& g, juce::Label& l) override
    {
        // lit text inside dark display combos (program / style boxes)
        if (auto* box = dynamic_cast<juce::ComboBox*> (l.getParentComponent()); box && box->getHeight() > 30 && ! l.isBeingEdited())
        {
            g.setFont (getComboBoxFont (*box));
            drawGlowText (g, l.getText(), l.getLocalBounds().toFloat(), juce::Justification::centredLeft, l.findColour (juce::Label::textColourId));
            return;
        }
        // value read-outs under knobs and faders: plain mono text, no box
        if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr && ! l.isBeingEdited())
        {
            g.setFont (mono (12.5f));
            g.setColour (l.findColour (juce::Label::textColourId));
            g.drawFittedText (l.getText(), l.getLocalBounds(), l.getJustificationType(), 1, 1.0f);
            return;
        }
        LookAndFeel_V4::drawLabel (g, l);
    }

    static void drawRaised (juce::Graphics& g, juce::Rectangle<float> r, bool hi, bool down)
    {
        g.setColour (juce::Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (r.translated (0, 1.0f), 5.0f);
        g.setGradientFill (juce::ColourGradient (down ? juce::Colour (0xff14161b) : juce::Colour (hi ? 0xff2c3039 : 0xff252830), r.getX(), r.getY(),
                                                 down ? juce::Colour (0xff1c1f25) : juce::Colour (0xff15171c), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colours::black); g.drawRoundedRectangle (r, 5.0f, 1.0f);
        if (! down)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawLine (r.getX() + 4, r.getY() + 1.0f, r.getRight() - 4, r.getY() + 1.0f, 1.0f);
        }
    }
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down) override
    {
        drawRaised (g, b.getLocalBounds().toFloat().reduced (1.0f), hi, down);
    }

    // ON / OFF switch with a status LED
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hi, bool down) override
    {
        const auto rr = b.getLocalBounds().toFloat().withSizeKeepingCentre (juce::jmin (56.0f, (float) b.getWidth() - 2.0f), juce::jmin (24.0f, (float) b.getHeight() - 2.0f));
        const bool on = b.getToggleState();
        drawRaised (g, rr, hi, down);
        const auto col = b.findColour (juce::ToggleButton::tickColourId);
        drawLed (g, { rr.getX() + 11.0f, rr.getCentreY() }, 3.0f, col, on);
        g.setColour (on ? kInk : kMuted);
        g.setFont (tracked (mono (10.5f, true), 0.08f));
        const auto txt = b.getButtonText().isEmpty() || b.getButtonText() == "ON" ? juce::String (on ? "ON" : "OFF") : b.getButtonText();
        g.drawText (txt, rr.withTrimmedLeft (17.0f), juce::Justification::centred);
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (mono (11.0f));
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::textColourId, kValue);
        return l;
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
        g.setColour (kEdge); g.drawRect (0, 0, w, h);
    }

    void drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h) override
    {
        g.fillAll (findColour (juce::TooltipWindow::backgroundColourId));
        g.setColour (kEdge); g.drawRect (0, 0, w, h);
        g.setColour (kInk); g.setFont (mono (11.0f));
        g.drawFittedText (text, 8, 0, w - 16, h, juce::Justification::centredLeft, 3);
    }
};

// ---- whole-window scaling --------------------------------------------------------------------------
// Editors lay out at their design size; the window can be resized 70%–200% and everything scales.
struct Scaler
{
    int baseW = 0, baseH = 0;
    void init (juce::AudioProcessorEditor& e, int w, int h)
    {
        baseW = w; baseH = h;
        e.setResizable (true, true);
        e.setResizeLimits ((int) (w * 0.7), (int) (h * 0.7), w * 2, h * 2);
        if (auto* c = e.getConstrainer()) c->setFixedAspectRatio ((double) w / (double) h);
        e.setSize (w, h);
    }
    float scale (const juce::Component& e) const { return baseW > 0 ? (float) e.getWidth() / (float) baseW : 1.0f; }
    juce::Rectangle<int> design() const { return { 0, 0, baseW, baseH }; }
    void apply (juce::Component& e) const
    {
        const auto t = juce::AffineTransform::scale (scale (e));
        for (auto* ch : e.getChildren())
            if (dynamic_cast<juce::ResizableCornerComponent*> (ch) == nullptr)
                ch->setTransform (t);
    }
    juce::Rectangle<int> toWindow (const juce::Component& e, juce::Rectangle<int> r) const
    {
        return r.toFloat().transformedBy (juce::AffineTransform::scale (scale (e))).getSmallestIntegerContainer().expanded (1);
    }
};
} // namespace ae8brand
