/*
 * MIT License.  use at your own risk.
 */
#include <Tympan_Library.h>
#include <SD.h>  // for text logging to the SD card

const float sample_rate_Hz = 44100.0f; 
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);
#define MAX_AUDIO_MEM 100

Tympan myTympan(TympanRev::E); 
SDClass sdx;  // explicitly create SD card, which we will pass to AudioSDWriter *and* which we will use for text logging
String experiment_log_filename = "ExperimentLog.txt";
AudioSynthWaveform_F32    sineWave(audio_settings); 

AudioInputI2S_F32    i2s_in(audio_settings);                     //Digital audio in *from* the Teensy Audio Board ADC.
AudioSDWriter_F32_UI audioSDWriter(&(sdx.sdfs), audio_settings);
AudioOutputI2S_F32   i2s_out(audio_settings);                    //Digital audio out *to* the Teensy Audio Board DAC.

//Make the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, i2s_out, 0);    //connect the Left input
AudioConnection_F32       patchCord2(i2s_in, 1, i2s_out, 1);    //connect the Right input
//Connect to SD logging
AudioConnection_F32       patchCord3(i2s_in, 0, audioSDWriter, 0); //connect Raw audio to left channel of SD writer
AudioConnection_F32       patchCord4(i2s_in, 1, audioSDWriter, 1); //connect Raw audio to right channel of SD writer

#include "SerialManager.h"
#include "State.h"
//Create BLE
BLE_UI ble(&Serial1);
SerialManager serialManager(&ble);
State myState(&audio_settings, &myTympan, &serialManager);

float digital_gain_dB = 0.0;      //this will be set by the app
void setupSerialManager() {
  serialManager.add_UI_element(&myState);
  serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
  serialManager.startTimer();
}

float setInputGain_dB(float val_dB) {
  return myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
}

void setConfiguration(void) {
  const float default_input_gain_dB = 15.0f;
  // could set some input source or something else with config
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  setInputGain_dB(default_input_gain_dB);

  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
}

void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();
  delay(1000);
  Serial.println("PerceptionOfCompressionParams: starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(MAX_AUDIO_MEM, audio_settings);

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate the Tympan's audio module
  myTympan.volume_dB(0.0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  myTympan.enableDigitalMicInputs(false);
  
  setConfiguration();

  delay(500);
  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!
  delay(500);

  setupSerialManager();

  //prepare the SD writer for the format that we want
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(2);

  if (!sdx.sdfs.begin(SdioConfig(FIFO_SDIO)))
    sdx.sdfs.errorHalt(&Serial, "setup: SD begin failed!");

  Serial.println("Setup complete.");
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //look for in-coming serial messages (via USB or via Bluetooth)
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle;
    int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) {
      serialManager.respondToByte(msgFromBle[i]);
    }
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)


  audioSDWriter.serviceSD_withWarnings(i2s_in);

  static int prev_SD_state = (int)audioSDWriter.getState();
  if ((int)audioSDWriter.getState() != prev_SD_state) {
    writeTextToSD(String(millis() + String(", audioSDWriter_State = ")
      + String((int)audioSDWriter.getState())));
  }
  prev_SD_state = (int)audioSDWriter.getState();

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(), audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING);

  //periodically print the CPU and Memory Usage
//  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
//  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.newExperimentTone > 0 && myState.toneStartTime == 0) {
    myState.toneStartTime = millis();
    AudioConnection_F32 patchCord101(sineWave, 0, i2s_out, 0);  //connect to left output
    AudioConnection_F32 patchCord102(sineWave, 0, i2s_out, 1);  //connect to right output
    sineWave.amplitude(0.03);
    myTympan.setAmberLED(HIGH);
  } else if (myState.toneStartTime > 0) {
    if ((millis() - myState.toneStartTime) >= myState.newExperimentTone) {
      AudioConnection_F32       patchCord1(i2s_in, 0, i2s_out, 0);    //connect the Left input
      AudioConnection_F32       patchCord2(i2s_in, 1, i2s_out, 1);
      myState.newExperimentTone = 0;
      myState.toneStartTime = 0;
      sineWave.amplitude(0.0f);
      myTympan.setAmberLED(LOW);
    }
  }

} //end loop();

void writeTextToSD(String results) {
  Serial.println("writeTextToSD: opening " + String(experiment_log_filename));
  FsFile resultsFile = sdx.sdfs.open(experiment_log_filename,  O_WRITE | O_CREAT | O_APPEND);
  Serial.println("writeTextToSD: writing: " + results);
  resultsFile.println(results);
  Serial.println("writeTextToSD: closing file.");
  resultsFile.close();
}

float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}

// Does it make sense to combine doSlowCompression and doFastCompression if we can parameterize?
void doSlowCompression(void) {
  Serial.println("Switching to the slow compression algorithm");
}

void doFastCompression(void) {
  Serial.println("Switching to the fast compression algorithm");
}
// what all needs to be logged for the Science?
void logCompressionPreference(char c) {
  // If all we need to do is write a simple selection, we can
  // delete this function and call WriteTextToSD
  // But if we have a large structure to log...
  writeTextToSD("The user selected a compression preference for option " + String(c));
}

void doWhatever(void) {
  writeTextToSD(String(millis()) + ", Time for a new experiment.");
  myState.newExperimentTone = 10000;
}
