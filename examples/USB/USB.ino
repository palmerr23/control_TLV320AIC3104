/*
 * Test TDM multi-codec with OpenAudioLibrary 
 USB test 
 Single board 8x8 (32 bit TDM)
 sine to both USBout
 USBin to Output 0 & 1. Only 2 USB audio channels are supported in Teensy 1.59
 Arduino board USB Type must include AUDIO
 */

#include "output_tdmA.h"
#include "input_tdmA.h"
#include <Audio.h>
#include <Wire.h>
#include "control_tlv320aic3104.h"

#define CODECS 4
#define BLOX_16 40
#define RST_PIN 22 

AudioInputTDM_A        tdm_in;    
AudioInputUSB          USBin;

AudioAnalyzePeak       peak0;
AudioAnalyzePeak       peak1;

AudioOutputUSB         USBout;
AudioOutputTDM_A       tdm_out;    

AudioConnection        patchCord01(tdm_in, 0, USBout, 0);
AudioConnection        patchCord02(tdm_in, 1, USBout, 1);
AudioConnection        patchCord03(tdm_in, 0, peak0, 0);
AudioConnection        patchCord04(tdm_in, 1, peak1, 0);

AudioConnection        patchCord11(USBin, 0, tdm_out, 0);
AudioConnection        patchCord12(USBin, 1, tdm_out, 1);

AudioControlTLV320AIC3104    aic(CODECS, true, AICMODE_TDM);

void setup() 
{
  AudioMemory(BLOX_16); 
  Serial.begin(115200);
  delay(100); 
  Serial.printf("\n\nT4 TDM AIC3104 - USB\n");

  Wire.begin();
  Wire.setClock(400000);

  aic.setVerbose(2);
  aic.begin(RST_PIN);
  aic.inputMode(AIC_DIFF);
  if(!aic.enable(-1)) 
    Serial.println("Failed to init codecs");
  aic.volume(1, -1, -1);  // muted on startup
  aic.inputLevel(0, -1, -1); // dB
  delay(100); // allow  a few buffers to run through  
 
  int procUse =  AudioProcessorUsage();
  int memUse16 = AudioMemoryUsage();
  Serial.printf("proc %i\%, mem %i\n", procUse, memUse16); 
  Serial.println("Done setup");
}

#define PROCESS_EVERY 5000
uint32_t timerx;
void loop() 
{
  if(millis() > timerx + PROCESS_EVERY)
  {
    timerx = millis();
    Serial.printf("%i: %0.3f | %0.3f\n", millis()/1000, peak0.read(), peak1.read());
    Serial.printf("audioProc %2.1f%%, audioMem %i\n", AudioProcessorUsage(), AudioMemoryUsage()); 
  }
}

