/*
 * TLV320AIC3104 TDM board multi-codec
 * Single board 8x8 operation
 * Hardwired patching
 * All Outputs connected to a single sine generator
 * First two inputs connected to peak reading analyzers
 * Uses TDMA Revised library
 * Board USB Type must include AUDIO and SERIAL if USB audio is employed
 */

#include "output_tdmA.h"
#include "input_tdmA.h"
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "control_tlv320aic3104.h"

#define CODECS 4
#define AUDIO_BLOCKS (CODECS * 8 + 4) // // 8 = 2 * (2 + 2) per CODEC + processing
#define RST_PIN 22 

AudioInputTDM_A          tdm_in;           
AudioSynthWaveformSine   sine1;      
AudioAnalyzePeak         peakRead0;          
AudioAnalyzePeak         peakRead1; 
AudioOutputTDM_A         tdm_out;           //xy=1107,845

AudioConnection          patchCord1(tdm_in, 0, peakRead0, 0);
AudioConnection          patchCord2(tdm_in, 1, peakRead1, 0);

AudioConnection          patchCord5(sine1, 0, tdm_out, 0);
AudioConnection          patchCord6(sine1, 0, tdm_out, 1);

AudioConnection          patchCord25(sine1, 0, tdm_out, 2);
AudioConnection          patchCord26(sine1, 0, tdm_out, 3);

AudioConnection          patchCord7(sine1, 0, tdm_out, 4);
AudioConnection          patchCord8(sine1, 0, tdm_out, 5);

AudioConnection          patchCord11(sine1, 0, tdm_out, 6);
AudioConnection          patchCord12(sine1, 0, tdm_out, 7);

AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM);

int muxesFound = 0;
void setup() 
{
  AudioMemory(AUDIO_BLOCKS);
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n\nT4 TDM AIC3104 example - 8x8 hardwired patching");


  Wire.begin();
  Wire.setClock(400000);
  aic.setVerbose(2); // for hardware validation, not required for production
  int8_t testCodec = AIC_ALL_CODECS;
  if (CODECS == 1)
    testCodec = 0;

  muxesFound = aic.begin();
  Serial.printf("Muxes found %i\n", muxesFound);
  aic.listMuxes();  

  // when issued before enable() all inputs are affected
  aic.inputMode(AIC_SINGLE); // or AIC_DIFF
  //aic.setHPF(AIC_HPF_0045); // default value

  if(!aic.enable(testCodec)) // After enable() DAC and ADC are muted
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
    Serial.printf("T %l3i: %1.3f %1.3f - ", millis()/1000, peakRead0.read(), peakRead1.read());  
    Serial.printf("audioProc %2.1f%%, audioMem %i\n", AudioProcessorUsage(), AudioMemoryUsage()); 
  }
}
