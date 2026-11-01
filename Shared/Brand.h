#pragma once
// AE-8 shared brand kit for plugin UIs: palette, fonts, look-and-feel, belt mark, header plate.
#include <JuceHeader.h>
#include "../Source/BrandPaths.h"

namespace ae8brand
{
inline const juce::Colour kBg      { 0xff050608 };
inline const juce::Colour kPanel   { 0xff111319 };
inline const juce::Colour kRaised  { 0xff1a1d25 };
inline const juce::Colour kLine    { 0xff2a2e38 };
inline const juce::Colour kInk     { 0xfff2f2ee };
inline const juce::Colour kMuted   { 0xff9b9fa8 };
inline juce::Colour flux (int i)
{
    static const juce::uint32 c[7] = { 0xff252466, 0xff2b46c8, 0xff4fd0e0, 0xff6fbd46, 0xfff7ec1e, 0xfff5901e, 0xffed1c22 };
    return juce::Colour (c[juce::jlimit (0, 6, i)]);
}

inline juce::Font sansBold (float h) { return juce::Font (juce::FontOptions (h, juce::Font::bold)); }
inline juce::Font mono (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h, bold ? juce::Font::bold : juce::Font::plain));
}

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

// Header plate: belt mark, wordmark, product name and catalog code, faint field lines, flux colorbar.
inline void drawHeader (juce::Graphics& g, juce::Rectangle<int> area, const juce::String& product, const juce::String& code)
{
    auto hb = area.toFloat();
    g.setColour (juce::Colour (0xff0b0c10)); g.fillRoundedRectangle (hb, 6.0f);
    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (area);
        const juce::Point<float> c (hb.getCentreX(), hb.getCentreY() + 4);
        g.setColour (kInk.withAlpha (0.07f));
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
                g.strokePath (p, juce::PathStrokeType (1.0f));
            }
    }
    g.setColour (kLine); g.drawRoundedRectangle (hb.reduced (0.5f), 6.0f, 1.0f);
    {
        auto bar = hb.reduced (12.0f, 0).removeFromBottom (3.0f).translated (0, -1);
        const float w = bar.getWidth() / 7.0f;
        for (int i = 0; i < 7; ++i) { g.setColour (flux (i)); g.fillRect (bar.getX() + w * (float) i, bar.getY(), w + 0.5f, bar.getHeight()); }
    }
    auto h = area.reduced (14, 0);
    drawBeltMark (g, h.removeFromLeft (70).withSizeKeepingCentre (70, 70).toFloat());
    h.removeFromLeft (8);
    auto word = h.removeFromLeft (86).withSizeKeepingCentre (86, 30);
    static const juce::Path wm = juce::Drawable::parseSVGPath (ae8::kWordmarkPath);
    g.setColour (kInk);
    g.fillPath (wm, wm.getTransformToScaleToFit (word.toFloat(), true, juce::Justification::centredLeft));
    g.setFont (sansBold (28.0f));
    g.drawText (product, word.getRight() + 12, area.getY(), 100, area.getHeight(), juce::Justification::centredLeft);
    g.setFont (mono (11.0f)); g.setColour (kMuted);
    g.drawText (code, word.getRight() + 112, area.getY(), 240, area.getHeight(), juce::Justification::centredLeft);
}

inline void drawSection (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title, const juce::String& tag, juce::Colour col, bool lightText)
{
    auto b = bounds.toFloat();
    g.setColour (kPanel); g.fillRoundedRectangle (b, 6.0f);
    auto strip = b.removeFromTop (20.0f);
    juce::Path p; p.addRoundedRectangle (strip.getX(), strip.getY(), strip.getWidth(), strip.getHeight(), 6.0f, 6.0f, true, true, false, false);
    g.setColour (col); g.fillPath (p);
    g.setColour (lightText ? kInk : kBg);
    g.setFont (sansBold (11.5f));
    g.drawText (title, strip.reduced (10, 0), juce::Justification::centredLeft);
    g.setFont (mono (10.0f));
    g.setColour ((lightText ? kInk : kBg).withAlpha (0.75f));
    g.drawText (tag, strip.reduced (10, 0), juce::Justification::centredRight);
}

class LookAndFeel : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
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
        setColour (juce::PopupMenu::highlightedBackgroundColourId, flux (1));
        setColour (juce::PopupMenu::highlightedTextColourId, kInk);
        setColour (juce::Label::textColourId, kInk);
        setColour (juce::TextButton::buttonColourId, kRaised);
        setColour (juce::TextButton::textColourOffId, kInk);
        setColour (juce::TextEditor::backgroundColourId, kBg);
        setColour (juce::TextEditor::textColourId, kInk);
    }
    juce::Font getComboBoxFont (juce::ComboBox& b) override { return mono (b.getWidth() < 80 ? 10.5f : 12.0f, true); }
    juce::Font getPopupMenuFont() override { return mono (13.0f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return mono (14.0f, true); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& s) override
    {
        const auto area = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (4.0f);
        const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
        const auto c = area.getCentre();
        const float ang = start + pos * (end - start);
        juce::Path track; track.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, end, true);
        g.setColour (kLine); g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const bool bipolar = s.getMinimum() < 0 && s.getMaximum() > 0;
        const float from = bipolar ? start + (float) ((0 - s.getMinimum()) / (s.getMaximum() - s.getMinimum())) * (end - start) : start;
        juce::Path arc; arc.addCentredArc (c.x, c.y, r - 2, r - 2, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
        g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
        g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float kr = r - 8;
        g.setColour (kBg); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        g.setColour (kLine); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.0f);
        juce::Path ptr; ptr.addRoundedRectangle (-1.5f, -kr + 3, 3.0f, kr * 0.6f, 1.5f);
        g.setColour (kInk); g.fillPath (ptr, juce::AffineTransform::rotation (ang).translated (c.x, c.y));
    }
    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
        g.setColour (box.findColour (juce::ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (box.findColour (juce::ComboBox::outlineColourId));    g.drawRoundedRectangle (r, 4.0f, 1.0f);
        juce::Path tri; const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
        tri.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId)); g.fillPath (tri);
    }
    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? kLine : (hi ? kRaised.brighter (0.15f) : kRaised)); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (kLine); g.drawRoundedRectangle (r, 4.0f, 1.0f);
    }
    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::textColourId, kInk);
        return l;
    }
};
} // namespace ae8brand
