#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

extern Tympan myTympan;
extern BLE ble;
extern AudioSDWriter_F32 audioSDWriter;
extern const float input_gain_dB;
extern float digital_gain_dB;
extern AudioEffectGain_F32 gainL;
extern AudioEffectGain_F32 gainR;
extern TympanRemoteFormatter myGUI;

extern void createTympanRemoteLayout(void);

class SerialManager {
  public:
    SerialManager(void) {  };

    void respondToByte(char c);
    void printTympanRemoteLayout(void);
    void changeGain(float change_in_gain_dB);
    void printGainLevels(void);
    void setButtonText(String btnId, String text);
    void setButtonState(String btnId, boolean newState);

  private:
    
};

  
void SerialManager::respondToByte(char c) {
    switch (c) {
      case 'J': 
      case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 'k':
      changeGain(3.0);
      printGainLevels();
      setButtonText("gainIndicator", String(digital_gain_dB));
      break;
    case 'K':
      changeGain(-3.0);
      printGainLevels();
      setButtonText("gainIndicator", String(digital_gain_dB));
      break;
    case 'R':
       myTympan.println("Received: begin SD recording");
       audioSDWriter.prepareSDforRecording();
       audioSDWriter.startRecording();
       setButtonState("recordOnButton", true);
       break;
    case 'Q':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      setButtonState("recordOnButton", false);
      break;
    }
}

  // Print the layout for the Tympan Remote app, in a JSON-ish string
  // (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
  // Please don't put commas or colons in your ID strings!
void SerialManager::printTympanRemoteLayout(void) {
    if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
    Serial.println(myGUI.asString());
    ble.sendMessage(myGUI.asString());
    setButtonText("gainIndicator", String(digital_gain_dB));
}

  //change the gain from the App
void SerialManager::changeGain(float change_in_gain_dB) {
    digital_gain_dB = digital_gain_dB + change_in_gain_dB;
    gainL.setGain_dB(digital_gain_dB);  //set the gain of the Left-channel gain processor
    gainR.setGain_dB(digital_gain_dB);  //set the gain of the Right-channel gain processor
}


  //Print gain levels 
void SerialManager::printGainLevels(void) {
    Serial.print("Analog Input Gain (dB) = "); 
    Serial.println(input_gain_dB); //print text to Serial port for debugging
    Serial.print("Digital Gain (dB) = "); 
    Serial.println(digital_gain_dB); //print text to Serial port for debugging
}

void SerialManager::setButtonText(String btnId, String text) {
    String str = "TEXT=BTN:" + btnId + ":"+text;
    Serial.println(str);
    ble.sendMessage(str);
}

void SerialManager::setButtonState(String btnId, boolean newState) {
    String str = "STATE=BTN:" + btnId + ":";
    str = newState ? str + "1" : str + "0";
    myTympan.println(str);
    ble.sendMessage(str);
}

#endif
