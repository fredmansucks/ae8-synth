#pragma once
#include <JuceHeader.h>
#include "SynthEngine.h"
#include "Presets.h"
#include "Arp.h"

class AE8Processor : public juce::AudioProcessor
{
public:
    AE8Processor();
    ~AE8Processor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AE-8 POLY"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return (int) ae8::factoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentProgram { 0 };
    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiMessage& m);

    ae8::Engine engine;
    ae8::Arpeggiator arp;
    bool arpWasOn = false, latchWasOn = false;
    double freeBeat = 0.0, sampleRate = 44100.0;
    std::array<std::atomic<float>*, ae8::NUM_PARAMS> raw {};
    juce::AudioBuffer<float> work;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AE8Processor)
};
