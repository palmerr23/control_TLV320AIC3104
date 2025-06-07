/*
 * Test TDM multi-codec with OpenAudioLibrary 
 USB test 
 **** The USB_Audio_F32 seems to drop 4 bits at the end of each buffer, 
 **** so use AudioConvert_F32toI16 and the standard Teensy Audio library USB objects
 Single board 8x8 (32 bit TDM)
 sine to both USBout
 USBin to Output 0 & 1
 Arduino board USB Type must include AUDIO
 */

#include <Audio.h>
#include "OpenAudio_ArduinoLibrary.h"
#include "AudioStream_F32.h"
//#include "USB_Audio_F32.h" // don't use this!
#include "output_tdm32.h"
#include "input_tdm32.h"
#include <Wire.h>
#include "control_tlv320aic3104.h"

#define SAMPLERATE 44100.0f
#define SAMPLEWIDTH 32
#define CODECS 4
#define BLOX_32 20
#define BLOX_16 10
#define RST_PIN 22 

AudioSettings_F32 audio_settings(SAMPLERATE, AUDIO_BLOCK_SAMPLES);

AudioInputTDM_32             tdm_in(SAMPLEWIDTH);    
AudioInputUSB                USBin;

AudioConvert_F32toI16        conv1_o; // to USB
AudioConvert_F32toI16        conv2_o; 

AudioConvert_I16toF32        conv1_i; // from USB
AudioConvert_I16toF32        conv2_i;

AudioOutputUSB               USBout;
AudioOutputTDM_32            tdm_out(SAMPLEWIDTH);    

// USB out
AudioConnection_F32          patchCord01(tdm_in, 0, conv1_o, 0);
AudioConnection_F32          patchCord02(tdm_in, 1, conv2_o, 0);
AudioConnection              patchCord03(conv1_o, 0, USBout, 0);
AudioConnection              patchCord04(conv2_o, 0, USBout, 1);

//USB in
AudioConnection              patchCord11(USBin, 0, conv1_i, 0);
AudioConnection              patchCord12(USBin, 1, conv2_i, 0);
AudioConnection_F32          patchCord13(conv1_i, 0, tdm_out, 0);
AudioConnection_F32          patchCord14(conv2_i, 0, tdm_out, 1);

AudioControlTLV320AIC3104    aic(CODECS, true, AICMODE_TDM, SAMPLERATE, SAMPLEWIDTH);

void setup() 
{
  AudioMemory_F32(BLOX_32);
  AudioMemory(BLOX_16); // usb and convertXtoY functions
  Serial.begin(115200);
  delay(100); 
  Serial.printf("\n\nT4 TDM AIC3104 USB_16 test - 32 bit samples\nsampleRate %5.1f sampleWidth %i\n", SAMPLERATE, SAMPLEWIDTH);

  Wire.begin();
  Wire.setClock(400000);

  aic.setVerbose(2);
  aic.begin(RST_PIN);
  aic.inputMode(AIC_DIFF);
  if(!aic.enable(-1)) 
    Serial.println("Failed to init codecs");
  aic.volume(1, -1, -1);  // muted on startup
  aic.inputLevel(0, -1, -1); //db

  delay(100); // allow  a few buffers to run through  
 
  int procUse =  AudioProcessorUsage();
  int memUse32 = AudioMemoryUsage_F32();
  int memUse16 = AudioMemoryUsage();
  Serial.printf("proc %i\%, mem32 %i, mem16 %i\n", procUse, memUse32, memUse16); 
  Serial.println("Done setup");
}

void loop() 
{
 
}

