#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"
#include <FlexiTimer2.h>

extern Tympan myTympan;

extern AudioSDWriter_F32_UI audioSDWriter;
extern State myState;
extern EarpieceMixer_F32_UI earpieceMixer;

//extern void setConfiguration(void);
//extern float incrementInputGain(float);
extern void writeTextToSD(String text);
extern void doWhatever(void);
extern void updateOutputGain(float);
extern void printOutputGain(void);
extern void setupMyCompressors(float gain, float attack, float release);


class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void startTimer(void);
    void printHelp(void);
    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void);
    bool processCharacter(char c);  // called by SerialManagerBase.respondToByte(char c)

    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGainDisplay(void);
    void printGainLevels(String change);
    
  private:
    TympanRemoteFormatter myGUI;
    void doFast(void);
    void doSlow(void);
};

void SerialManager::startTimer(void) {
  FlexiTimer2::set(60, 1, doWhatever);
  FlexiTimer2::start();
}

void SerialManager::printHelp(void) {
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("  tbd");
  SerialManagerBase::printHelp();
  Serial.println();
}

bool SerialManager::processCharacter(char c) {
  bool ret_val = true;
  switch (c) {
    case 'h':
      printHelp();
      break;
    case 'J':
    case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 't':
      updateOutputGain(1.0);     //raise
      printOutputGain();  //print to USB Serial (ie, to the SerialMonitor)
      updateGainDisplay();  //update the App
      break;
    case 'T':
      updateOutputGain(-1.0); //lower
      printOutputGain();  //print to USB Serial (ie, to the SerialMonitor)
      updateGainDisplay();  //update the App
      break;
    case 'F':
      doFast();
      break;
    case 'S':
      doSlow();
      break;  
    case 'f':
      // write to the SD card that 'F' was the preferred selection
      writeTextToSD("Comparison Fast vs Slow: winner FAST");
      break;
    case 's':
      // write to the SD card that 'S' was the preferred selection
      writeTextToSD("Comparison Fast vs Slow: winner SLOW");
      break;
    default:
      ret_val = SerialManagerBase::processCharacter(c);
      break;
    }
    return ret_val;
}

void SerialManager::createTympanRemoteLayout(void) {
  TR_Page *page_handle;
  TR_Card *card_gain_handle;
  TR_Card *card_comp_handle;

    //Add first page to GUI
  page_handle = myGUI.addPage("Perception of Compression Params");
      //Add a card under the first page
      card_gain_handle = page_handle->addCard("Change Loudness");

      card_gain_handle->addButton("-", "T", "minusGain", 4);  //displayed string, command, button ID, button width (out of 12)
      card_gain_handle->addButton("", "","outputGain", 4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      card_gain_handle->addButton("+", "t", "plusGain", 4);   //displayed string, command, button ID, button width (out of 12)

     //Add a second card under the first page
      card_comp_handle = page_handle->addCard("Prescription Selection");

      card_comp_handle->addButton("A", "F", "fast", 4);  //displayed string, command, button ID, button width (out of 12)
      card_comp_handle->addButton("", "", "space", 4);
      card_comp_handle->addButton("B", "S", "slow", 4);   //displayed string, command, button ID, button width (out of 12)

      card_comp_handle->addButton("A Better", 's', "fastIsBest", 4);
      card_comp_handle->addButton("", "", "anotherSpace", 4);
      card_comp_handle->addButton("B Better", 'f', "slowIsBest", 4);

      //card_handle = audioSDWriter.addCard_sdRecord(page_handle);
      audioSDWriter.addCard_sdRecord(page_handle);

  //add some pre-defined pages to the GUI
  // myGUI.addPredefinedPage("serialMonitor");

}

  // Print the layout for the Tympan Remote app, in a JSON-ish string
  // (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
  // Please don't put commas or colons in your ID strings!
void SerialManager::printTympanRemoteLayout(void) {
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    String gui = myGUI.asString();
    Serial.println(gui);
    ble->sendMessage(gui);
    setFullGUIState();
}

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  updateGainDisplay();

  SerialManagerBase::setFullGUIState(activeButtonsOnly);
}

void SerialManager::updateGainDisplay(void) {
  setButtonText("outputGain", String(myState.output_gain_dB,0));
}

  //Print gain levels
void SerialManager::printGainLevels(String change) {
    writeTextToSD(String(millis()) + ", Output_Gain_dB: " + String(myState.output_gain_dB) + " " + change);
    Serial.println(myState.output_gain_dB); //print text to Serial port for debugging
}

void SerialManager::doFast() {
  setupMyCompressors(0.0, myState.FAST_ATTACK, myState.FAST_RELEASE);
  myState.activeAlgorithm = myState.FAST;
  setButtonState("fast", true, true);
  setButtonState("slow", false, true);
}

void SerialManager::doSlow() {
  setupMyCompressors(25.5, myState.SLOW_ATTACK, myState.SLOW_RELEASE);
  myState.activeAlgorithm = myState.SLOW;
  setButtonState("slow", true, true);
  setButtonState("fast", false, false);
}
#endif
