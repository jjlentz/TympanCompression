// Instantiate the audio classess
AudioInputI2SQuad_F32       i2s_in(audio_settings);     //Digital audio in *from* the Teensy Audio Board ADC.
EarpieceMixer_F32_UI        earpieceMixer(audio_settings);  //mixes earpiece mics, allows switching to analog inputs, mixes left+right, etc
AudioFilterBiquad_F32       hp_filt1(audio_settings);   //IIR filter doing a highpass filter.  Left.
AudioFilterBiquad_F32       hp_filt2(audio_settings);   //IIR filter doing a highpass filter.  Right.
AudioEffectCompWDRC_F32_UI  comp1(audio_settings);      //Compresses the dynamic range of the audio.  Left.  UI enabled!!!
AudioEffectCompWDRC_F32_UI  comp2(audio_settings);      //Compresses the dynamic range of the audio.  Right. UI enabled!!!
AudioOutputI2SQuad_F32      i2s_out(audio_settings);    //Digital audio out *to* the Teensy Audio Board DAC.
AudioSDWriter_F32_UI        audioSDWriter(audio_settings);//this is stereo by default


// Make all of the audio connections
AudioConnection_F32         patchcord1(i2s_in,0,earpieceMixer,0);
AudioConnection_F32         patchcord2(i2s_in,1,earpieceMixer,1);
AudioConnection_F32         patchcord3(i2s_in,2,earpieceMixer,2);
AudioConnection_F32         patchcord4(i2s_in,3,earpieceMixer,3);

//connect the left and right outputs of the earpiece mixer to the two filter modules (one for left, one for right)
AudioConnection_F32         patchCord11(earpieceMixer,  earpieceMixer.LEFT,  hp_filt1, 0);   //connect the Left input to the left highpass filter
AudioConnection_F32         patchCord12(earpieceMixer,  earpieceMixer.RIGHT, hp_filt2, 0);   //connect the Right input to the right highpass filter

// connect to the compressors
AudioConnection_F32        patchCord21(hp_filt1, 0, comp1, 0);    //connect to the left gain/compressor/limiter
AudioConnection_F32        patchCord22(hp_filt2, 0, comp2, 0);    //connect to the right gain/compressor/limiter

//Connect the gain modules to the outputs so that you hear the audio
AudioConnection_F32        patchcord31(comp1, 0, i2s_out, EarpieceShield::OUTPUT_LEFT_TYMPAN);    //First AIC, Main tympan board headphone jack, left channel
AudioConnection_F32        patchcord32(comp2, 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_TYMPAN);   //First AIC, Main tympan board headphone jack, right channel
AudioConnection_F32        patchcord33(comp1, 0, i2s_out, EarpieceShield::OUTPUT_LEFT_EARPIECE);  //Second AIC (Earpiece!), left output
AudioConnection_F32        patchcord34(comp2, 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_EARPIECE); //Secibd AIC (Earpiece!), right output

// Connect the raw mic audio to the SD writer
AudioConnection_F32     patchcord41(i2s_in, EarpieceShield::PDM_LEFT_FRONT,  audioSDWriter, 0);    //Left-Front Mic
AudioConnection_F32     patchcord42(i2s_in, EarpieceShield::PDM_LEFT_REAR,   audioSDWriter, 1);    //Left-Rear Mic
AudioConnection_F32     patchcord43(i2s_in, EarpieceShield::PDM_RIGHT_FRONT, audioSDWriter, 2);    //Right-Front Mic
