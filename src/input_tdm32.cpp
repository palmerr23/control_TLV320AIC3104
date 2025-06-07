/* Audio Library for Teensy 3.X
 *** for 32 bit float samples and OpenAudio_Arduino library
 *** 8 channels per frame
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

#include <Arduino.h>
#include "input_tdm32.h"
#include "output_tdm32.h"
#if defined(KINETISK) || defined(__IMXRT1062__)
#include "utility/imxrt_hw.h"

//************** upgrade to 64 bit DMA transfers ***********
DMAMEM __attribute__((aligned(32)))
static int32_t tdm_rx_buffer[TDM_CHANNELS*2*AUDIO_BLOCK_SAMPLES]; // 2 complete sets
audio_block_f32_t * AudioInputTDM_32::block_incoming[TDM_CHANNELS] = {
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
bool AudioInputTDM_32::update_responsibility = false;
DMAChannel AudioInputTDM_32::dma(false);
static int _sampleLengthI;

void AudioInputTDM_32::begin(int sampleLength, float sampleRate)
{
	_sampleLengthI = sampleLength;
//	sample_rate_Hz = sampleRate;
	dma.begin(true); // Allocate the DMA channel first

	// TODO: should we set & clear the I2S_RCSR_SR bit here?
	AudioOutputTDM_32::config_tdm();
#if defined(KINETISK)
	CORE_PIN13_CONFIG = PORT_PCR_MUX(4); // pin 13, PTC5, I2S0_RXD0
	dma.TCD->SADDR = &I2S0_RDR0;
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = tdm_rx_buffer;
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S0_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	I2S0_TCSR |= I2S_TCSR_TE | I2S_TCSR_BCE; // TX clock enable, because sync'd to TX
	dma.attachInterrupt(isr);
#elif defined(__IMXRT1062__)
	CORE_PIN8_CONFIG  = 3;  //RX_DATA0
	IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2;
	dma.TCD->SADDR = &I2S1_RDR0;
	dma.TCD->SOFF = 0;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = 0;
	dma.TCD->DADDR = tdm_rx_buffer;
	dma.TCD->DOFF = 4;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->DLASTSGA = -sizeof(tdm_rx_buffer);
	dma.TCD->BITER_ELINKNO = sizeof(tdm_rx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_RX);
	update_responsibility = update_setup();
	dma.enable();

	I2S1_RCSR = I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FRDE | I2S_RCSR_FR;
	dma.attachInterrupt(isr);	
#endif	
}

// dma buffer holds two full sets of samples
volatile int dmaBalz = 0;
int AudioInputTDM_32::getDMAbal(void) { return dmaBalz;}


// read int32_t samples into float32 without conversion (done in update)
void AudioInputTDM_32::isr(void)
{
	uint32_t daddr;
	const int32_t *src;
	float32_t *dest;
	unsigned int i, j;

	daddr = (uint32_t)(dma.TCD->DADDR);
	dma.clearInterrupt();

	if (daddr < (uint32_t)tdm_rx_buffer + sizeof(tdm_rx_buffer) / 2) {
		// DMA is receiving to the first half of the buffer
		// need to remove data from the second half
		src = &tdm_rx_buffer[AUDIO_BLOCK_SAMPLES*TDM_CHANNELS];
		dmaBalz++;
	} else {
		// DMA is receiving to the second half of the buffer
		// need to remove data from the first half
		src = &tdm_rx_buffer[0];
		dmaBalz--;
	}
	if (block_incoming[0] != nullptr) // one block allocated -> all blocks OK
	{
		#if IMXRT_CACHE_ENABLED >=1
		arm_dcache_delete((void*)src, sizeof(tdm_rx_buffer) / 2);
		#endif

		for(j = 0; j < AUDIO_BLOCK_SAMPLES; j++)
		{
			for (i = 0; i < TDM_CHANNELS; i++) 
			{ 
				dest = (float32_t *)&(block_incoming[i]->data[j]);
				*dest = ((float32_t)(*src)) * I32_TO_F32_NORM_FACTOR;
				src++;
			}
		}
	}
	if (update_responsibility) update_all();
}

void AudioInputTDM_32::scale_i32_to_f32(int32_t *p_i32, float32_t *p_f32, int len) {
	for (int i=0; i<len; i++) 
	{ 
		*p_f32++ = (float32_t)((*p_i32++) * I32_TO_F32_NORM_FACTOR); 
	}
}

void AudioInputTDM_32::update(void)
{
	unsigned int i, j;
	audio_block_f32_t *new_block[TDM_CHANNELS];
	audio_block_f32_t *out_block[TDM_CHANNELS];

	// allocate 8 new blocks.  If any fails, allocate none
	for (i=0; i < TDM_CHANNELS; i++) {
		new_block[i] = allocate_f32();
		if (new_block[i] == nullptr) {
			for (j = 0; j < i; j++) {
				release(new_block[j]);
			}
			memset(new_block, 0, sizeof(new_block)); // is this needed? 
			break;
		}
	}
	__disable_irq();
	memcpy(out_block, block_incoming, sizeof(out_block));
	memcpy(block_incoming, new_block, sizeof(block_incoming));
	__enable_irq();
	if (out_block[0] != nullptr) {
		// if we got 1 block, all 8 are filled
		// copy the pointers
		for (i = 0; i < TDM_CHANNELS; i++) {
			//scale_i32_to_f32(out_block[i]->data, out_block[i]->data, AUDIO_BLOCK_SAMPLES); // convert in place
			transmit(out_block[i], i);
			release(out_block[i]);
		}
	}
}


#endif
