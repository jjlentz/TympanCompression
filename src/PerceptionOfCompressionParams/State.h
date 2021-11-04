#ifndef State_h
#define State_h

extern const int N_CHAN;

class State : public TympanStateBase_UI {
  public:
    State(AudioSettings_F32 *settings, Print *serial, SerialManagerBase *serial_manager) 
      : TympanStateBase_UI(settings, serial, serial_manager) {}
            
  float input_gain_dB = 10.0;
  float output_gain_dB = 0.0;
  // check the base class to see if this is already there
  float digital_gain_dB = 0.0;
  int newExperimentTone = 0;
  unsigned long toneStartTime = 0;
};

#endif
