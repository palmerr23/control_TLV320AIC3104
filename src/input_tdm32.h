/* Audio Library for Teensy 3.X
 *** for 32 bit float samples and OpenAudio_Arduino library
 *** 8 channels per frame
 *** Teensy 4 only
 * Copyright (c) 2017, Paul Stoffregen, paul@pjrc.com
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _input_tdm32_h_
#define _input_tdm32_h_

#include <Arduino.h>  
#include "AudioStream_F32.h"
#include <DMAChannel.h> 

#define TDM_CHANNELS 8
#define I32_TO_F32_NORM_FACTOR (4.656612875245797e-10)   //which is 1/(2^31 - 1)
class AudioInputTDM_32 : public AudioStream_F32
{
public:
	AudioInputTDM_32(void) : AudioStream_F32(0, NULL) { begin(); } 
	AudioInputTDM_32(int sampleLength = 32, float sampleRate = 44100.0) : AudioStream_F32(0, NULL) 
		{ begin(sampleLength, sampleRate); }
	virtual void update(void);
	void begin(int sampleLength = 32, float sampleRate = 44100.0);
	void scale_i32_to_f32( int32_t *p_i32, float32_t *p_f32, int len);
	int getDMAbal(void);
protected:	
	static bool update_responsibility;
	static DMAChannel dma;
	static void isr(void);
	//static int audio_block_samples;
	static float sample_rate_Hz;
private:
	static audio_block_f32_t *block_incoming[8];
};

#endif
