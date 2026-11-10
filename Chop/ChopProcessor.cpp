#include "ChopProcessor.h"
#include "ChopEditor.h"

using namespace ae8chop;

juce::AudioProcessorValueTreeState::ParameterLayout ChopProcessor::createLayout()
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
            layout.add (std::make_unique<juce::AudioParameterInt> (pid, d.name, (int) d.min, (int) d.max, (int) d.def));
        else
        {
            juce::NormalisableRange<float> r (d.min, d.max);
            if (d.skewCentre > 0.0f) r.setSkewForCentre (d.skewCentre);
            const juce::String unit (d.unit), id (d.id);
            auto attrs = juce::AudioParameterFloatAttributes().withLabel (unit)
                .withStringFromValueFunction ([unit, id] (float v, int) -> juce::String
                {
                    if (id == "bits") return v >= 15.95f ? juce::String ("OFF") : juce::String (juce::roundToInt (v));
                    if (id == "rate") return v >= 47900.0f ? juce::String ("OFF") : juce::String (v / 1000.0f, 1) + "k";
                    if (id == "cutoff") return v >= 1000.0f ? juce::String (v / 1000.0f, 1) + "k" : juce::String (juce::roundToInt (v));
                    if (unit == "ms") return juce::String (juce::roundToInt (v)) + " ms";
                    return juce::String (juce::roundToInt (v * 100.0f)) + "%";
                });
            layout.add (std::make_unique<juce::AudioParameterFloat> (pid, d.name, r, d.def, attrs));
        }
    }
    return layout;
}

ChopProcessor::ChopProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr,
#ifdef AE8_FLIP
      "AE8FLIP",
#else
      "AE8CHOP",
#endif
      createLayout())
{
    formats.registerBasicFormats();
    for (int i = 0; i < NUM_PARAMS; ++i)
        raw[(size_t) i] = apvts.getRawParameterValue (paramDefs()[(size_t) i].id);
    for (auto* id : { "chopmode", "sens", "slices", "division", "targetkey" })
        apvts.addParameterListener (id, this);
    events.reserve (1024);
}

ChopProcessor::~ChopProcessor()
{
    for (auto* id : { "chopmode", "sens", "slices", "division", "targetkey" })
        apvts.removeParameterListener (id, this);
    cancelPendingUpdate();
}

bool ChopProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void ChopProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    work.setSize (2, juce::jmax (512, samplesPerBlock));
    keyboardState.reset();
}

void ChopProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    for (int i = 0; i < NUM_PARAMS; ++i) engine.setParam (i, raw[(size_t) i]->load());

    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    events.clear();
    for (const auto m : midi)
    {
        const auto msg = m.getMessage();
        if (msg.isNoteOn()) events.push_back ({ m.samplePosition, msg.getNoteNumber(), msg.getFloatVelocity(), true });
        else if (msg.isNoteOff()) events.push_back ({ m.samplePosition, msg.getNoteNumber(), 0.0f, false });
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
    }
    midi.clear();

    double bpm = 120, ppq = -1; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }

    if (work.getNumSamples() < n) work.setSize (2, n, false, false, true);
    engine.process (work.getWritePointer (0), work.getWritePointer (1), n, events, bpm, ppq, playing);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.copyFrom (0, 0, work, 0, 0, n);
        buffer.copyFrom (1, 0, work, 1, 0, n);
        for (int c = 2; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    }
    else
    {
        buffer.copyFrom (0, 0, work.getReadPointer (0), n, 0.5f);
        buffer.addFrom  (0, 0, work.getReadPointer (1), n, 0.5f);
    }
}

//==============================================================================
bool ChopProcessor::loadFile (const juce::File& f)
{
    std::unique_ptr<juce::AudioFormatReader> r (formats.createReaderFor (f));
    if (! r || r->lengthInSamples <= 0) return false;
    const auto len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 600));
    juce::AudioBuffer<float> b ((int) juce::jmax (1u, r->numChannels), len);
    r->read (&b, 0, len, 0, true, true);
    auto s = std::make_shared<Sample>();
    s->sr = r->sampleRate;
    s->L.assign (b.getReadPointer (0), b.getReadPointer (0) + len);
    const int rc = b.getNumChannels() > 1 ? 1 : 0;
    s->R.assign (b.getReadPointer (rc), b.getReadPointer (rc) + len);
    setSampleData (s, f.getFileName(), f.getFullPathName());
    return true;
}

void ChopProcessor::setSampleData (std::shared_ptr<Sample> s, const juce::String& name, const juce::String& path)
{
    sampleName = name; samplePath = path;
    std::vector<float> mono (s->L.size());
    for (size_t i = 0; i < mono.size(); ++i) mono[i] = 0.5f * (s->L[i] + s->R[i]);
    analysis = analyse (mono, s->sr);
    engine.allNotesOff();
    previous = std::atomic_load (&engine.sample);
    std::atomic_store (&engine.sample, std::shared_ptr<const Sample> (s));
    buildOverview();
    if (! restoring)
    {
        detectedBpm = analysis.bpm;
        engine.sampleBpm = detectedBpm;
        setDetectedKey (analysis.keyRoot, analysis.keyMinor);
        manual.clear();
        for (auto& sl : engine.slice) sl.reset();
        engine.selected = 0;
        reslice();
    }
    ++changeCounter;
}

void ChopProcessor::buildOverview()
{
    auto s = currentSample();
    overview.assign ((size_t) kOverview, { 0, 0, 0 });
    if (! s || s->size() == 0) return;
    const double k1 = 1 - std::exp (-2 * kPiD * 220 / s->sr), k2 = 1 - std::exp (-2 * kPiD * 2200 / s->sr);
    double lp1 = 0, lp1b = 0, lp2 = 0;
    const int64_t n = s->size();
    for (int64_t i = 0; i < n; ++i)
    {
        const double x = 0.5 * (s->L[(size_t) i] + s->R[(size_t) i]);
        lp1 += (x - lp1) * k1; lp1b += (lp1 - lp1b) * k1; lp2 += (x - lp2) * k2;
        const float lo = (float) std::fabs (lp1b), hi = (float) std::fabs (x - lp2), mid = (float) std::fabs (lp2 - lp1b);
        auto& c = overview[(size_t) (i * kOverview / n)];
        c.lo = std::max (c.lo, lo); c.mid = std::max (c.mid, mid); c.hi = std::max (c.hi, hi);
    }
}

void ChopProcessor::parameterChanged (const juce::String& id, float)
{
    if (id == "targetkey") { engine.keyShiftSemis = keyShift(); return; }
    if (! restoring) triggerAsyncUpdate();
}

void ChopProcessor::handleAsyncUpdate() { reslice(); }

void ChopProcessor::reslice()
{
    auto s = currentSample();
    if (! s || s->size() == 0) return;
    const int mode = (int) raw[CHOP_MODE]->load();
    const int64_t len = s->size();
    std::vector<int64_t> m;
    static const double div[5] = { 4, 2, 1, 0.5, 0.25 };
    switch (mode)
    {
        case CHOP_TRANSIENT: m = transientSlices (analysis, raw[SENS]->load(), kMaxSlices); break;
        case CHOP_BEAT:      m = beatSlices (len, s->sr, detectedBpm, div[juce::jlimit (0, 4, (int) raw[DIVISION]->load())], kMaxSlices); break;
        case CHOP_EQUAL:     m = equalSlices (len, (int) raw[SLICES]->load()); break;
        default:             m = manual.empty() ? markers() : manual; break;
    }
    if (m.empty()) m = { 0 };
    applyMarkers (m);
}

void ChopProcessor::applyMarkers (const std::vector<int64_t>& in)
{
    auto s = currentSample();
    const int64_t len = s ? s->size() : 0;
    std::vector<int64_t> m;
    for (auto p : in) if (p >= 0 && p < len - 32) m.push_back (p);
    std::sort (m.begin(), m.end());
    m.erase (std::unique (m.begin(), m.end(), [] (int64_t a, int64_t b) { return std::llabs (a - b) < 64; }), m.end());
    if ((int) m.size() > kMaxSlices) m.resize ((size_t) kMaxSlices);
    engine.numSlices = 0;                     // stop new triggers while markers change
    for (size_t i = 0; i < m.size(); ++i) engine.starts[i] = m[i];
    engine.numSlices = (int) m.size();
    if (engine.selected.load() >= (int) m.size()) engine.selected = juce::jmax (0, (int) m.size() - 1);
    if ((int) raw[CHOP_MODE]->load() == CHOP_MANUAL) manual = m;
    ++changeCounter;
}

std::vector<int64_t> ChopProcessor::markers() const
{
    std::vector<int64_t> m;
    for (int i = 0; i < engine.numSlices.load(); ++i) m.push_back (engine.starts[(size_t) i].load());
    return m;
}

void ChopProcessor::setManualMarkers (std::vector<int64_t> m)
{
    manual = std::move (m);
    if ((int) raw[CHOP_MODE]->load() != CHOP_MANUAL)
    {
        restoring = true;   // don't let the listener reslice over our markers
        if (auto* p = apvts.getParameter ("chopmode")) p->setValueNotifyingHost (p->convertTo0to1 ((float) CHOP_MANUAL));
        restoring = false;
    }
    applyMarkers (manual);
    manual = markers();
}

void ChopProcessor::find()
{
    auto s = currentSample(); if (! s) return;
    setManualMarkers (findSlices (analysis, s->size(), (int) raw[SLICES]->load()));
}

void ChopProcessor::randomise()
{
    auto s = currentSample(); if (! s) return;
    randomSeed = randomSeed * 1664525u + 1013904223u;
    setManualMarkers (randomSlices (analysis, s->size(), (int) raw[SLICES]->load(), randomSeed));
}

void ChopProcessor::setDetectedBpm (double b)
{
    detectedBpm = juce::jlimit (20.0, 400.0, b);
    engine.sampleBpm = detectedBpm;
    if ((int) raw[CHOP_MODE]->load() == CHOP_BEAT) reslice();
    ++changeCounter;
}

void ChopProcessor::setDetectedKey (int root, bool minor)
{
    keyRoot = ((root % 12) + 12) % 12; keyMinor = minor;
    engine.keyShiftSemis = keyShift();
    ++changeCounter;
}

int ChopProcessor::keyShift() const
{
    const int t = (int) raw[TARGET_KEY]->load();
    return ae8chop::keyShift (keyRoot, keyMinor, t % 12, t >= 12);
}

juce::File ChopProcessor::exportMidi()
{
    auto s = currentSample();
    const auto m = markers();
    if (! s || m.empty()) return {};
    juce::MidiMessageSequence seq;
    const int tpq = 960;
    const double bpm = detectedBpm;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) (60000000.0 / bpm)), 0);
    auto toTicks = [&] (int64_t p) { return (double) p / s->sr * bpm / 60.0 * tpq; };
    for (size_t i = 0; i < m.size(); ++i)
    {
        const double a = toTicks (m[i]), b = toTicks (i + 1 < m.size() ? m[i + 1] : s->size());
        seq.addEvent (juce::MidiMessage::noteOn (1, 36 + (int) i, (juce::uint8) 100), a);
        seq.addEvent (juce::MidiMessage::noteOff (1, 36 + (int) i), juce::jmax (a + 1, b - 1));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (tpq); mf.addTrack (seq);
    const auto base = sampleName.isEmpty() ? juce::String ("chops") : juce::File::createLegalFileName (sampleName.upToLastOccurrenceOf (".", false, false));
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("AE-8 CHOP - " + base + ".mid");
    f.deleteFile();
    if (auto os = f.createOutputStream()) mf.writeTo (*os);
    return f;
}

//==============================================================================
void ChopProcessor::setCurrentProgram (int index)
{
    const auto& s = presets();
    if (! juce::isPositiveAndBelow (index, (int) s.size())) return;
    currentProgram = index;
#ifdef AE8_FLIP
    engine.flipMode = index;
#endif
    static const int touched[] = { LAYOUT, VOICE, REPEAT, REPEAT_RATE, TRIPLET, GLIDE, SWING, VEL, CUTOFF, RES, FTYPE, BITS, RATE, DRIVE, WARP };
    std::array<float, NUM_PARAMS> vals {};
    for (int i = 0; i < NUM_PARAMS; ++i) vals[(size_t) i] = paramDefs()[(size_t) i].def;
    for (auto& kv : s[(size_t) index].v) vals[(size_t) kv.first] = kv.second;
    for (int i : touched)
        if (auto* p = apvts.getParameter (paramDefs()[(size_t) i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (vals[(size_t) i]));
}

const juce::String ChopProcessor::getProgramName (int index)
{
    const auto& s = presets();
    return juce::isPositiveAndBelow (index, (int) s.size()) ? juce::String (s[(size_t) index].name) : juce::String();
}

void ChopProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);

    juce::ValueTree smp ("SAMPLE");
    smp.setProperty ("name", sampleName, nullptr);
    smp.setProperty ("path", samplePath, nullptr);
    smp.setProperty ("bpm", detectedBpm, nullptr);
    smp.setProperty ("keyRoot", keyRoot, nullptr);
    smp.setProperty ("keyMinor", keyMinor, nullptr);
    smp.setProperty ("selected", engine.selected.load(), nullptr);
    juce::StringArray mk; for (auto p : markers()) mk.add (juce::String (p));
    smp.setProperty ("markers", mk.joinIntoString (","), nullptr);
    juce::StringArray man; for (auto p : manual) man.add (juce::String (p));
    smp.setProperty ("manual", man.joinIntoString (","), nullptr);
    for (int i = 0; i < kMaxSlices; ++i)
    {
        juce::StringArray v; for (int k = 0; k < NUM_SP; ++k) v.add (juce::String (engine.slice[(size_t) i][k]));
        smp.setProperty ("s" + juce::String (i), v.joinIntoString (","), nullptr);
    }
    // embed the audio (FLAC) so the set still opens when the file moves; up to 5 minutes
    if (auto s = currentSample(); s && s->size() > 0 && s->size() < (int64_t) (s->sr * 300))
    {
        juce::MemoryBlock mb;
        juce::FlacAudioFormat flac;
        auto* os = new juce::MemoryOutputStream (mb, false);
        std::unique_ptr<juce::AudioFormatWriter> w (flac.createWriterFor (os, s->sr, 2, 24, {}, 5));
        if (w)
        {
            const float* ch[2] = { s->L.data(), s->R.data() };
            w->writeFromFloatArrays (ch, 2, (int) s->size());
            w.reset();
            smp.setProperty ("audio", mb.toBase64Encoding(), nullptr);
        }
        else delete os;
    }
    state.appendChild (smp, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ChopProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (! xml || ! xml->hasTagName (apvts.state.getType())) return;
    auto tree = juce::ValueTree::fromXml (*xml);
    auto smp = tree.getChildWithName ("SAMPLE");
    tree.removeChild (smp, nullptr);
    restoring = true;
    currentProgram = (int) tree.getProperty ("program", 0);
    apvts.replaceState (tree);

    if (smp.isValid())
    {
        std::shared_ptr<Sample> s;
        const juce::String b64 = smp.getProperty ("audio").toString();
        if (b64.isNotEmpty())
        {
            juce::MemoryBlock mb;
            if (mb.fromBase64Encoding (b64))
            {
                juce::FlacAudioFormat flac;
                std::unique_ptr<juce::AudioFormatReader> r (flac.createReaderFor (new juce::MemoryInputStream (mb, true), true));
                if (r)
                {
                    const int len = (int) r->lengthInSamples;
                    juce::AudioBuffer<float> b (2, len);
                    r->read (&b, 0, len, 0, true, true);
                    s = std::make_shared<Sample>(); s->sr = r->sampleRate;
                    s->L.assign (b.getReadPointer (0), b.getReadPointer (0) + len);
                    s->R.assign (b.getReadPointer (1), b.getReadPointer (1) + len);
                }
            }
        }
        if (s) setSampleData (s, smp.getProperty ("name").toString(), smp.getProperty ("path").toString());
        else if (juce::File f (smp.getProperty ("path").toString()); f.existsAsFile()) loadFile (f);

        detectedBpm = (double) smp.getProperty ("bpm", 120.0);
        engine.sampleBpm = detectedBpm;
        setDetectedKey ((int) smp.getProperty ("keyRoot", 0), (bool) smp.getProperty ("keyMinor", false));
        for (int i = 0; i < kMaxSlices; ++i)
        {
            juce::StringArray v; v.addTokens (smp.getProperty ("s" + juce::String (i)).toString(), ",", "");
            for (int k = 0; k < NUM_SP && k < v.size(); ++k) engine.slice[(size_t) i].v[(size_t) k] = v[k].getFloatValue();
        }
        auto parse = [] (const juce::String& str) { std::vector<int64_t> r; juce::StringArray a; a.addTokens (str, ",", ""); for (auto& x : a) if (x.isNotEmpty()) r.push_back (x.getLargeIntValue()); return r; };
        manual = parse (smp.getProperty ("manual").toString());
        applyMarkers (parse (smp.getProperty ("markers").toString()));
        engine.selected = (int) smp.getProperty ("selected", 0);
    }
    restoring = false;
    ++changeCounter;
}

juce::AudioProcessorEditor* ChopProcessor::createEditor() { return new ChopEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChopProcessor(); }
