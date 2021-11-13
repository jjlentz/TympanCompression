#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"
#include <FlexiTimer2.h>

extern Tympan myTympan;

extern AudioSDWriter_F32_UI audioSDWriter;
extern State myState;
extern void setConfiguration(void);
extern float incrementInputGain(float);
extern void writeTextToSD(String text);
extern void doWhatever(void);
extern void doSlowCompression(void);
extern void doFastCompression(void);
extern void logCompressionPreference(char c);


class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void startTimer(void);
    void printHelp(void);
    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void);
    bool processCharacter(char c);  // called by SerialManagerBase.respondToByte(char c)

    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGUI_inputGain(bool activeButtonsOnly = false);


    int gainIncrement_dB = 2.5;
    
  private:
    TympanRemoteFormatter myGUI;
    void printGainLevels(String change);
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
    case 'A':
      incrementInputGain(-gainIncrement_dB);
      printGainLevels("Decrease");
      break;
    case 'a':
      // write to the SD card that 'A' was the preferred selection
      writeTextToSD("Comparison A vs B: winner A");
      break;
    case 'B':
      incrementInputGain(gainIncrement_dB);
      printGainLevels("Increase");
      break;
    case 'b':
      // write to the SD card that 'B' was the preferred selection
      writeTextToSD("Comparison A vs B: winner B");
      break;
    case 'S':
      doSlowCompression();
      break;
    case 'F':
      doFastCompression();
      break;
    case 's':
       // falls through on purpose
    case 'f':
      logCompressionPreference(c);
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

          //Add a "-" digital gain button with the Label("-"); Command("A"); Internal ID ("minusButton"); and width (4)
          card_gain_handle->addButton("-", "A", "minusButton", 4);  //displayed string, command, button ID, button width (out of 12)
          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain indicator"); width (4).
          card_gain_handle->addButton("","","inpGain",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_gain_handle->addButton("+", "B", "plusButton", 4);   //displayed string, command, button ID, button width (out of 12)

          card_gain_handle->addButton("Less OK", 'a', "AisBest", 4);
          card_gain_handle->addButton("", "", "aSpaceb", 4);
          card_gain_handle->addButton("More OK", 'b', "BisBest", 4);

     //Add a second card under the first page
      card_comp_handle = page_handle->addCard("Change Settings");

          //Add a '1' button for slow compression and a '2' button for fast compression
          card_comp_handle->addButton("1", "S", "slowButton", 4);  //displayed string, command, button ID, button width (out of 12)
          card_comp_handle->addButton("", "", "aSpaceb", 4);
          card_comp_handle->addButton("2", "F", "fastButton", 4);   //displayed string, command, button ID, button width (out of 12)

          card_comp_handle->addButton("1 Better", 's', "1isBest", 4);
          card_comp_handle->addButton("", "", "aSpaceb", 4);
          card_comp_handle->addButton("2 Better", 'f', "2isBest", 4);

      //card_handle = audioSDWriter.addCard_sdRecord(page_handle);
      audioSDWriter.addCard_sdRecord(page_handle);

  //add some pre-defined pages to the GUI
  myGUI.addPredefinedPage("serialMonitor");

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
  updateGUI_inputGain(activeButtonsOnly);

  SerialManagerBase::setFullGUIState(activeButtonsOnly);
}

void SerialManager::updateGUI_inputGain(bool activeButtonsOnly) {
  setButtonText("inpGain", String(myState.input_gain_dB, 1));
}

  //Print gain levels
void SerialManager::printGainLevels(String change) {
    writeTextToSD(String(millis()) + ", Input_Gain_dB: " + String(myState.input_gain_dB) + " " + change);
    Serial.print(change + " Analog Input Gain = " + String(myState.input_gain_dB) + " dB");
    Serial.println(myState.input_gain_dB); //print text to Serial port for debugging
    Serial.print("Digital Gain = " + String(myState.digital_gain_dB) + " dB");
    updateGUI_inputGain(true);
}

#endif
