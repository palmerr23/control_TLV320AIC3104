/*
 * Test TDM multi-codec with OpenAudioLibrary (32 bit)
 Single board 8x8 
 Output 2 different sine waves to TDM 0,1
 Input two channels from TDM 0,1

Board USB Type must include AUDIO and SERIAL

 */

#include <Audio.h>
#include "OpenAudio_ArduinoLibrary.h"
#include "AudioStream_F32.h"
#include "output_tdm32.h"
#include "input_tdm32.h"
#include <Wire.h>
#include "control_tlv320aic3104.h"

#define SAMPLERATE 44100.0f
#define SAMPLEWIDTH 32
#define CODECS 4
#define BLOX 40
#define RST_PIN 22 

const float sample_rate_Hz = SAMPLERATE ;  // 24000, 44117, or other frequencies listed above
const int   audio_block_samples = 128;    // Always 128, which is AUDIO_BLOCK_SAMPLES from AudioStream.h
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

AudioInputTDM_32             tdm_in(SAMPLEWIDTH);    
AudioSynthSineCosine_F32     sine1;     
AudioAnalyzePeak_F32         peak0;
AudioAnalyzePeak_F32         peak1;
AudioOutputTDM_32            tdm_out(SAMPLEWIDTH);  

AudioConnection_F32          patchCord1(sine1, 0, tdm_out, 0);
AudioConnection_F32          patchCord2(sine1, 1, tdm_out, 1);
AudioConnection_F32          patchCord12(sine1, 1, tdm_out, 2);
AudioConnection_F32          patchCord3(tdm_in, 0, peak0, 0);
AudioConnection_F32          patchCord4(tdm_in, 1, peak1, 0);

AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM, SAMPLERATE, SAMPLEWIDTH);
void setup() 
{
  AudioMemory_F32(BLOX);
  Serial.begin(115200);
  delay(100); 
  Serial.println("\n\nT4 OpenAudioLib F32 base test");

  Wire.begin();
  Wire.setClock(400000);
  aic.setVerbose(0);
  int boardsFound = aic.begin(RST_PIN);
  Serial.printf("Boards found %i\n", boardsFound);
  
  aic.inputMode(AIC_DIFF);
  if(!aic.enable(-1)) // After enable() DAC and ADC are muted
    Serial.println("Failed to init codecs");

  aic.volume(1, -1, -1);  // muted on startup
  aic.inputLevel(0, -1, -1); //db

  sine1.frequency(500);
  sine1.amplitude(0.9);

  int procUse = AudioProcessorUsage();
  int memUse = AudioMemoryUsage();
  Serial.printf("proc %i%%, mem %i\n", procUse, memUse);
  Serial.println("Done setup");
}
uint32_t  vTimer;
#define PROCESS_EVERY 4000

void loop() 
{
  if(millis() > vTimer + PROCESS_EVERY)
  {
    vTimer = millis();   
    Serial.printf("%4i: val  %3.3f | %3.3f, ", millis()/1000, peak0.read(), peak1.read()); 
    Serial.printf("Proc %2.2f%%, Mem %i\n", AudioProcessorUsage(), AudioMemoryUsage_F32());  
  }
}
