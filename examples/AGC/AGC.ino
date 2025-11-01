/*
 * TLV320AIC3104 TDM board multi-codec
 * Single board AGC operation. Compressor and noise gate example.
 * Burst on Channel 0 - loopback to input 0
 * AGC on Input 0 patched to output 1
 * Uses TDMA Revised library
 * Board USB Type must include AUDIO and SERIAL if USB audio is employed
 */

#include "output_tdmA.h"
#include "input_tdmA.h"
#include <Audio.h>
#include "control_tlv320aic3104.h"

#define CODECS 4
#define AUDIO_BLOCKS (CODECS * 8 + 4) // // 8 = 2 * (2 + 2) per CODEC + processing
#define RST_PIN 22 

AudioInputTDM_A          tdm_in;           
AudioSynthWaveformSine   sine1;      
AudioOutputTDM_A         tdm_out;           //xy=1107,845

AudioConnection          patchCord1(sine1, 0, tdm_out, 0);
AudioConnection          patchCord2(tdm_in, 0, tdm_out, 1);

AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM);

int8_t testCodec = 0;
int muxesFound = 0;
float low, high;
void setup() 
{
  high = 0.9;
  low = 0.01; // -40dB
  AudioMemory(AUDIO_BLOCKS);
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n\nT4 TDM AIC3104 example - AGC");

  Wire.begin();
  Wire.setClock(400000);
  aic.setVerbose(1); // for testing.  Omit for production

  if (CODECS == 1)
    testCodec = 0;

  muxesFound = aic.begin();
  Serial.printf("Muxes found %i\n", muxesFound);
  aic.listMuxes();   // not required for production

  // when issued before enable() all inputs are affected
  aic.inputMode(AIC_SINGLE); // or AIC_DIFF

  if(!aic.enable(testCodec)) // After enable() DAC and ADC are muted
    Serial.println("Failed to initialise codec");

  // After enable() DAC and ADC are muted. Codecs to change need to be explicitly specified.
  aic.volume(high, CH_BOTH, testCodec);  // muted on startup
  aic.inputLevel(0, CH_BOTH, testCodec); // level in dB: 0 = line level, ~-50 = mic level

  // Compressor
  // Target level: -10dB, Attack: 8mS, Decay: 400mS, Max gain: 0dB, Hysteresis: 1dB, Noise threshold: -30dB, Clip step: Off
  aic.AGC(AGCT10, AGCA8, AGCD400, 0, AGCH1, -30, false, CH_BOTH, AIC_ALL_CODECS);

  sine1.frequency(500);
  sine1.amplitude(high);

  delay(100);
  Serial.println("Done setup");
}

uint32_t vTimer, bTimer;
bool burst;
#define REPORT_EVERY 5000
#define BURST_TIME 400 

void loop() 
{

  if(millis() > bTimer + BURST_TIME)
  {
    sine1.amplitude((burst)? high : low); 
    burst = !burst;
    bTimer = millis();
  }

  if(millis() > vTimer + REPORT_EVERY)
  {
    vTimer = millis();
    Serial.printf("audioProc %2.1f%%, audioMem %i\n", AudioProcessorUsage(), AudioMemoryUsage()); 
  }
}
