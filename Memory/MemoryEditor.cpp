#include "MemoryEditor.h"
using namespace ae8brand;
MemoryEditor::MemoryEditor(MemoryProcessor& p):AudioProcessorEditor(&p),processor(p) {
 setLookAndFeel(&look);
 const char* ids[]={"rate","bits","bandwidth","drive","mix","output"};
 const char* names[]={"SAMPLE RATE","BIT DEPTH","BANDWIDTH","SATURATION","MIX","OUTPUT"};
 const int colours[]={6,4,2,5,2,3};
 for(int i=0;i<6;++i) {
  auto c=std::make_unique<Control>();
  c->title.setText(names[i],juce::dontSendNotification);
  c->title.setJustificationType(juce::Justification::centred);
  c->title.setFont(mono(10,true));
  c->title.setColour(juce::Label::textColourId,kMuted);
  c->knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  c->knob.setTextBoxStyle(juce::Slider::TextBoxBelow,false,95,20);
  c->knob.setColour(juce::Slider::rotarySliderFillColourId,flux(colours[i]));
  c->knob.setDoubleClickReturnValue(true, i==0?11025:i==1?12:i==2?5500:i==3?.2:i==4?1:0);
  addAndMakeVisible(c->knob); addAndMakeVisible(c->title);
  c->attachment=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,ids[i],c->knob);
  knobs[i]=std::move(c);
 }
 for(int i=0;i<processor.getNumPrograms();++i) programs.addItem(juce::String(i+1).paddedLeft('0',2)+"  "+processor.getProgramName(i),i+1);
 programs.setSelectedId(1,juce::dontSendNotification);
 programs.onChange=[this]{if(programs.getSelectedId()>0)processor.setCurrentProgram(programs.getSelectedId()-1);};
 prev.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+processor.getNumPrograms()-1)%processor.getNumPrograms());programs.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);};
 next.onClick=[this]{processor.setCurrentProgram((processor.getCurrentProgram()+1)%processor.getNumPrograms());programs.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);};
 addAndMakeVisible(programs); addAndMakeVisible(prev); addAndMakeVisible(next);
 scaler.init(*this,1180,650);
}
MemoryEditor::~MemoryEditor(){setLookAndFeel(nullptr);}
void MemoryEditor::paint(juce::Graphics& g) {
 g.addTransform(juce::AffineTransform::scale(scaler.scale(*this)));
 drawWindow(g,scaler.design());
 drawHeader(g,header,"MEMORY","M-01 / VINTAGE SAMPLING PROCESSOR");
 drawSection(g,display,"DIGITAL MEMORY","SAMPLE / CONVERSION",flux(2),false);
 drawSection(g,conversion,"CONVERSION","SAMPLE RATE / QUANTIZATION",flux(4),false);
 drawSection(g,colour,"CHARACTER","DRIVE / BLEND",flux(5),false);
 drawSection(g,output,"OUTPUT","OUT",flux(2),false);
 auto a=display.reduced(24,35).toFloat();
 drawGlass(g,a,6);
 g.setColour(kMuted);
 g.setFont(mono(12,true));
 g.drawText("ORIGINAL",a.removeFromTop(25),juce::Justification::centredLeft);
 const auto rate=processor.state.getRawParameterValue("rate")->load();
 const auto bits=(int)std::lround(processor.state.getRawParameterValue("bits")->load());
 const auto bw=processor.state.getRawParameterValue("bandwidth")->load();
 g.setColour(flux(2));g.setFont(mono(21,true));
 g.drawText(juce::String(rate/1000.f,2)+" kHz  /  "+juce::String(bits)+" BIT  /  "+juce::String(bw/1000.f,1)+" kHz BANDWIDTH",a.reduced(15,4),juce::Justification::centred);
 g.setColour(kMuted);g.setFont(mono(11));
 g.drawText("ORIGINAL DIGITAL SAMPLER COLORATION  -  NOT HARDWARE EMULATION",display.reduced(24,8).removeFromBottom(24),juce::Justification::centred);
}
void MemoryEditor::resized() {
 const int gap=10;
 auto a=scaler.design().reduced(gap);
 header=a.removeFromTop(64);a.removeFromTop(gap);
 auto pr=header.reduced(12,14);
 next.setBounds(pr.removeFromRight(34));pr.removeFromRight(5);
 prev.setBounds(pr.removeFromRight(34));pr.removeFromRight(8);
 programs.setBounds(pr.removeFromRight(295));
 display=a.removeFromTop(260);a.removeFromTop(gap);
 auto b=a.removeFromTop(280);
 conversion=b.removeFromLeft(590);b.removeFromLeft(gap);
 colour=b.removeFromLeft(370);b.removeFromLeft(gap);
 output=b;
 const juce::Rectangle<int> blocks[]={conversion,conversion,conversion,colour,colour,output};
 const int counts[]={3,3,3,2,2,1};
 const int positions[]={0,1,2,0,1,0};
 for(int i=0;i<6;++i){
  auto r=blocks[i].withTrimmedTop(34).reduced(12,10);
  const int cellw=r.getWidth()/counts[i];
  r=r.withX(r.getX()+cellw*positions[i]).withWidth(cellw);
  knobs[i]->title.setBounds(r.removeFromTop(25));
  knobs[i]->knob.setBounds(r.reduced(5,8));
 }
 scaler.apply(*this);
}
