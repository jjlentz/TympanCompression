#ifndef State_h
#define State_h

extern const int N_CHAN;

class State : public TympanStateBase_UI {
  public:
    State(AudioSettings_F32 *settings, Print *serial, SerialManagerBase *serial_manager) 
      : TympanStateBase_UI(settings, serial, serial_manager) {}
            
    const float FAST_ATTACK = 5.0;
    const float FAST_RELEASE = 20.0;
    const String FAST = "fast";

    const float SLOW_ATTACK = 10.0;
    const float SLOW_RELEASE = 500.0;
    const String SLOW = "slow";
    
    float output_gain_dB = 0.0;
    String activeAlgorithm = "fast";
    int newExperimentTone = 0;
    unsigned long toneStartTime = 0;

    EarpieceMixerState *earpieceMixer;
    AudioCompWDRCState *comp1, *comp2;

    //This controls the printing/plotting of the input and output level of the left compressor
    bool flag_printSignalLevels = false;

};

#endif
