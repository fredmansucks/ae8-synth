#include <JuceHeader.h>
#include "../Shared/Brand.h"
#include <algorithm>
#include <vector>

// AE-8 CRATE v0.2 — local sample explorer with virtual crates and OS file drag.
// Indexes paths without moving/copying the user's source audio.
// A real Windows file drag, not an internal JUCE drag; DAWs can receive the path.
class SampleDragButton final : public juce::TextButton {
public:
 juce::File dragFile;
 void mouseDown(const juce::MouseEvent& e) override {
   started=false;
   juce::TextButton::mouseDown(e);
 }
 void mouseDrag(const juce::MouseEvent& e) override {
   if(started || !e.mouseWasDraggedSinceMouseDown() || !dragFile.existsAsFile())return;
   started=true;
   juce::StringArray paths;
   paths.add(dragFile.getFullPathName());
   juce::DragAndDropContainer::performExternalDragDropOfFiles(paths,false,this,{});
 }
private:
 bool started=false;
};

class CrateApp final : public juce::AudioAppComponent,
                       public juce::ListBoxModel,
                       public juce::FileDragAndDropTarget,
                       public juce::ChangeListener {
public:
 CrateApp() : cache(32), thumbnail(256, formats, cache) {
   using namespace ae8brand;
   setLookAndFeel(&look);
   formats.registerBasicFormats();
   search.setTextToShowWhenEmpty("SEARCH SAMPLES, FILES OR PATHS", kMuted);
   search.onTextChange=[this]{ refresh(); };
   addAndMakeVisible(search);
   folder.setButtonText("OPEN FOLDER");
   folder.onClick=[this]{
     chooser=std::make_unique<juce::FileChooser>("Choose your sample folder");
     chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser& c){const auto f=c.getResult(); if(f.isDirectory())scan(f);});
   };
   addAndMakeVisible(folder);
   favourites.setButtonText("FAVORITES ONLY");
   favourites.setClickingTogglesState(true);
   favourites.onClick=[this]{refresh();};
   addAndMakeVisible(favourites);
   blind.setButtonText("BLIND DIG");
   blind.setClickingTogglesState(true);
   blind.onClick=[this]{list.repaint();};
   addAndMakeVisible(blind);
   audition.setButtonText("PLAY / PAUSE");
   audition.onClick=[this]{ if(transport.isPlaying())transport.stop(); else transport.start(); };
   addAndMakeVisible(audition);
   star.setButtonText("TOGGLE FAVORITE");
   star.onClick=[this]{if(!selected.existsAsFile())return;const auto k=selected.getFullPathName(); if(favoritePaths.contains(k))favoritePaths.removeString(k);else favoritePaths.add(k);saveFavorites();refresh();};
   addAndMakeVisible(star);
   showLocation.setButtonText("REVEAL FILE");
   showLocation.onClick=[this]{if(selected.existsAsFile())selected.revealToUser();};
   addAndMakeVisible(showLocation);
   dragOut.setButtonText("DRAG SAMPLE TO ABLETON");
   dragOut.setTooltip("Click and drag this control onto an Ableton audio track, Simpler or FLIP.");
   addAndMakeVisible(dragOut);
   newCrateName.setTextToShowWhenEmpty("NAME A NEW CRATE",kMuted);
   addAndMakeVisible(newCrateName);
   createCrate.setButtonText("CREATE CRATE");
   createCrate.onClick=[this]{
     const auto name=newCrateName.getText().trim();
     if(name.isEmpty())return;
     for(const auto& c:crates)if(c.name.equalsIgnoreCase(name)){
       status.setText("CRATE NAME ALREADY EXISTS",juce::dontSendNotification);return;
     }
     crates.push_back({name,{}});
     saveCrates();rebuildCrateSelector();
     crateSelect.setSelectedId((int)crates.size()+1,juce::sendNotification);
     newCrateName.clear();
   };
   addAndMakeVisible(createCrate);
   crateSelect.onChange=[this]{refresh();};
   addAndMakeVisible(crateSelect);
   addToCrate.setButtonText("+ ADD SELECTED");
   addToCrate.onClick=[this]{
     const int index=crateSelect.getSelectedId()-2;
     if(index<0||index>=(int)crates.size()||!selected.existsAsFile())return;
     auto& paths=crates[(size_t)index].paths;
     const auto p=selected.getFullPathName();
     if(!paths.contains(p))paths.add(p);
     saveCrates();refresh();
   };
   addAndMakeVisible(addToCrate);
   removeFromCrate.setButtonText("- REMOVE");
   removeFromCrate.onClick=[this]{
     const int index=crateSelect.getSelectedId()-2;
     if(index<0||index>=(int)crates.size()||!selected.existsAsFile())return;
     crates[(size_t)index].paths.removeString(selected.getFullPathName());
     saveCrates();refresh();
   };
   addAndMakeVisible(removeFromCrate);
   deleteCrate.setButtonText("DELETE CRATE");
   deleteCrate.setTooltip("Deletes the active virtual crate only; source files are never deleted.");
   deleteCrate.onClick=[this]{
     const int index=crateSelect.getSelectedId()-2;
     if(index<0||index>=(int)crates.size())return;
     crates.erase(crates.begin()+index);
     saveCrates();rebuildCrateSelector();
     crateSelect.setSelectedId(1,juce::sendNotification);
   };
   addAndMakeVisible(deleteCrate);
   loadCrates();
   rebuildCrateSelector();
   list.setModel(this);list.setRowHeight(35);list.setColour(juce::ListBox::backgroundColourId,kGlass);
   addAndMakeVisible(list);
   status.setColour(juce::Label::textColourId,kMuted);status.setFont(mono(12));
   status.setJustificationType(juce::Justification::centredLeft);
   addAndMakeVisible(status);
   thumbLabel.setColour(juce::Label::textColourId,kInk);thumbLabel.setFont(mono(14,true));
   addAndMakeVisible(thumbLabel);
   thumbnail.addChangeListener(this);
   favoritePaths.addLines(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
       .getChildFile("AE-8").getChildFile("crate-favorites.txt").loadFileAsString());
   setSize(1200,760);setAudioChannels(0,2);
   status.setText("CHOOSE A FOLDER TO INDEX YOUR SAMPLES",juce::dontSendNotification);
 }
 ~CrateApp() override {
   thumbnail.removeChangeListener(this);transport.stop();transport.setSource(nullptr);
   source.reset();shutdownAudio();setLookAndFeel(nullptr);
 }
 void prepareToPlay(int block,double rate) override {transport.prepareToPlay(block,rate);}
 void getNextAudioBlock(const juce::AudioSourceChannelInfo& out) override {transport.getNextAudioBlock(out);}
 void releaseResources() override {transport.releaseResources();}
 int getNumRows() override {return (int)visible.size();}
 void paintListBoxItem(int row,juce::Graphics& g,int width,int height,bool selectedRow) override {
   if(row<0||row>=(int)visible.size())return;
   auto f=files[(size_t)visible[(size_t)row]];
   if(selectedRow){g.setColour(ae8brand::flux(2).withAlpha(.20f));g.fillRect(0,0,width,height);}
   g.setColour(ae8brand::kInk);g.setFont(ae8brand::mono(13,true));
   const auto title=blind.getToggleState()?"UNKNOWN RECORDING  "+juce::String(row+1):f.getFileName();
   g.drawText(title,12,0,width-160,height,juce::Justification::centredLeft,true);
   g.setColour(ae8brand::kMuted);g.setFont(ae8brand::mono(10));
   const bool fav=favoritePaths.contains(f.getFullPathName());
   g.drawText(fav?"FAVORITE  *":"AUDIO",width-145,0,130,height,juce::Justification::centredRight);
 }
 void selectedRowsChanged(int row) override {
   if(row<0||row>=(int)visible.size())return;
   selected=files[(size_t)visible[(size_t)row]];
   dragOut.dragFile=selected;
   playFile(selected);
 }
 bool isInterestedInFileDrag(const juce::StringArray&) override {return true;}
 void filesDropped(const juce::StringArray& paths,int,int) override {
   if(paths.isEmpty())return;
   auto f=juce::File(paths[0]);
   if(f.isDirectory())scan(f);
   else if(f.existsAsFile())scan(f.getParentDirectory());
 }
 void changeListenerCallback(juce::ChangeBroadcaster*) override {repaint(waveArea);}
 void paint(juce::Graphics& g) override {
   using namespace ae8brand;
   drawWindow(g,getLocalBounds());
   drawHeader(g,header,"CRATE","C-01 / SAMPLE LIBRARY EXPLORER");
   drawSection(g,libraryArea,"LIBRARY","BROWSE / SEARCH / PERSONAL CRATES",flux(2),false);
   drawSection(g,previewArea,"PREVIEW","AUDIO / WAVEFORM",flux(4),false);
   if(selected.existsAsFile()){
     const auto area=waveArea.toFloat();
     drawGlass(g,area,5);
     g.setColour(flux(2));
     thumbnail.drawChannels(g,waveArea.reduced(8,5),0.0,thumbnail.getTotalLength(),0.9f);
   }
   g.setColour(kMuted);g.setFont(mono(11));
   g.drawText("AE-8 CRATE v0.2 / DRAG SAMPLE TO ABLETON / SOURCE FILES NEVER MOVE",getLocalBounds().removeFromBottom(24).reduced(16,0),juce::Justification::centredLeft);
 }
 void resized() override {
   auto b=getLocalBounds().reduced(12);
   header=b.removeFromTop(68);b.removeFromTop(10);
   libraryArea=b.removeFromTop(b.getHeight()*2/3);b.removeFromTop(10);
   previewArea=b.removeFromTop(170);
   auto top=libraryArea.withTrimmedTop(29).reduced(12,8);
   auto tools=top.removeFromTop(42);
   folder.setBounds(tools.removeFromLeft(140));tools.removeFromLeft(10);
   search.setBounds(tools.removeFromLeft(440));tools.removeFromLeft(10);
   favourites.setBounds(tools.removeFromLeft(145));tools.removeFromLeft(10);
   blind.setBounds(tools.removeFromLeft(130));
   top.removeFromTop(6);
   auto crateBar=top.removeFromTop(38);
   crateSelect.setBounds(crateBar.removeFromLeft(180));crateBar.removeFromLeft(8);
   newCrateName.setBounds(crateBar.removeFromLeft(170));crateBar.removeFromLeft(8);
   createCrate.setBounds(crateBar.removeFromLeft(115));crateBar.removeFromLeft(8);
   addToCrate.setBounds(crateBar.removeFromLeft(135));crateBar.removeFromLeft(8);
   removeFromCrate.setBounds(crateBar.removeFromLeft(90));crateBar.removeFromLeft(8);
   deleteCrate.setBounds(crateBar.removeFromLeft(118));
   top.removeFromTop(8);list.setBounds(top);
   auto p=previewArea.withTrimmedTop(27).reduced(12,8);
   thumbLabel.setBounds(p.removeFromTop(23));
   waveArea=p.removeFromTop(67);
   p.removeFromTop(6);
   audition.setBounds(p.removeFromLeft(135));p.removeFromLeft(8);
   star.setBounds(p.removeFromLeft(160));p.removeFromLeft(8);
   showLocation.setBounds(p.removeFromLeft(120));p.removeFromLeft(8);
   dragOut.setBounds(p.removeFromLeft(200));p.removeFromLeft(12);
   status.setBounds(p);
 }
private:
 void scan(const juce::File& dir) {
   scannedFiles.clear();
   juce::Array<juce::File> found;
   dir.findChildFiles(found,juce::File::findFiles,true,"*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
   // JUCE wildcard matching is platform-dependent; verify extension as well.
   for(const auto& f:found)scannedFiles.push_back(f);
   std::sort(scannedFiles.begin(),scannedFiles.end(),[](const auto& a,const auto& b){return a.getFileName().compareIgnoreCase(b.getFileName())<0;});
   currentFolder=dir;refresh();
 }
 void refresh() {
   visible.clear();
   files=scannedFiles;
   const int crateIndex=crateSelect.getSelectedId()-2;
   if(crateIndex>=0&&crateIndex<(int)crates.size()){
     for(const auto& path:crates[(size_t)crateIndex].paths){
       const juce::File candidate(path);
       if(!candidate.existsAsFile())continue;
       const auto it=std::find_if(files.begin(),files.end(),[&](const juce::File& f){return f==candidate;});
       if(it==files.end())files.push_back(candidate);
     }
   }
   const auto q=search.getText().trim();
   for(int i=0;i<(int)files.size();++i){
     const auto& f=files[(size_t)i];
     if(q.isNotEmpty()&&!f.getFullPathName().containsIgnoreCase(q))continue;
     if(favourites.getToggleState()&&!favoritePaths.contains(f.getFullPathName()))continue;
     if(crateIndex>=0&&crateIndex<(int)crates.size()
       &&!crates[(size_t)crateIndex].paths.contains(f.getFullPathName()))continue;
     visible.push_back(i);
   }
   list.updateContent();list.repaint();
   status.setText(juce::String(visible.size())+" / "+juce::String(files.size())+" SAMPLES",juce::dontSendNotification);
 }
 void playFile(const juce::File& file) {
   transport.stop();transport.setSource(nullptr);source.reset();
   std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(file));
   if(!reader){status.setText("UNSUPPORTED AUDIO FILE",juce::dontSendNotification);return;}
   const double sr=reader->sampleRate;
   source=std::make_unique<juce::AudioFormatReaderSource>(reader.release(),true);
   transport.setSource(source.get(),0,nullptr,sr);
   thumbnail.setSource(new juce::FileInputSource(file));
   thumbLabel.setText(file.getFileName(),juce::dontSendNotification);
   transport.start();repaint();
 }
 struct VirtualCrate {juce::String name; juce::StringArray paths;};
 juce::File cratesFile() const {
   return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
      .getChildFile("AE-8").getChildFile("crate-collections.xml");
 }
 void rebuildCrateSelector() {
   const auto old=crateSelect.getText();
   crateSelect.clear(juce::dontSendNotification);
   crateSelect.addItem("ALL SAMPLES",1);
   for(size_t i=0;i<crates.size();++i)
     crateSelect.addItem(crates[i].name,(int)i+2);
   int chosen=1;
   for(size_t i=0;i<crates.size();++i)
     if(crates[i].name==old)chosen=(int)i+2;
   crateSelect.setSelectedId(chosen,juce::dontSendNotification);
   refresh();
 }
 void loadCrates() {
   auto xml=juce::XmlDocument::parse(cratesFile());
   if(!xml)return;
   const auto tree=juce::ValueTree::fromXml(*xml);
   if(!tree.hasType("CRATES"))return;
   for(int i=0;i<tree.getNumChildren();++i){
     auto item=tree.getChild(i);
     if(!item.hasType("CRATE"))continue;
     VirtualCrate c;
     c.name=item.getProperty("name").toString();
     if(c.name.isEmpty())continue;
     for(int j=0;j<item.getNumChildren();++j){
       const auto path=item.getChild(j).getProperty("path").toString();
       if(path.isNotEmpty())c.paths.addIfNotAlreadyThere(path);
     }
     crates.push_back(std::move(c));
   }
 }
 void saveCrates() {
   juce::ValueTree tree("CRATES");
   for(const auto& c:crates){
     juce::ValueTree item("CRATE");
     item.setProperty("name",c.name,nullptr);
     for(const auto& path:c.paths){
       juce::ValueTree f("FILE");
       f.setProperty("path",path,nullptr);
       item.addChild(f,-1,nullptr);
     }
     tree.addChild(item,-1,nullptr);
   }
   const auto file=cratesFile();
   file.getParentDirectory().createDirectory();
   if(auto xml=tree.createXml())file.replaceWithText(xml->toString());
 }
 void saveFavorites() {
   auto dest=juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("AE-8").getChildFile("crate-favorites.txt");
   dest.getParentDirectory().createDirectory();
   dest.replaceWithText(favoritePaths.joinIntoString("\n"));
 }
 ae8brand::LookAndFeel look;
 juce::AudioFormatManager formats;
 juce::AudioThumbnailCache cache;
 juce::AudioThumbnail thumbnail;
 juce::AudioTransportSource transport;
 std::unique_ptr<juce::AudioFormatReaderSource> source;
 std::vector<juce::File> files,scannedFiles;
 std::vector<VirtualCrate> crates;
 std::vector<int> visible;
 juce::File selected,currentFolder;
 juce::StringArray favoritePaths;
 juce::ListBox list;
 juce::TextEditor search,newCrateName;
 juce::ComboBox crateSelect;
 juce::TextButton folder,favourites,blind,audition,star,showLocation;
 juce::TextButton createCrate,addToCrate,removeFromCrate,deleteCrate;
 SampleDragButton dragOut;
 juce::Label status,thumbLabel;
 juce::Rectangle<int> header,libraryArea,previewArea,waveArea;
 std::unique_ptr<juce::FileChooser> chooser;
};
class CrateApplication final : public juce::JUCEApplication {
public:
 const juce::String getApplicationName() override {return "AE-8 CRATE";}
 const juce::String getApplicationVersion() override {return "0.2";}
 void initialise(const juce::String&) override {
   window=std::make_unique<MainWindow>(getApplicationName());
 }
 void shutdown() override {window.reset();}
 class MainWindow final : public juce::DocumentWindow {
 public:
  explicit MainWindow(const juce::String& name):
   DocumentWindow(name,juce::Colour(0xff050608),DocumentWindow::allButtons){
   setUsingNativeTitleBar(true);
   setContentOwned(new CrateApp(),true);
   centreWithSize(getWidth(),getHeight());setVisible(true);
  }
  void closeButtonPressed() override {juce::JUCEApplication::getInstance()->systemRequestedQuit();}
 };
 std::unique_ptr<MainWindow> window;
};
START_JUCE_APPLICATION(CrateApplication)
