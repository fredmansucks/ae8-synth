#pragma once
#include <JuceHeader.h>
#include "ChopEngine.h"
#include "ChopAnalysis.h"

namespace ae8chop
{
struct Preset { const char* name; std::vector<std::pair<int, float>> v; };

// Programs change how CHOP plays and colours the sound; they never replace your sample or chops.
inline const std::vector<Preset>& presets()
{
#ifdef AE8_FLIP
    static const std::vector<Preset> v = {
      { "Original Chops", {} }, { "Reverse Phrase", {} }, { "Displaced", {} },
      { "Rhythmic Invert", {} }, { "Half Time", {} }, { "Stutter Skip", {} },
      { "Alternate Starts", {} }
    };
    return v;
#else
    static const std::vector<Preset> s =
    {
        { "Clean Chop",     { { VOICE, 0 } } },
        { "Dusty Soul",     { { VOICE, 0 }, { BITS, 12 }, { RATE, 26000 }, { CUTOFF, 6000 }, { RES, 0.2f }, { DRIVE, 0.25f }, { SWING, 0.56f } } },
        { "SP Crunch",      { { VOICE, 0 }, { BITS, 10 }, { RATE, 22050 }, { DRIVE, 0.45f }, { CUTOFF, 9000 }, { SWING, 0.58f } } },
        { "Boom Bap Mono",  { { VOICE, 0 }, { SWING, 0.6f }, { DRIVE, 0.2f }, { CUTOFF, 12000 } } },
        { "Rolls 1/16",     { { VOICE, 1 }, { REPEAT, 1 }, { REPEAT_RATE, 2 } } },
        { "Trap Rolls 1/32T", { { VOICE, 1 }, { REPEAT, 1 }, { REPEAT_RATE, 3 }, { TRIPLET, 1 } } },
        { "Vapor Keys",     { { VOICE, 1 }, { LAYOUT, 1 }, { WARP, 1 }, { GLIDE, 0 }, { CUTOFF, 4500 }, { RES, 0.3f } } },
        { "Lo-Fi Phone",    { { VOICE, 0 }, { FTYPE, 2 }, { CUTOFF, 1600 }, { RES, 0.35f }, { BITS, 8 }, { DRIVE, 0.5f } } },
    };
    return s;
#endif
}

inline juce::Colour sliceColour (int i)
{
    static const int m[16] = { 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 1, 2, 3, 5 };
    static const juce::uint32 c[7] = { 0xff252466, 0xff2b46c8, 0xff4fd0e0, 0xff6fbd46, 0xfff7ec1e, 0xfff5901e, 0xffed1c22 };
    return juce::Colour (c[m[((i % 16) + 16) % 16]]);
}
} // namespace ae8chop

class ChopProcessor : public juce::AudioProcessor,
                      private juce::AudioProcessorValueTreeState::Listener,
                      private juce::AsyncUpdater
{
public:
    ChopProcessor();
    ~ChopProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
#ifdef AE8_FLIP
    const juce::String getName() const override { return "AE-8 FLIP"; }
#else
    const juce::String getName() const override { return "AE-8 CHOP"; }
#endif
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return (int) ae8chop::presets().size(); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- sample & chops (message thread)
    bool loadFile (const juce::File&);
    void setSampleData (std::shared_ptr<ae8chop::Sample>, const juce::String& name, const juce::String& path);
    void reslice();                                  // rebuild markers from the current chop mode
    void setManualMarkers (std::vector<int64_t>);    // switches to Manual
    void find();
    void randomise();
    void setDetectedBpm (double);
    void setDetectedKey (int root, bool minor);
    juce::File exportMidi();                         // slices in original order, as a .mid file
    std::vector<int64_t> markers() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;           // on-screen pads
    std::atomic<int> currentProgram { 0 };
    ae8chop::ChopEngine engine;

    // display data (message thread)
    struct Band { float lo, mid, hi; };
    std::vector<Band> overview;                      // 3-band peaks, kOverview columns
    static constexpr int kOverview = 4096;
    juce::String sampleName, samplePath;
    double detectedBpm = 120;
    int keyRoot = 0; bool keyMinor = false;
    int keyShift() const;
    std::shared_ptr<const ae8chop::Sample> currentSample() const { return std::atomic_load (&engine.sample); }
    std::atomic<int> changeCounter { 0 };            // bumps whenever markers/sample change (GUI repaints)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void parameterChanged (const juce::String&, float) override;
    void handleAsyncUpdate() override;
    void applyMarkers (const std::vector<int64_t>&);
    void buildOverview();

    std::array<std::atomic<float>*, ae8chop::NUM_PARAMS> raw {};
    ae8chop::Analysis analysis;
    std::vector<int64_t> manual;
    std::shared_ptr<const ae8chop::Sample> previous;   // keeps the old sample alive until the next swap
    juce::AudioFormatManager formats;
    juce::AudioBuffer<float> work;
    std::vector<ae8chop::NoteEvent> events;
    uint32_t randomSeed = 1;
    bool restoring = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChopProcessor)
};
