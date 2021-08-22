/*
 * MIT License.  use at your own risk.
 */#include <AudioEffectEmpty_F32.h>
#include <Tympan_Library.h>
 
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::E, audio_settings);     //do TympanRev::D or TympanRev::E
AudioInputI2S_F32         i2s_in;                     //Digital audio in *from* the Teensy Audio Board ADC.
AudioEffectGain_F32       gainL;                      //Applies digital gain to audio data.  Left.
AudioEffectGain_F32       gainR;                      //Applies digital gain to audio data.  Right.
AudioOutputI2S_F32        i2s_out;                    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gainL, 0);    //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gainR, 0);    //connect the Right input
AudioConnection_F32       patchCord11(gainL, 0, i2s_out, 0);  //connect the Left gain to the Left output
AudioConnection_F32       patchCord12(gainR, 0, i2s_out, 1);  //connect the Right gain to the Right output

//define class for handling the GUI on the app
TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App

//Create BLE
BLE ble = BLE(&Serial1);


// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 10.0f; //gain on the microphone
float digital_gain_dB = 0.0;      //this will be set by the app
void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(1000);
  Serial.println("BasicGain_wApp: starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate the Tympan's audio module

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps
  gainL.setGain_dB(digital_gain_dB);       //set the digital gain of the Left-channel
  gainR.setGain_dB(digital_gain_dB);       //set the digital gainof the Right-channel gain processor  

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!

  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
   ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

} //end loop();


// ///////////////// Servicing routines

//respond to serial commands
void respondToByte(char c) {
  Serial.print("Received character "); Serial.println(c);
  
  switch (c) {
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
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
  }
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// Please don't put commas or colons in your ID strings!
void printTympanRemoteLayout(void) {
  if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
  Serial.println(myGUI.asString());
  ble.sendMessage(myGUI.asString());
  setButtonText("gainIndicator", String(digital_gain_dB));
}

//define the GUI for the App
void createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI
  page_h = myGUI.addPage("Currently Just a Copy of Gain Demo");
      //Add a card under the first page
      card_h = page_h->addCard("Change Loudness");
          //Add a "-" digital gain button with the Label("-"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("-","K","minusButton",4);  //displayed string, command, button ID, button width (out of 12)

          //Add an indicator that's a button with no command:  Label (value of the digital gain); Command (""); Internal ID ("gain indicator"); width (4).
          card_h->addButton("","","gainIndicator",4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  
          //Add a "+" digital gain button with the Label("+"); Command("K"); Internal ID ("minusButton"); and width (4)
          card_h->addButton("+","k","plusButton",4);   //displayed string, command, button ID, button width (out of 12)
        
  //add some pre-defined pages to the GUI
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}


//change the gain from the App
void changeGain(float change_in_gain_dB) {
  digital_gain_dB = digital_gain_dB + change_in_gain_dB;
  gainL.setGain_dB(digital_gain_dB);  //set the gain of the Left-channel gain processor
  gainR.setGain_dB(digital_gain_dB);  //set the gain of the Right-channel gain processor
}


//Print gain levels 
void printGainLevels(void) {
  Serial.print("Analog Input Gain (dB) = "); 
  Serial.println(input_gain_dB); //print text to Serial port for debugging
  Serial.print("Digital Gain (dB) = "); 
  Serial.println(digital_gain_dB); //print text to Serial port for debugging
}

void setButtonText(String btnId, String text) {
  String str = "TEXT=BTN:" + btnId + ":"+text;
  Serial.println(str);
  ble.sendMessage(str);
}