#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace ae8;

juce::AudioProcessorValueTreeState::ParameterLayout AE8Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : paramDefs())
    {
        const juce::ParameterID pid { d.id, 1 };
        switch (d.kind)
        {
            case Choice:
            {
                juce::StringArray items;
                for (auto* c : d.choices) items.add (c);
                layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, items, (int) d.def));
                break;
            }
            case Bool:
                layout.add (std::make_unique<juce::AudioParameterBool> (pid, d.name, d.def > 0.5f));
                break;
            case Int:
                layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def,
                                                                       juce::AudioParameterIntAttributes().withLabel (d.unit)));
                break;
            case Float:
            default:
            {
                juce::NormalisableRange<float> r (d.min, d.max);
                if (d.skewCentre > 0.0f) r.setSkewForCentre (d.skewCentre);
                const juce::String unit (d.unit);
                auto attrs = juce::AudioParameterFloatAttributes().withLabel (unit)
                    .withStringFromValueFunction ([unit] (float v, int)
                    {
                        if (unit == "Hz") return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz" : juce::String (v, v < 100 ? 1 : 0) + " Hz";
                        if (unit == "s")  return v < 1.0f ? juce::String (v * 1000.0f, 0) + " ms" : juce::String (v, 2) + " s";
                        if (unit == "dB") return juce::String (v, 1) + " dB";
                        if (unit == "ct") return juce::String (v, 1) + " ct";
                        return juce::String (v, 2);
                    });
                layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, r, d.def, attrs));
                break;
            }
        }
    }
    return layout;
}

AE8Processor::AE8Processor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "AE8", createLayout())
{
    for (int i = 0; i < NUM_PARAMS; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (paramDefs()[(size_t) i].id);
    setCurrentProgram (0);
}

bool AE8Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void AE8Processor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    engine.prepare (newSampleRate);
    arp.reset();
    work.setSize (2, juce::jmax (samplesPerBlock, 512));
}

void AE8Processor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())           engine.noteOff (m.getNoteNumber());
    else if (m.isPitchWheel())        engine.pitchBend ((float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isSustainPedalOn())    engine.setSustain (true);
    else if (m.isSustainPedalOff())   engine.setSustain (false);
    else if (m.isController() && m.getControllerNumber() == 1) engine.modWheel ((float) m.getControllerValue() / 127.0f);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff (m.isAllSoundOff());
}

void AE8Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    keyboardState.processNextMidiBuffer (midi, 0, n, true);   // on-screen keyboard

    for (int i = 0; i < NUM_PARAMS; ++i)
        engine.setParam (i, raw[(size_t) i]->load());

    // ---- arpeggiator settings
    const bool arpOn = raw[ARP_ON]->load() > 0.5f;
    const bool latch = raw[ARP_LATCH]->load() > 0.5f;
    arp.mode = (int) raw[ARP_MODE]->load();
    arp.rate = (int) raw[ARP_RATE]->load();
    arp.octaves = (int) raw[ARP_OCT]->load();
    arp.gate = raw[ARP_GATE]->load();
    arp.latch = latch;
    if (arpOn != arpWasOn) { engine.allNotesOff(); arp.reset(); arpWasOn = arpOn; }
    if (latch && ! latchWasOn) arp.syncLatch();
    latchWasOn = latch;

    // ---- host timeline (falls back to a free-running 120 BPM clock)
    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit (20.0, 999.0, *b);
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }
    const double bps = bpm / 60.0 / sampleRate;
    const double beat0 = playing ? ppq : freeBeat;

    // ---- gather everything the engine should receive this block, in time order
    struct Ev { int pos; int kind; juce::MidiMessage msg; ArpEvent a; };   // kind 0 = midi, 1 = arp
    std::vector<Ev> evs;
    std::vector<ArpEvent> arpOut;
    int s0 = 0;
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int t = juce::jlimit (0, juce::jmax (0, n - 1), meta.samplePosition);
        if (arpOn && (m.isNoteOn() || m.isNoteOff()))
        {
            if (t > s0) { arp.advance (s0, t, beat0, bps, playing, arpOut); s0 = t; }
            if (m.isNoteOn()) arp.noteOn (m.getNoteNumber(), m.getFloatVelocity());
            else              arp.noteOff (m.getNoteNumber());
        }
        else evs.push_back ({ t, 0, m, {} });
    }
    if (arpOn && s0 < n) arp.advance (s0, n, beat0, bps, playing, arpOut);
    for (auto& a : arpOut) evs.push_back ({ a.pos, 1, {}, a });
    std::stable_sort (evs.begin(), evs.end(), [] (const Ev& x, const Ev& y) { return x.pos < y.pos; });
    if (! playing) freeBeat += n * bps;

    if (work.getNumSamples() < n) work.setSize (2, n, false, false, true);
    float* L = work.getWritePointer (0);
    float* R = work.getWritePointer (1);

    int pos = 0;
    for (auto& e : evs)
    {
        if (e.pos > pos) { engine.render (L + pos, R + pos, e.pos - pos); pos = e.pos; }
        if (e.kind == 0) handleMidi (e.msg);
        else if (e.a.on) engine.noteOn (e.a.note, e.a.vel);
        else engine.noteOff (e.a.note);
    }
    if (pos < n) engine.render (L + pos, R + pos, n - pos);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, L, n);
        buffer.copyFrom (1, 0, R, n);
        for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.copyFrom (0, 0, L, n, 0.5f);
        buffer.addFrom  (0, 0, R, n, 0.5f);
    }
    midi.clear();
}

void AE8Processor::setCurrentProgram (int index)
{
    const auto& presets = factoryPresets();
    if (index < 0 || index >= (int) presets.size()) return;
    currentProgram = index;

    std::array<float, NUM_PARAMS> vals {};
    for (int i = 0; i < NUM_PARAMS; ++i) vals[(size_t) i] = paramDefs()[(size_t) i].def;
    for (const auto& kv : presets[(size_t) index].v) vals[(size_t) kv.first] = kv.second;

    for (int i = 0; i < NUM_PARAMS; ++i)
        if (auto* p = apvts.getParameter (paramDefs()[(size_t) i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (vals[(size_t) i]));
}

const juce::String AE8Processor::getProgramName (int index)
{
    const auto& presets = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) presets.size()) ? juce::String (presets[(size_t) index].name) : juce::String();
}

void AE8Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void AE8Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = (int) tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* AE8Processor::createEditor() { return new AE8Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AE8Processor(); }
