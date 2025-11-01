/*
 * MatrixMixer.ino
 * TLV320AIC3104 multi board TDM multi-codec example
 *
 * 8x8 matrix mixer
 * Sine generators connected to first two mixer channels  
 * Remaining CODEC inputs connected to mixer
 * All mixer channels to CODEC outputs and peak-reading meters  
 */

#include "output_tdmA.h"
#include "input_tdmA.h"
#include <Audio.h>
#include "mixerMatrix.h"
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "control_tlv320aic3104.h"

#define MEASURE_PEAKS
#define CODECS        4
#define AUDIO_BLOCKS (CODECS * 8 + 4) // 8 = 2 * (2 + 2) per CODEC + processing
#define PEAKS (CODECS * 2) //  all inputs 


AudioInputTDM_A          tdm_in;   // to teensy   
AudioSynthWaveformSine   sine[2];      
  
AudioMixerMatrix         mixer(8,8);
AudioConnection          acIn[CODECS*2]; //  CODEC or sine to mixer

#ifdef MEASURE_PEAKS
AudioAnalyzePeak         peakRead[PEAKS]; 
AudioConnection          peakIn[CODECS*2];  // mixer to Peak
#endif

AudioConnection          acOut[CODECS*2]; // mixer to CODECC outputs
AudioOutputTDM_A         tdm_out;  // from Teensy  
AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM);

void setup() 
{
  AudioMemory(AUDIO_BLOCKS);
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nT4 TDM AIC3104 example - 8x8 matrix mixer");

  int chan, chanB;
    // connect first two mixer inputs to the sine generators
  for(chan = 0; chan < CODECS *2; chan++)
  {
    if (chan < 2)
      acIn[chan].connect(sine[chan], 0, mixer, chan);
    else
      acIn[chan].connect(tdm_in, chan, mixer, chan);

    acOut[chan].connect(mixer, chan, tdm_out, chan);
#ifdef MEASURE_PEAKS
    if(chan < PEAKS)
      peakIn[chan].connect(mixer, chan, peakRead[chan], 0);
#endif
    // set all the faders to zero
    for(chanB = 0; chanB < CODECS *2; chanB++)
      mixer.gain(chan, chanB, 0.0);
  }

  Wire.begin();
  Wire.setClock(400000);

  aic.setVerbose(2); // for hardware validation, not required for production
  int8_t testCodec = AIC_ALL_CODECS;

  aic.begin();
  aic.listMuxes();  // not required for production

  // when issued before enable() all inputs are affected
  aic.inputMode(AIC_SINGLE); // or AIC_DIFF

  if(!aic.enable(testCodec)) 
    Serial.println("Failed to initialise codec");

  // After enable() DAC and ADC are muted. Codecs to change need to be explicitly specified.
  aic.volume(1, CH_BOTH, testCodec);  // muted on startup
  aic.inputLevel(0, CH_BOTH, testCodec); // level in dB: 0 = line level, ~-50 = mic level

  sine[0].frequency(500);
  sine[0].amplitude(0.4);

  sine[1].frequency(1200);
  sine[1].amplitude(0.8);
  mixer.gain(0, 0, 1.0); // mix: first two input channels to corresponding output channels and peaks
  mixer.gain(1, 1, 1.0);

  Serial.println("Done setup");
}

uint32_t vTimer;
#define PROCESS_EVERY 4000
int counter;
void loop() 
{
  if(millis() > vTimer + PROCESS_EVERY)
  {
    vTimer = millis();
    counter++;
    Serial.printf("T %l3i: ", millis()/1000);  
#ifdef MEASURE_PEAKS
    for(int i = 0; i < PEAKS; i++)
      Serial.printf("%1.3f%s", peakRead[i].read(), (i % 4 == 3)? " | " : ", ");
#endif
    Serial.printf("audioProc %2.1f%%, audioMem %i\n", AudioProcessorUsage(), AudioMemoryUsage()); 
  
    if(counter % 2 == 0)
    {
      mixer.gain(0, 0, 1.0); 
      mixer.gain(1, 1, 1.0);
      mixer.gain(1, 0, 0.0); 
      mixer.gain(0, 1, 0.0); 
    }
    else
    {      
      mixer.gain(1, 0, 1.0); // swap the two sine waves
      mixer.gain(0, 1, 1.0);   
      mixer.gain(0, 0, 0.0); 
      mixer.gain(1, 1, 0.0);   
    }
  }
}
