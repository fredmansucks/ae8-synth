#pragma once
#include <JuceHeader.h>
#include "TapeEngine.h"

namespace ae8tape
{
struct Preset { const char* name; std::vector<std::pair<int, float>> v; };

inline const std::vector<Preset>& presets()
{
    static const std::vector<Preset> s =
    {
        { "Master Deck 15 IPS", { { SATURATE, 0.3f }, { WOW, 0.05f }, { FLUTTER, 0.05f }, { WEAR, 0 }, { HISS, 0.08f }, { CROSSTALK, 0.05f }, { NR, 1 } } },
        { "Hot Mixdown",        { { INPUT, 6 }, { SATURATE, 0.6f }, { BIAS, -0.2f }, { WOW, 0.08f }, { FLUTTER, 0.08f }, { HISS, 0.1f }, { LOW, 1.5f }, { NR, 1 } } },
        { "Cassette Warm",      { { SATURATE, 0.45f }, { BIAS, 0.4f }, { WOW, 0.3f }, { FLUTTER, 0.3f }, { WEAR, 0.3f }, { HISS, 0.3f }, { CROSSTALK, 0.2f }, { HIGH, -2 } } },
        { "Lo-Fi Worn",         { { SATURATE, 0.55f }, { BIAS, 0.6f }, { WOW, 0.55f }, { FLUTTER, 0.45f }, { WEAR, 0.7f }, { HISS, 0.5f }, { CROSSTALK, 0.35f }, { LOW, 2 }, { HIGH, -4 } } },
        { "Seasick",            { { SATURATE, 0.35f }, { WOW, 0.9f }, { FLUTTER, 0.2f }, { WEAR, 0.2f }, { HISS, 0.2f } } },
        { "Half Speed",         { { SPEED, 0.5f }, { SATURATE, 0.4f }, { WOW, 0.15f }, { FLUTTER, 0.1f }, { HISS, 0.15f } } },
        { "Tape Stop FX",       { { SATURATE, 0.35f }, { WOW, 0.1f }, { FLUTTER, 0.1f }, { HISS, 0.1f } } },
        { "Dirty Bus",          { { INPUT, 9 }, { SATURATE, 0.85f }, { BIAS, -0.6f }, { WOW, 0.1f }, { FLUTTER, 0.15f }, { HISS, 0.15f }, { OUTPUT, -4 } } },
    };
    return s;
}
} // namespace ae8tape

class TapeProcessor : public juce::AudioProcessor
{
public:
    TapeProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "AE-8 TAPE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override { return (int) ae8tape::presets().size(); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentProgram { 0 };
    ae8tape::TapeEngine engine;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    std::array<std::atomic<float>*, ae8tape::NUM_PARAMS> raw {};
    juce::AudioBuffer<float> work;
#ifdef AE8_TAPE_DEMO
    struct Demo : juce::HighResolutionTimer { TapeProcessor& p; double ph = 0; explicit Demo (TapeProcessor& o) : p (o) {}
        void hiResTimerCallback() override {
            juce::AudioBuffer<float> b (2, 480); juce::MidiBuffer m;
            for (int i = 0; i < 480; ++i) { ph += 1.0 / 48000; const float v = (float) (0.4 * std::sin (2 * ae8tape::kPi * 220 * ph) * (0.6 + 0.4 * std::sin (2 * ae8tape::kPi * 0.7 * ph)));
                b.setSample (0, i, v); b.setSample (1, i, v * 0.8f); }
            p.processBlock (b, m); } };
    Demo demo { *this };
public:
    void startDemo() { prepareToPlay (48000, 480); demo.startTimer (10); }
private:
#endif
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TapeProcessor)
};
