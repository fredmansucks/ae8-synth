#include "TuneProcessor.h"
#include "TuneEditor.h"

using namespace ae8tune;

juce::AudioProcessorValueTreeState::ParameterLayout TuneProcessor::createLayout()
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

TuneProcessor::TuneProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AE8TUNE", createLayout())
{
    for (int i = 0; i < NUM_PARAMS; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (paramDefs()[(size_t) i].id);
}

bool TuneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet(), in = l.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono();
}

void TuneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    setLatencySamples (engine.latencySamples());
    work.setSize (2, juce::jmax (512, samplesPerBlock));
}

void TuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int n = buffer.getNumSamples();
    for (int i = 0; i < NUM_PARAMS; ++i) engine.setParam (i, raw[(size_t) i]->load());

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) engine.setTempo (*b);

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

void TuneProcessor::setCurrentProgram (int index)
{
    const auto& s = styles();
    if (! juce::isPositiveAndBelow (index, (int) s.size())) return;
    currentProgram = index;
    std::array<float, NUM_PARAMS> vals {};
    for (int i = 0; i < NUM_PARAMS; ++i) vals[(size_t) i] = paramDefs()[(size_t) i].def;
    for (auto& kv : s[(size_t) index].v) vals[(size_t) kv.first] = kv.second;
    for (int i = 0; i < NUM_PARAMS; ++i)
    {
        if (i == KEY || i == SCALE || i == MIX || i == OUTPUT) continue;   // keep the song's key and the user's levels
        if (auto* p = apvts.getParameter (paramDefs()[(size_t) i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (vals[(size_t) i]));
    }
}

const juce::String TuneProcessor::getProgramName (int index)
{
    const auto& s = styles();
    return juce::isPositiveAndBelow (index, (int) s.size()) ? juce::String (s[(size_t) index].name) : juce::String();
}

void TuneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void TuneProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* TuneProcessor::createEditor() { return new TuneEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TuneProcessor(); }
