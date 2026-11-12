#pragma once
#include <JuceHeader.h>
#include "MemoryEngine.h"
class MemoryProcessor final : public juce::AudioProcessor {
public:
  MemoryProcessor();
  void prepareToPlay(double sr, int block) override;
  void releaseResources() override {}
  bool isBusesLayoutSupported(const BusesLayout&) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override { return true; }
  const juce::String getName() const override { return "AE-8 MEMORY"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }
  double getTailLengthSeconds() const override { return 0.0; }
  int getNumPrograms() override { return 5; }
  int getCurrentProgram() override { return program; }
  void setCurrentProgram(int) override;
  const juce::String getProgramName(int) override;
  void changeProgramName(int, const juce::String&) override {}
  void getStateInformation(juce::MemoryBlock&) override;
  void setStateInformation(const void*, int) override;
  juce::AudioProcessorValueTreeState state;
  static juce::AudioProcessorValueTreeState::ParameterLayout layout();
private:
  ae8memory::Engine engine;
  float lastGain = -1.0f;
  std::atomic<int> program {0};
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryProcessor)
};
