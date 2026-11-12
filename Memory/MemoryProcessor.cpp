#include "MemoryProcessor.h"
#include "MemoryEditor.h"
MemoryProcessor::MemoryProcessor()
 : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
   state(*this, nullptr, "AE8MEMORY", layout()) {}
juce::AudioProcessorValueTreeState::ParameterLayout MemoryProcessor::layout() {
  juce::AudioProcessorValueTreeState::ParameterLayout p;
  auto add=[&](const char* id,const char* name,float lo,float hi,float def){
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi),def));};
  add("rate","Sample Rate",3000.f,48000.f,11025.f);
  add("bits","Bit Depth",4.f,16.f,12.f);
  add("bandwidth","Bandwidth",400.f,20000.f,5500.f);
  add("drive","Saturation",0.f,1.f,0.2f);
  add("mix","Mix",0.f,1.f,1.f);
  add("output","Output", -18.f,12.f,0.f);
  return p;
}
bool MemoryProcessor::isBusesLayoutSupported(const BusesLayout& l) const {
  const auto out=l.getMainOutputChannelSet(), in=l.getMainInputChannelSet();
  return (out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo()) && in==out;
}
void MemoryProcessor::prepareToPlay(double sr,int) { engine.prepare(sr); }
void MemoryProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& m) {
  juce::ScopedNoDenormals noDenormals; m.clear();
  auto val=[&](const char* id){return state.getRawParameterValue(id)->load();};
  engine.set(val("rate"),(int)std::lround(val("bits")),val("bandwidth"),val("drive"),val("mix"));
  float* channels[2] = { b.getWritePointer(0), b.getNumChannels()>1 ? b.getWritePointer(1) : nullptr };
  engine.process(channels,b.getNumChannels(),b.getNumSamples());
  const float g=juce::Decibels::decibelsToGain(val("output"));
  b.applyGainRamp(0,b.getNumSamples(),lastGain<0?g:lastGain,g);
  lastGain=g;
}
void MemoryProcessor::setCurrentProgram(int index) {
  if(index<0||index>=getNumPrograms())return;
  struct Preset {float rate,bits,bw,drive,mix;};
  static constexpr Preset p[]={{11025,12,5500,.2f,1},{26040,12,11000,.3f,1},{8000,8,3200,.38f,1},{16000,10,6800,.65f,1},{4000,6,1800,.5f,1}};
  const auto& x=p[index];
  const char* ids[]={"rate","bits","bandwidth","drive","mix"};
  const float vals[]={x.rate,x.bits,x.bw,x.drive,x.mix};
  program=index;
  for(int i=0;i<5;++i) if(auto* param=state.getParameter(ids[i]))
    param->setValueNotifyingHost(param->convertTo0to1(vals[i]));
}
const juce::String MemoryProcessor::getProgramName(int i) {
  static const char* names[]={"DUST LOOP","WARM CONVERTER","8-BIT BASEMENT","HOT SAMPLER","BROKEN MEMORY"};
  return i>=0 && i<5 ? names[i] : "";
}
void MemoryProcessor::getStateInformation(juce::MemoryBlock& dest) {
  auto t=state.copyState();t.setProperty("program",program.load(),nullptr);
  if(auto x=t.createXml())copyXmlToBinary(*x,dest);
}
void MemoryProcessor::setStateInformation(const void* data,int bytes) {
  if(auto xml=getXmlFromBinary(data,bytes))
    if(xml->hasTagName(state.state.getType())) {
      auto tree=juce::ValueTree::fromXml(*xml);
      program=(int)tree.getProperty("program",0);
      state.replaceState(tree);
    }
}
juce::AudioProcessorEditor* MemoryProcessor::createEditor(){return new MemoryEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MemoryProcessor();}
