/*
 * MIT License.  use at your own risk.
 */
#include <AudioEffectEmpty_F32.h>
#include <Tympan_Library.h>
#include "SerialManager.h"

// TODO: are these next two things the right audio settings?
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);
#define MAX_AUDIO_MEM 60

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::E, audio_settings);     //do TympanRev::D or TympanRev::E
AudioInputI2S_F32         i2s_in(audio_settings);                     //Digital audio in *from* the Teensy Audio Board ADC.
AudioSDWriter_F32         audioSDWriter(audio_settings);
AudioEffectGain_F32       gainL;                      //Applies digital gain to audio data.  Left.
AudioEffectGain_F32       gainR;                      //Applies digital gain to audio data.  Right.
AudioOutputI2S_F32        i2s_out(audio_settings);                    //Digital audio out *to* the Teensy Audio Board DAC.

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, gainL, 0);    //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, gainR, 0);    //connect the Right input
//Connect to SD logging
AudioConnection_F32       patchCord3(i2s_in, 0, audioSDWriter, 0); //connect Raw audio to left channel of SD writer
AudioConnection_F32       patchCord4(i2s_in, 1, audioSDWriter, 1); //connect Raw audio to right channel of SD writer

AudioConnection_F32       patchCord11(gainL, 0, i2s_out, 0);  //connect the Left gain to the Left output
AudioConnection_F32       patchCord12(gainR, 0, i2s_out, 1);  //connect the Right gain to the Right output

//define class for handling the GUI on the app
TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App

//Create BLE
BLE ble = BLE(&Serial1);
SerialManager serialManager;


// define the setup() function, the function that is called once when the device is booting

const float input_gain_dB = 15.0f; // Pretty sure serves no purpose in our project and can be removed
float digital_gain_dB = 0.0;      //this will be set by the app
void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();delay(1000);
  Serial.println("BasicGain_wApp: starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(MAX_AUDIO_MEM, audio_settings);

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate the Tympan's audio module

  myTympan.enableDigitalMicInputs(false);
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

  //prepare the SD writer for the format that we want
  audioSDWriter.setSerial(&myTympan);
  //this is the default type. TODO any benefit from changing that to FLOAT32?
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);
  audioSDWriter.setNumWriteChannels(2);

  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  serviceSD();

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
   ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

} //end loop();


// ///////////////// Servicing routines


//define the GUI for the App
void createTympanRemoteLayout(void) {
  
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  TR_Card *card_record_handle;  //dummy handle for a card


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
      card_record_handle = page_h->addCard("RECORD COMMENTS");
          card_record_handle->addButton("ON", "R", "recordOnButton", 4);
          card_record_handle->addButton("Record", "", "recordingindicator", 4);
          card_record_handle->addButton("OFF", "Q", "recordOff", 4);
        
  //add some pre-defined pages to the GUI
  myGUI.addPredefinedPage("serialMonitor");
  //myGUI.addPredefinedPage("serialPlotter");
}

void serviceSD(void) {
  static int max_max_bytes_written = 0; //for timing diagnotstics
  static int max_bytes_written = 0; //for timing diagnotstics
  static int max_dT_micros = 0; //for timing diagnotstics
  static int max_max_dT_micros = 0; //for timing diagnotstics

  unsigned long dT_micros = micros();  //for timing diagnotstics
  int bytes_written = audioSDWriter.serviceSD();
  dT_micros = micros() - dT_micros;  //timing calculation

  if ( bytes_written > 0 ) {

    max_bytes_written = max(max_bytes_written, bytes_written);
    max_dT_micros = max((int)max_dT_micros, (int)dT_micros);

    if (dT_micros > 10000) {  //if the write took a while, print some diagnostic info

      max_max_bytes_written = max(max_bytes_written,max_max_bytes_written);
      max_max_dT_micros = max(max_dT_micros, max_max_dT_micros);

      Serial.print("serviceSD: bytes written = ");
      Serial.print(bytes_written); Serial.print(", ");
      Serial.print(max_bytes_written); Serial.print(", ");
      Serial.print(max_max_bytes_written); Serial.print(", ");
      Serial.print("dT millis = ");
      Serial.print((float)dT_micros/1000.0,1); Serial.print(", ");
      Serial.print((float)max_dT_micros/1000.0,1); Serial.print(", ");
      Serial.print((float)max_max_dT_micros/1000.0,1);Serial.print(", ");
      Serial.println();
      max_bytes_written = 0;
      max_dT_micros = 0;
    }
    //print a warning if there has been an SD writing hiccup
    if (i2s_in.get_isOutOfMemory()) {
      float approx_time_sec = ((float)(millis()-audioSDWriter.getStartTimeMillis()))/1000.0;
      if (approx_time_sec > 0.1) {
        myTympan.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
        myTympan.println(approx_time_sec );
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}
