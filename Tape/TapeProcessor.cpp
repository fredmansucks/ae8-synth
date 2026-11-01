#include "TapeProcessor.h"
#include "TapeEditor.h"

using namespace ae8tape;

juce::AudioProcessorValueTreeState::ParameterLayout TapeProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : paramDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.kind == ae8::Choice)
        {
            juce::StringArray items; for (auto* c : d.choices) items.add (c);
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, items, (int) d.def));
        }
        else if (d.kind == ae8::Bool)
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.kind == ae8::Int)
            layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def,
                                                                   juce::AudioParameterIntAttributes().withLabel (d.unit)));
        else
        {
            juce::NormalisableRange<float> r (d.min, d.max);
            if (d.skewCentre > 0.0f) r.setSkewForCentre (d.skewCentre);
            const juce::String unit (d.unit);
            auto attrs = juce::AudioParameterFloatAttributes().withLabel (unit)
                .withStringFromValueFunction ([unit] (float v, int)
                {
                    if (unit == "ms") return v < 0.5f ? juce::String ("INSTANT") : juce::String (v, 0) + " ms";
                    if (unit == "dB") return juce::String (v, 1) + " dB";
                    return juce::String (juce::roundToInt (v * 100.0f)) + "%";
                });
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, r, d.def, attrs));
        }
    }
    return layout;
}

TapeProcessor::TapeProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AE8TAPE", createLayout())
{
    for (int i = 0; i < NUM_PARAMS; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (paramDefs()[(size_t) i].id);
}

bool TapeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet(), in = l.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void TapeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    setLatencySamples (engine.latencySamples());
    work.setSize (2, juce::jmax (512, samplesPerBlock));
}

void TapeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    for (int i = 0; i < NUM_PARAMS; ++i) engine.setParam (i, raw[(size_t) i]->load());

    if (work.getNumSamples() < n) work.setSize (2, n, false, false, true);
    const int inCh = getTotalNumInputChannels();
    const float* inL = buffer.getReadPointer (0);
    const float* inR = inCh > 1 ? buffer.getReadPointer (1) : nullptr;
    engine.process (inL, inR, work.getWritePointer (0), work.getWritePointer (1), n);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, work, 0, 0, n);
        buffer.copyFrom (1, 0, work, 1, 0, n);
    }
    else
    {
        buffer.copyFrom (0, 0, work.getReadPointer (0), n, 0.5f);
        buffer.addFrom  (0, 0, work.getReadPointer (1), n, 0.5f);
    }
}

void TapeProcessor::setCurrentProgram (int index)
{
    const auto& s = presets();
    if (! juce::isPositiveAndBelow (index, (int) s.size())) return;
    currentProgram = index;
    std::array<float, NUM_PARAMS> vals {};
    for (int i = 0; i < NUM_PARAMS; ++i) vals[(size_t) i] = paramDefs()[(size_t) i].def;
    for (auto& kv : s[(size_t) index].v) vals[(size_t) kv.first] = kv.second;
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        if (i == MIX || i == OUTPUT || i == STOP) continue;   // keep the user's levels
        if (auto* p = apvts.getParameter (paramDefs()[(size_t) i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (vals[(size_t) i]));
    }
}

const juce::String TapeProcessor::getProgramName (int index)
{
    const auto& s = presets();
    return juce::isPositiveAndBelow (index, (int) s.size()) ? juce::String (s[(size_t) index].name) : juce::String();
}

void TapeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void TapeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* TapeProcessor::createEditor() { return new TapeEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TapeProcessor(); }
