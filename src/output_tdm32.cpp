/* Audio Library for Teensy 4.X
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

#if !defined(KINETISL)

#include "output_tdm32.h"
#include "memcpy_audio.h"
#include "utility/imxrt_hw.h"
// kinetis.h does not included correctly
#define I2S0_TCR2		(*(volatile uint32_t *)0x4002F008) // SAI Transmit Configuration 2 Register

audio_block_f32_t * AudioOutputTDM_32::block_input[TDM_CHANNELS] = {
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};
bool AudioOutputTDM_32::update_responsibility = false;
DMAChannel AudioOutputTDM_32::dma(false);
DMAMEM __attribute__((aligned(32)))
static int32_t zeros[AUDIO_BLOCK_SAMPLES]; // already scaled
DMAMEM __attribute__((aligned(32)))
static int32_t tdm_tx_buffer[TDM_CHANNELS*2*AUDIO_BLOCK_SAMPLES]; // two full sets
static int32_t scaled_i32[TDM_CHANNELS][AUDIO_BLOCK_SAMPLES]; 		// scaled samples for dma
static int _sampleLengthO;

void AudioOutputTDM_32::begin(int sampleLength, float sampleRate)
{
	_sampleLengthO = sampleLength;
	//sample_rate_Hz = sampleRate;
	dma.begin(true); // Allocate the DMA channel first

	for (int i=0; i < TDM_CHANNELS; i++) {
		block_input[i] = nullptr;
	}
	memset(zeros, 0, sizeof(zeros));
	memset(tdm_tx_buffer, 0, sizeof(tdm_tx_buffer));

	// TODO: should we set & clear the I2S_TCSR_SR bit here?
	config_tdm();
#if defined(KINETISK)
	CORE_PIN22_CONFIG = PORT_PCR_MUX(6); // pin 22, PTC1, I2S0_TXD0

	dma.TCD->SADDR = tdm_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = -sizeof(tdm_tx_buffer);
	dma.TCD->DADDR = &I2S0_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_I2S0_TX);

	update_responsibility = update_setup();
	dma.enable();

	I2S0_TCSR = I2S_TCSR_SR;
	I2S0_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;
#elif defined(__IMXRT1062__)
	CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0

	dma.TCD->SADDR = tdm_tx_buffer;
	dma.TCD->SOFF = 4;
	dma.TCD->ATTR = DMA_TCD_ATTR_SSIZE(2) | DMA_TCD_ATTR_DSIZE(2);
	dma.TCD->NBYTES_MLNO = 4;
	dma.TCD->SLAST = -sizeof(tdm_tx_buffer);
	dma.TCD->DADDR = &I2S1_TDR0;
	dma.TCD->DOFF = 0;
	dma.TCD->CITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->DLASTSGA = 0;
	dma.TCD->BITER_ELINKNO = sizeof(tdm_tx_buffer) / 4;
	dma.TCD->CSR = DMA_TCD_CSR_INTHALF | DMA_TCD_CSR_INTMAJOR;
	dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SAI1_TX);

	update_responsibility = update_setup();
	dma.enable();

	I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE;
	I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE | I2S_TCSR_FRDE;

#endif
	dma.attachInterrupt(isr);
}


//define F32_TO_I32_NORM_FACTOR (8388607)   //which is 2^23-1
// input (-1.0 .. 1.0) result is scaled to an int_32 array
void AudioOutputTDM_32::scale_f32_to_i32(float32_t *p_f32, int32_t *p_i32, int len) {
    for (int i = 0; i < len; i++) // constrain(scaled, +/- NORM FACTOR)
		 *p_i32++ = (int32_t)max(-F32_TO_I32_NORM_FACTOR, min(F32_TO_I32_NORM_FACTOR, (*p_f32++) * F32_TO_I32_NORM_FACTOR)); 
}

// dma buffer holds two full sets of samples
volatile int dmaBal = 0;
int AudioOutputTDM_32::getDMAbal(void) { return dmaBal;}

void AudioOutputTDM_32::isr(void)
{
	int32_t *dest;
	uint32_t i, j, saddr;

#if defined(KINETISK) || defined(__IMXRT1062__)
	saddr = (uint32_t)(dma.TCD->SADDR);
#endif
	dma.clearInterrupt();
	if (saddr < (uint32_t)tdm_tx_buffer + sizeof(tdm_tx_buffer) / 2) {
		// DMA is transmitting the first half of the buffer
		// so we must fill the second half
		dest = &tdm_tx_buffer[AUDIO_BLOCK_SAMPLES*TDM_CHANNELS];
		dmaBal++;
	} else {
		// DMA is transmitting the second half of the buffer
		// so we must fill the first half
		dest = tdm_tx_buffer;
		dmaBal--;
	}
	if (update_responsibility) AudioStream::update_all();

	#if IMXRT_CACHE_ENABLED >= 2
	int32_t *dc = dest;
	#endif
	
	// I2S byte twiddling doesn't make sense for 8 x 32
	for (j = 0; j < AUDIO_BLOCK_SAMPLES; j++)
		for (i = 0; i < TDM_CHANNELS; i++) 
		{
			*dest = (block_input[i] != nullptr) ? scaled_i32[i][j] : 1;
			dest++;
		}

	#if IMXRT_CACHE_ENABLED >= 2
	arm_dcache_flush_delete(dc, sizeof(tdm_tx_buffer) / 2 );
	#endif	
		for (i=0; i < TDM_CHANNELS; i++) 
			if (block_input[i]) 
			{
				release(block_input[i]);
				block_input[i] = nullptr;
			}
}

void AudioOutputTDM_32::update(void)
{
	audio_block_f32_t *prev[TDM_CHANNELS];
	unsigned int i;

	__disable_irq();
	for (i = 0; i < TDM_CHANNELS; i++) 
	{
		prev[i] = block_input[i];
		block_input[i] = receiveReadOnly_f32(i);
		if(block_input[i] != nullptr) 		// null input block also detected in isr
			scale_f32_to_i32(block_input[i]->data, scaled_i32[i], AUDIO_BLOCK_SAMPLES);
	}
	__enable_irq();
	for (i = 0; i < TDM_CHANNELS; i++) 
	{
		if (prev[i]) 
			release(prev[i]);
	}
}

#if defined(KINETISK)
// MCLK needs to be 48e6 / 1088 * 512 = 22.588235 MHz -> 44.117647 kHz sample rate
//
#if F_CPU == 96000000 || F_CPU == 48000000 || F_CPU == 24000000
  // PLL is at 96 MHz in these modes
  #define MCLK_MULT 4
  #define MCLK_DIV  17
#elif F_CPU == 72000000
  #define MCLK_MULT 16
  #define MCLK_DIV  51
#elif F_CPU == 120000000
  #define MCLK_MULT 16
  #define MCLK_DIV  85
#elif F_CPU == 144000000
  #define MCLK_MULT 8
  #define MCLK_DIV  51
#elif F_CPU == 168000000
  #define MCLK_MULT 16
  #define MCLK_DIV  119
#elif F_CPU == 180000000
  #define MCLK_MULT 32
  #define MCLK_DIV  255
  #define MCLK_SRC  0
#elif F_CPU == 192000000
  #define MCLK_MULT 2
  #define MCLK_DIV  17
#elif F_CPU == 216000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
#elif F_CPU == 240000000
  #define MCLK_MULT 2
  #define MCLK_DIV  85
  #define MCLK_SRC  0
#elif F_CPU == 256000000
  #define MCLK_MULT 12
  #define MCLK_DIV  17
  #define MCLK_SRC  1
#else
  #error "This CPU Clock Speed is not supported by the Audio library";
#endif

#ifndef MCLK_SRC
#if F_CPU >= 20000000
  #define MCLK_SRC  3  // the PLL
#else
  #define MCLK_SRC  0  // system clock
#endif
#endif
#endif

void AudioOutputTDM_32::config_tdm() // argument ignored for now
{ 
#if defined(KINETISK)
	SIM_SCGC6 |= SIM_SCGC6_I2S;
	SIM_SCGC7 |= SIM_SCGC7_DMA;
	SIM_SCGC6 |= SIM_SCGC6_DMAMUX;

	// if either transmitter or receiver is enabled, do nothing
	if (I2S0_TCSR & I2S_TCSR_TE) return;
	if (I2S0_RCSR & I2S_RCSR_RE) return;

	// enable MCLK output
	I2S0_MCR = I2S_MCR_MICS(MCLK_SRC) | I2S_MCR_MOE;
	while (I2S0_MCR & I2S_MCR_DUF) ;
	I2S0_MDR = I2S_MDR_FRACT((MCLK_MULT-1)) | I2S_MDR_DIVIDE((MCLK_DIV-1));

	// configure transmitter
	I2S0_TMR = 0;
	I2S0_TCR1 = I2S_TCR1_TFW(4);
	I2S0_TCR2 = I2S_TCR2_SYNC(0) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(0);
	I2S0_TCR3 = I2S_TCR3_TCE;
	I2S0_TCR4 = I2S_TCR4_FRSZ(7) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSD;
	I2S0_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	// configure receiver (sync'd to transmitter clocks)
	I2S0_RMR = 0;
	I2S0_RCR1 = I2S_RCR1_RFW(4);
	I2S0_RCR2 = I2S_RCR2_SYNC(1) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(0);
	I2S0_RCR3 = I2S_RCR3_RCE;
	I2S0_RCR4 = I2S_RCR4_FRSZ(7) | I2S_RCR4_SYWD(0) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSD;
	I2S0_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

	// configure pin mux for 3 clock signals
	CORE_PIN23_CONFIG = PORT_PCR_MUX(6); // pin 23, PTC2, I2S0_TX_FS (LRCLK) - 44.1kHz
	CORE_PIN9_CONFIG  = PORT_PCR_MUX(6); // pin  9, PTC3, I2S0_TX_BCLK  - 11.2 MHz
	CORE_PIN11_CONFIG = PORT_PCR_MUX(6); // pin 11, PTC6, I2S0_MCLK - 22.5 MHz

#elif defined(__IMXRT1062__)
	CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

	// if either transmitter or receiver is enabled, do nothing
	if (I2S1_TCSR & I2S_TCSR_TE) return;
	if (I2S1_RCSR & I2S_RCSR_RE) return;
//PLL:
	int fs = AUDIO_SAMPLE_RATE_EXACT;
	// PLL between 27*24 = 648MHz und 54*24=1296MHz
	int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
	int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

	double C = ((double)fs * 256 * n1 * n2) / 24000000;
	int c0 = C;
	int c2 = 10000;
	int c1 = C * c2 - (c0 * c2);
	set_audioClock(c0, c1, c2);
	// clear SAI1_CLK register locations
	CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
		   | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4

	n1 = n1 / 2; //Double Speed for TDM

	CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
		   | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
		   | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

	IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1 & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
			| (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));	//Select MCLK

	// configure transmitter
	int rsync = 0;
	int tsync = 1;

	I2S1_TMR = 0;
	I2S1_TCR1 = I2S_TCR1_RFW(4);
	I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP | I2S_TCR2_MSEL(1)
		| I2S_TCR2_BCD | I2S_TCR2_DIV(0); 
	I2S1_TCR3 = I2S_TCR3_TCE;
	I2S1_TCR4 = I2S_TCR4_FRSZ(7) | I2S_TCR4_SYWD(0) | I2S_TCR4_MF
		| I2S_TCR4_FSE | I2S_TCR4_FSD;
	I2S1_TCR5 = I2S_TCR5_WNW(31) | I2S_TCR5_W0W(31) | I2S_TCR5_FBT(31);

	I2S1_RMR = 0;
	I2S1_RCR1 = I2S_RCR1_RFW(4);
	I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_TCR2_BCP | I2S_RCR2_MSEL(1)
		| I2S_RCR2_BCD | I2S_RCR2_DIV(0);
	I2S1_RCR3 = I2S_RCR3_RCE;
	I2S1_RCR4 = I2S_RCR4_FRSZ(7) | I2S_RCR4_SYWD(0) | I2S_RCR4_MF
		| I2S_RCR4_FSE | I2S_RCR4_FSD;
	I2S1_RCR5 = I2S_RCR5_WNW(31) | I2S_RCR5_W0W(31) | I2S_RCR5_FBT(31);

	CORE_PIN23_CONFIG = 3;  //1:MCLK
	CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
	CORE_PIN20_CONFIG = 3;  //1:RX_SYNC
#endif
}

#endif
