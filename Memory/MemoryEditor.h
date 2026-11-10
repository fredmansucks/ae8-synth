#pragma once
#include <JuceHeader.h>
#include "../Shared/Brand.h"
#include "MemoryProcessor.h"
class MemoryEditor final : public juce::AudioProcessorEditor {
public:
 explicit MemoryEditor(MemoryProcessor&);
 ~MemoryEditor() override;
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 struct Control {
  juce::Slider knob;
  juce::Label title;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
 };
 MemoryProcessor& processor;
 ae8brand::LookAndFeel look;
 ae8brand::Scaler scaler;
 std::array<std::unique_ptr<Control>,6> knobs;
 juce::ComboBox programs;
 juce::TextButton prev{"<"}, next{">"};
 juce::Rectangle<int> header, display, conversion, colour, output;
 JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MemoryEditor)
};
