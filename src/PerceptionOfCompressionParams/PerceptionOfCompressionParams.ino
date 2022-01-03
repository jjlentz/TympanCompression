/*
 * MIT License.  use at your own risk.
 */
#include <Tympan_Library.h>
#include <SD.h>  // for text logging to the SD card

const float sample_rate_Hz = 24000.0f; 
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);
#define MAX_AUDIO_MEM 100

Tympan myTympan(TympanRev::E); 
EarpieceShield              earpieceShield(TympanRev::E, AICShieldRev::A);
SDClass sdx;  // explicitly create SD card, which we will pass to AudioSDWriter *and* which we will use for text logging
String experiment_log_filename = "ExperimentLog.txt";
//AudioSynthWaveform_F32    sineWave(audio_settings); 
//
//AudioInputI2S_F32    i2s_in(audio_settings);                     //Digital audio in *from* the Teensy Audio Board ADC.
//AudioSDWriter_F32_UI audioSDWriter(&(sdx.sdfs), audio_settings);
//AudioOutputI2S_F32   i2s_out(audio_settings);                    //Digital audio out *to* the Teensy Audio Board DAC.

//Make the audio connections
//AudioConnection_F32       patchCord1(i2s_in, 0, i2s_out, 0);    //connect the Left input
//AudioConnection_F32       patchCord2(i2s_in, 1, i2s_out, 1);    //connect the Right input
////Connect to SD logging
//AudioConnection_F32       patchCord3(i2s_in, 0, audioSDWriter, 0); //connect Raw audio to left channel of SD writer
//AudioConnection_F32       patchCord4(i2s_in, 1, audioSDWriter, 1); //connect Raw audio to right channel of SD writer

#include      "AudioConnections.h"
#include "SerialManager.h"
#include "State.h"
//Create BLE
BLE_UI ble(&myTympan);
SerialManager serialManager(&ble);
State myState(&audio_settings, &myTympan, &serialManager);

const float SPL_OF_FULL_SCALE_INPUT = 115.0;
const float LINEAR_GAIN_DB = 0.0; //10.0;
const float KNEE_COMPRESSOR_dBSPL = 55.0;  //follows setMaxdB() above
const float COMP_RATIO = 1.5;
const float KNEE_LIMITER_dBSPL = SPL_OF_FULL_SCALE_INPUT - 20.0;

//define a function to configure the left and right compressors
void setupMyCompressors(float gain, float attack, float release) {
  comp1.setAttack_msec(attack); //left channel
  comp2.setAttack_msec(attack); //right channel
  comp1.setRelease_msec(release); //left channel
  comp2.setRelease_msec(release); //right channel

  //Single point system calibration.  what is the SPL of a full scale input (including effect of input_gain_dB)?
  comp1.setMaxdB(SPL_OF_FULL_SCALE_INPUT);  //left channel
  comp2.setMaxdB(SPL_OF_FULL_SCALE_INPUT);  //right channel

  //set the linear gain
  comp1.setGain_dB(gain);  //set the gain of the Left-channel linear gain at start of compression
  comp2.setGain_dB(gain);  //set the gain of the Right-channel linear gain at start of compression

  //now define the compression parameters
  comp1.setKneeCompressor_dBSPL(KNEE_COMPRESSOR_dBSPL);  //left channel
  comp2.setKneeCompressor_dBSPL(KNEE_COMPRESSOR_dBSPL);  //right channel
  comp1.setCompRatio(COMP_RATIO); //left channel
  comp2.setCompRatio(COMP_RATIO); //right channel

  //finally, the WDRC module includes a limiter at the end.  set its parameters
  //note: the WDRC limiter is hard-wired to a compression ratio of 10:1
  comp1.setKneeLimiter_dBSPL(KNEE_LIMITER_dBSPL);  //left channel
  comp2.setKneeLimiter_dBSPL(KNEE_LIMITER_dBSPL);  //right channel

}

void connectClassesToOverallState(void) {
  myState.comp1 = &comp1.state;
  myState.comp2 = &comp2.state;
  myState.earpieceMixer = &earpieceMixer.state;
  
}

void setupSerialManager() {
  serialManager.add_UI_element(&myState);
  //serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
  serialManager.startTimer();
}

//void setConfiguration(void) {
//  const float default_input_gain_dB = 15.0f;
//  // could set some input source or something else with config
//  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
//  setInputGain_dB(default_input_gain_dB);
//
//  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
//  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
//}

void setup() {

  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();
  delay(1000);
  Serial.println("PerceptionOfCompressionParams: starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  // TODO revisit MAX_AUDIO_MEM should it be 30? should it be something else?
  AudioMemory_F32(MAX_AUDIO_MEM, audio_settings);

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate the Tympan's audio module
  earpieceShield.enable();
  earpieceMixer.setTympanAndShield(&myTympan, &earpieceShield);

  connectClassesToOverallState();
  setupSerialManager();

  earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop(), if micDetect is enabled
  earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_PDM);       // ****but*** then activate the PDM mics
  Serial.println("setup: PDM Earpiece is the active input.");
  
  myTympan.volume_dB(myState.output_gain_dB);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  earpieceShield.volume_dB(myState.output_gain_dB);
  myTympan.setInputGain_dB(myState.earpieceMixer->inputGain_dB);
  
  //set the highpass filter on the Tympan hardware to reduce DC drift
  float hardware_cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, hardware_cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  earpieceShield.setHPFonADC(true, hardware_cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable

  delay(500);
  //setup BLE
  while (Serial1.available()) {
    Serial1.read(); //clear the incoming Serial1 (BT) buffer
  }
  ble.setupBLE(myTympan.getBTFirmwareRev());  //this uses the default firmware assumption. You can override!
  delay(500);

  //configure the left and right compressors with the desired settings
  setupMyCompressors(LINEAR_GAIN_DB, myState.FAST_ATTACK, myState.FAST_RELEASE);

  //prepare the SD writer for the format that we want
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(2);

  if (!sdx.sdfs.begin(SdioConfig(FIFO_SDIO)))
    sdx.sdfs.errorHalt(&Serial, "setup: SD begin failed!");

  Serial.println("Setup complete.");
  serialManager.printHelp();
  serialManager.setButtonState("fast", true, false);
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

//  static int prev_SD_state = (int)audioSDWriter.getState();
//  if ((int)audioSDWriter.getState() != prev_SD_state) {
//    writeTextToSD(String(millis() + String(", audioSDWriter_State = ")
//      + String((int)audioSDWriter.getState())));
//  }
//  prev_SD_state = (int)audioSDWriter.getState();

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(), audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING);

  //periodically print the CPU and Memory Usage
//  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
//  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)
    // TODO
//  if (myState.newExperimentTone > 0 && myState.toneStartTime == 0) {
//    myState.toneStartTime = millis();
//    AudioConnection_F32 patchCord101(sineWave, 0, i2s_out, 0);  //connect to left output
//    AudioConnection_F32 patchCord102(sineWave, 0, i2s_out, 1);  //connect to right output
//    sineWave.amplitude(0.03);
//    myTympan.setAmberLED(HIGH);
//  } else if (myState.toneStartTime > 0) {
//    if ((millis() - myState.toneStartTime) >= myState.newExperimentTone) {
//      AudioConnection_F32       patchCord1(i2s_in, 0, i2s_out, 0);    //connect the Left input
//      AudioConnection_F32       patchCord2(i2s_in, 1, i2s_out, 1);
//      myState.newExperimentTone = 0;
//      myState.toneStartTime = 0;
//      sineWave.amplitude(0.0f);
//      myTympan.setAmberLED(LOW);
//    }
//  }

} //end loop();

void writeTextToSD(String results) {
  Serial.println("writeTextToSD: opening " + String(experiment_log_filename));
  FsFile resultsFile = sdx.sdfs.open(experiment_log_filename,  O_WRITE | O_CREAT | O_APPEND);
  Serial.println("writeTextToSD: writing: " + results);
  resultsFile.println(results);
  Serial.println("writeTextToSD: closing file.");
  resultsFile.close();
}

//float incrementInputGain(float increment_dB) {
//  return setInputGain_dB(myState.input_gain_dB + increment_dB);
//}

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

void printSignalLevels(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    
    float in_dB = comp1.getCurrentLevel_dB();         //here is what the compressor is seeing at its input
    float gain_dB = comp1.getCurrentGain_dB();
    float out_dB = in_dB + gain_dB; //this is the ouptut, including the effect of the changing gain of the compressor
    
    myTympan.println("State: printLevels: Left In, gain, Out (dBFS) = " + String(in_dB,1) + ", " + String(gain_dB,1) + ", " + String(out_dB,1)); //send to USB Serial
  
    lastUpdate_millis = curTime_millis;
  } // end if
}
void updateOutputGain(float increment) {
  myState.output_gain_dB += increment;
  earpieceShield.volume_dB(myState.output_gain_dB);
}
void printOutputGain(void) {
  Serial.println("Output gain at " + String(myState.output_gain_dB) + " dB");
}
