/* Matrix Mixer for the TeensyAudio Library (3.X & 4.X)
 * Mix N signals into N outputs (4 <= N <= 16).
 * Copyright (c) 2014, Paul Stoffregen, paul@pjrc.com
 *
 * Modified by Macaba, palmerr23 (2019, 2025)

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

#ifndef mixer16_h_
#define mixer16_h_

#include "Arduino.h"
#include "AudioStream.h"
#define MMINPUTS  4 // default numbers for inputs and outputs
#define MMOUTPUTS 4
#define MMINMAX	  16 // Biggest values 
#define MMOUTMAX  16

class AudioMixerMatrix : public AudioStream
{
#if defined(__ARM_ARCH_7EM__)
public:
	AudioMixerMatrix(uint8_t inCh = MMINPUTS, uint8_t outCh = MMOUTPUTS) : AudioStream(inCh, inputQueueArray) {
		inChannels = inCh;
		outChannels = outCh;
		for (int i=0; i< MMINMAX; i++) 
			for(int j=0; j<MMOUTMAX; j++) 
				multiplier[i][j] = 65536;
	}
	virtual void update(void);
	void gain(unsigned int inChannel, unsigned int outChannel,  float gain) {
		if (outChannel >= MMOUTMAX) return;
		if (inChannel >= MMINMAX) return;
		if (gain > 32767.0f) gain = 32767.0f;
		else if (gain < -32767.0f) gain = -32767.0f;
		multiplier[inChannel][outChannel] = gain * 65536.0f; // TODO: proper roundoff?
	}
private:
	int32_t multiplier[MMINMAX][MMOUTMAX]; 
	audio_block_t *inputQueueArray[MMINMAX];
	uint8_t inChannels, outChannels;

#elif defined(KINETISL)
public:
	AudioMixerMatrix(uint8_t inCh = MMINPUTS, uint8_t outCh = MMOUTPUTS) : AudioStream(inCh, inputQueueArray) {
		inChannels = inCh;
		outChannels = outCh;
		for (int i=0; i< MMINMAX; i++) 
			for(int j=0; j<MMOUTMAX; j++) 
				multiplier[i][j] = multiplier[i][j] = 256;
	}
	virtual void update(void);
	void gain(unsigned int inChannel, unsigned int outChannel, float gain) {
		if (outChannel >= MMOUTMAX) return;
		if (inChannel >= MMINMAX) return;
		if (gain > 127.0f) gain = 127.0f;
		else if (gain < -127.0f) gain = -127.0f;
		multiplier[inChannel][outChannel] = gain * 256.0f; // TODO: proper roundoff?
	}
private:
	int16_t multiplier[MMINMAX][MMOUTMAX];
	audio_block_t *inputQueueArray[MMINMAX];
	uint8_t inChannels, outChannels;
#endif
};

#endif
