/*
 * DynamicPatch.ino
 * TLV320AIC3104 multi board
 * TDM multi-codec example
 * 8x8 dynamic patching - see https://forum.pjrc.com/index.php?threads/teensyduino-1-57-released.70740/page-2#post-310627
 * Outputs connected to a single sine generator
 * Inputs connected to peak reading analyzers
 */

#include "output_tdmA.h"
#include "input_tdmA.h"
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "control_tlv320aic3104.h"

#define MEASURE_PEAKS
#define CODECS        8
#define AUDIO_BLOCKS (CODECS * 8 + 4) // 8 = 2 * (2 + 2) per CODEC + processing
#define PEAKS (CODECS * 2) //  all inputs 


AudioInputTDM_A          tdm_in;   // to teensy   
AudioSynthWaveformSine   sine1;      
#ifdef MEASURE_PEAKS
AudioAnalyzePeak         peakRead[PEAKS]; 
#endif
AudioOutputTDM_A         tdm_out;  // from Teensy    
AudioConnection          acOut[CODECS*2]; // all outputs
AudioConnection          acIn[CODECS*2]; // just two inputs

AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM);

void setup() 
{
  AudioMemory(AUDIO_BLOCKS);
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nT4 TDM AIC3104 example - 8x8 dynamic patching");

  int chan;
  // connect all outputs to the sine generator and inputs to peak analyzers
  for(chan = 0; chan < CODECS *2; chan++)
  {
    acOut[chan].connect(sine1, 0, tdm_out, chan);
#ifdef MEASURE_PEAKS
    if(chan < PEAKS)
      acIn[chan].connect(tdm_in, chan, peakRead[chan], 0);
#endif
  }

  Wire.begin();
  Wire.setClock(400000);

  aic.setVerbose(2); // for hardware validation, not required for production
  int8_t testCodec = AIC_ALL_CODECS;

  aic.begin();
  aic.listMuxes();  // not required for production

  // when issued before enable() all inputs are affected
  aic.inputMode(AIC_SINGLE); // or AIC_DIFF
  aic.setHPF(AIC_HPF_0125) // default is AIC_HPF_0045
  if(!aic.enable(testCodec)) 
    Serial.println("Failed to initialise codec");

  // After enable() DAC and ADC are muted. Codecs to change need to be explicitly specified.
  aic.volume(1, -1, testCodec);  // muted on startup
  aic.inputLevel(0, -1, testCodec); // level in dB: 0 = line level, ~-50 = mic level

  sine1.frequency(1200);
  sine1.amplitude(0.9);

  delay(100);

  Serial.println("Done setup");
}

uint32_t vTimer;
#define PROCESS_EVERY 4000

void loop() 
{
  if(millis() > vTimer + PROCESS_EVERY)
  {
    vTimer = millis();
    Serial.printf("T %l3i: ", millis()/1000);  
#ifdef MEASURE_PEAKS
    for(int i = 0; i < PEAKS; i++)
      Serial.printf("%1.3f%s", peakRead[i].read(), (i % 4 == 3)? " | " : ", ");
#endif
    Serial.printf("audioProc %2.1f%%, audioMem %i\n", AudioProcessorUsage(), AudioMemoryUsage()); 
  }
}
