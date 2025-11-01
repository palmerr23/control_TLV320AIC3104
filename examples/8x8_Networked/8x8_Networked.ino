/*
 * 8x8_Networked.ino
 * TLV320AIC3104 multi board
 * TDM multi-codec example
 * 8x8 dynamic patching
 * Outputs connected to a single sine generator
 * Newtorked and Audio outputs
 * Multi-channel audio creates a significant amount of network traffic. An isolated audio-only network may have less dropouts. 
 * Wireless networks are not recommended as the "guaranteed delivery" mechanisms used can result in  poor performance. 
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

#include "control_ethernet.h"
#include "input_net.h"
#include "output_net.h"

#define MEASURE_PEAKS
#define CODECS        4
#define AUDIO_BLOCKS (CODECS * 8 + 4) // 8 = 2 * (2 + 2) per CODEC + processing
#define PEAKS (2) //  (CODECS * 2) all inputs 


AudioInputNet            eth_in(2);
AudioInputTDM_A          tdm_in;   // to teensy   
AudioSynthWaveformSine   sine1;      

#ifdef MEASURE_PEAKS
AudioAnalyzePeak         peakRead[PEAKS]; 
#endif
AudioOutputTDM_A         tdm_out;  // from Teensy   
AudioOutputNet           eth_out(2); 

//AudioConnection          acOut[CODECS*2]; // all outputs
//AudioConnection          acIn[CODECS*2]; // just two inputs

AudioConnection          eOut(sine1, 0, eth_out, 0);
AudioConnection           t_e(tdm_in, 0, eth_out, 1);

AudioConnection          tout0(sine1, 0, tdm_out, 1);
AudioConnection          e1In(eth_in, 0, tdm_out, 0);

AudioConnection          e1Peak(eth_in, 0, peakRead[0], 0);
AudioConnection          e2Peak(tdm_in, 0, peakRead[1], 0);
AudioControlEthernet      ether1;
AudioControlTLV320AIC3104 aic(CODECS, true, AICMODE_TDM);
#include "utils.h" // diagnostic print functions - assumes "AudioControlEthernet  ether1;""
void setup() 
{
  AudioMemory(AUDIO_BLOCKS);
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n\nT4 TDM AIC3104 example - 8x8 Ethernet");
  char myHost[] = "Teensy1";
  ether1.setHostName(myHost);
  ether1.begin();
  if(!ether1.linkIsUp())
    Serial.printf("Ethernet is disconnected");
  else
    Serial.println(ether1.getMyIP());
  eth_in.begin(); 
  char s1[] = "Stream1";
  eth_in.subscribe(s1);
  eth_out.subscribe("Stream1");
  eth_out.begin();
  int chan;
  // connect all outputs to the sine generator and inputs to peak analyzers
  /*
  for(chan = 0; chan < CODECS *2; chan++)
  {
    //acOut[chan].connect(sine1, 0, tdm_out, chan);
#ifdef MEASURE_PEAKS
    if(chan < PEAKS)
      //acIn[chan].connect(tdm_in, chan, peakRead[chan], 0);
#endif
  }
 */
  Wire.begin();
  Wire.setClock(400000);

  aic.setVerbose(2); // for hardware validation, not required for production
  int8_t testCodec = AIC_ALL_CODECS;

  aic.begin();
  aic.listMuxes();  // not required for production

  // when issued before enable() all inputs are affected
  aic.inputMode(AIC_SINGLE); // or AIC_DIFF
  aic.setHPF(AIC_HPF_0125); // default is AIC_HPF_0045
  if(!aic.enable(testCodec)) 
    Serial.println("Failed to initialise codec");

  // After enable() DAC and ADC are muted. Codecs to change need to be explicitly specified.
  aic.volume(1, CH_BOTH, testCodec);  // muted on startup
  aic.inputLevel(0, CH_BOTH, testCodec); // level in dB: 0 = line level, ~-50 = mic level

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
    //Serial.printf("LinkIs Up %i, IP ", ether1.linkIsUp());
   // Serial.println(ether1.getMyIP());
    printActiveStreams(STREAM_IN);
    printActiveStreams(STREAM_OUT);
    //ether1.printHosts();
    //printActiveSubs();
  }
}
