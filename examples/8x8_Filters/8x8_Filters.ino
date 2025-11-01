/* ADC and DAC Filter test
 * ADC has a high-pass filter.
 * Each DAC has 2 stages of biquad filtering.
 * TLV320AIC3104 TDM board multi-codec
 * Single board 8x8 operation
 * First output is connected to a white  noise generator
 * the output is looped back to first inputs.
 * The input is connected a FFT analyzer.
 * First the input HPF is activated, then an output DAC filter. 
 * Uses TDMA Revised library
 * Board USB Type must include AUDIO 
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
  
AudioSynthNoiseWhite     noise;      
AudioAnalyzeFFT1024      fft0;          

AudioOutputTDM_A         tdm_out;       

// Send the input via USB to monitor the signal using a PC tool such as REW
// AudioOutputUSB           USBout;  // from Teensy  
// AudioConnection          patchCord2(tdm_in, 0, USBout, 0);

AudioConnection          patchCord1(noise, 0, tdm_out, 0);
AudioConnection          patchCord4(noise, 0, tdm_out, 1);

AudioConnection          patchCord3(tdm_in, 0, fft0, 0);

AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM);

int muxesFound = 0;
void setup() 
{
  AudioMemory(AUDIO_BLOCKS);
  Serial.begin(115200);
  delay(1000); 
  Serial.println("\n\nT4 TDM AIC3104 example - ADC and DAC filters");

  Wire.begin();
  Wire.setClock(400000);
  aic.setVerbose(0); // > 0: diagnostics for hardware validation, not required for production
 
  muxesFound = aic.begin();
  Serial.printf("Boards found %i\n", muxesFound);
  aic.listMuxes();  

  // when issued before enable() all inputs are affected
  aic.inputMode(AIC_DIFF); 

  if(!aic.enable(0)) // After enable() DAC and ADC are muted
    Serial.println("Failed to initialise codec");

  // After enable() DAC and ADC are muted. Codecs to change need to be explicitly specified.
  aic.volume(1, CH_BOTH, 0);  // muted on startup
  aic.inputLevel(0, CH_BOTH, 0); // level in dB: 0 = line level
  noise.amplitude(0.8);

  delay(8000); // wait for filters and FFTs to stabilise
  printFFTs("No filters");

  // ADC input HP filter - codec 0, both channels
  aic.adcHPF(200, CH_BOTH, 0); 
  delay(8000);
  printFFTs("ADC: 200Hz Highpass filter");

  // DAC output HP filter
  aic.adcHPF(0, CH_BOTH, 0); // turn the ADC filter off
  aic.setHighpass(0, 400, 1, CH_BOTH, 0);
  delay(8000);
  printFFTs("ADC HPF off\nDAC: 1-stage 400Hz Highpass filter");

  aic.setNotch(1, 2000, 2, CH_BOTH, 0);
  delay(8000);
  printFFTs("DAC: add 2kHz notch filter");

  Serial.printf("audioProc %2.1f%%, audioMem %i\n - DONE -\n", AudioProcessorUsage(), AudioMemoryUsage()); 
}

void loop() 
{
  delay(10);
}

#define SAMPLES 20
#define POINTS 7
#define NORMALISE 15
#define PLOTSCALE 3 // dB steps
int freq[POINTS] = {100, 200, 500, 1000, 2000, 5000, 10000};
float fftSUM[POINTS];
float val[POINTS];
void printFFTs(const char *title)
{
  int bin;
  int i, j;
  float k;

  Serial.printf("FFT: %s - all dB values are relative\n", title);
  for (i = 0; i < POINTS; i++)  
    fftSUM[i] = 0;
  for( j = 0; j < SAMPLES; j++) // average over many fft cycles
  {    
    while(!fft0.available())
      delay(10);
    for(i = 0; i < POINTS; i++)
    {
      bin = freq[i] / 43; // 43Hz equi-spaced buckets
      fftSUM[i] += fft0.read(bin);       
    }
    delay(20); // longer than the FFTS refresh of 1/86 secs
  }

  // find upper and lower bounds of bins being sampled
  float max =-100;
  float min = 100;
  for(i = 0; i < POINTS; i++)
  {
    val[i] = 20 * log10(fftSUM[i]) + NORMALISE;
    int bin = freq[i]/43;
    Serial.printf(" %5i Hz [%3i]: %+2.1f dB\n", freq[i], bin, val[i]); // measured dB level
    if(val[i] > max) max = val[i];
    if(val[i] < min) min = val[i];
  }

  // graph the values
  for(k = max; k >= min; k-= PLOTSCALE)
  {
    Serial.printf("\n %3.0f dB", max - k); // relative dB level
    for(j = 0; j < POINTS; j++)
      Serial.print( (val[j] <= k && val[j] > k - PLOTSCALE) ? "  *  " : "     ");
  }

  // add frequency axis
  Serial.printf("\n Freq ");
  for(j = 0; j < POINTS; j++)
    Serial.printf("%4i ",freq[j]);
  Serial.println("\n");
}
