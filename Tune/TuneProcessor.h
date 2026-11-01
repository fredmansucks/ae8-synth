#pragma once
#include <JuceHeader.h>
#include "TuneEngine.h"

namespace ae8tune
{
struct Style { const char* name; std::vector<std::pair<int, float>> v; };

// Vocal styles. Key and Scale are never changed by a style (they belong to the song).
inline const std::vector<Style>& styles()
{
    static const std::vector<Style> s =
    {
        { "Classic",          { { SPEED, 20 }, { AMOUNT, 1 }, { HUMANIZE, 0.2f } } },
        { "Hard Tune",        { { SPEED, 0 }, { AMOUNT, 1 }, { HUMANIZE, 0 } } },
        { "Natural",          { { SPEED, 90 }, { AMOUNT, 0.85f }, { HUMANIZE, 0.7f } } },
        { "Robot",            { { SPEED, 0 }, { HUMANIZE, 0 }, { DOUBLE, 0.35f }, { DIST_DRIVE, 0.35f }, { DIST_TONE, 0.45f }, { DIST_MIX, 0.3f } } },
        { "Duet",             { { SPEED, 15 }, { HUMANIZE, 0.25f }, { DOUBLE, 0.85f }, { REV_MIX, 0.12f } } },
        { "Big Harmony",      { { SPEED, 15 }, { HUMANIZE, 0.2f }, { HARMONY, 0.8f }, { HARM_INT, H_3RD_5TH }, { DOUBLE, 0.4f }, { REV_MIX, 0.2f }, { REV_SIZE, 0.7f } } },
        { "Modern Rap",       { { SPEED, 5 }, { HUMANIZE, 0.1f }, { DOUBLE, 0.3f }, { DIST_DRIVE, 0.25f }, { DIST_MIX, 0.2f },
                                { DLY_TIME, 3 }, { DLY_FB, 0.3f }, { DLY_MIX, 0.15f }, { REV_MIX, 0.15f } } },
        { "Melodic Trap",     { { SPEED, 0 }, { HUMANIZE, 0.05f }, { DLY_TIME, 2 }, { DLY_FB, 0.35f }, { DLY_MIX, 0.12f }, { REV_MIX, 0.22f }, { REV_SIZE, 0.6f } } },
        { "Lo-Fi Phone",      { { SPEED, 20 }, { DIST_DRIVE, 0.7f }, { DIST_TONE, 0.25f }, { DIST_MIX, 0.85f } } },
        { "Space Echo",       { { SPEED, 30 }, { HUMANIZE, 0.3f }, { DLY_TIME, 5 }, { DLY_FB, 0.55f }, { DLY_MIX, 0.3f }, { REV_SIZE, 0.92f }, { REV_MIX, 0.35f } } },
        { "Octave Monster",   { { SPEED, 10 }, { HARMONY, 0.85f }, { HARM_INT, H_OCT_DOWN }, { DIST_DRIVE, 0.3f }, { DIST_MIX, 0.25f } } },
        { "Fifth Up Choir",   { { SPEED, 25 }, { HUMANIZE, 0.3f }, { HARMONY, 0.7f }, { HARM_INT, H_5TH_UP }, { DOUBLE, 0.5f }, { REV_MIX, 0.3f }, { REV_SIZE, 0.85f } } },
    };
    return s;
}
} // namespace ae8tune

class TuneProcessor : public juce::AudioProcessor
{
public:
    TuneProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AE-8 TUNE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return (int) ae8tune::styles().size(); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentProgram { 0 };
    ae8tune::TuneEngine engine;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::array<std::atomic<float>*, ae8tune::NUM_PARAMS> raw {};
    juce::AudioBuffer<float> work;
#ifdef AE8_TUNE_DEMO   // screenshot builds only: feeds an off-key synthetic voice without an audio device
    struct Demo : juce::HighResolutionTimer { TuneProcessor& p; double ph = 0, t = 0; explicit Demo (TuneProcessor& o) : p (o) {}
        void hiResTimerCallback() override {
            juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m;
            for (int i = 0; i < 480; ++i) { t += 1.0 / 48000; const double tt = std::fmod (t, 6.0);
                const double note = tt < 1.5 ? 57.35 : tt < 2.0 ? 57.35 + (tt - 1.5) * 5.9 : tt < 3.5 ? 60.3 + 0.35 * std::sin (2 * ae8tune::kPiD * 5.5 * tt) : tt < 5 ? 62.55 : 63.6;
                ph += 440 * std::pow (2, (note - 69) / 12) / 48000; ph -= std::floor (ph);
                const float v = (float) (0.2 * (std::sin (2 * ae8tune::kPiD * ph) + 0.5 * std::sin (4 * ae8tune::kPiD * ph) + 0.3 * std::sin (6 * ae8tune::kPiD * ph)));
                b.setSample (0, i, v); b.setSample (1, i, v); }
            p.processBlock (b, m); } };
    Demo demo { *this };
public:
    void startDemo() { prepareToPlay (48000, 480); demo.startTimer (10); }
private:
#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TuneProcessor)
};
