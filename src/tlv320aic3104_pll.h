/*
	TLV320AIC3104
	PLL
	*** Only tested at 44100 in TDM (BCLK inverted) and I2S (BCLK normal) modes ***
	* This software is published under the MIT Licence
	* R. Palmer 2025
	
	V1.5:
	Changed setPll()
		added q parameter with default
		*** breaking change *** return value has changed to bool and inverted 	
*/

/* PLL Q is normally set to 2 when PLL is not used.
	 FsRef = 1/8 * CLK_IN * K * R / P
	 K = J.D			see p26.
	 in most cases K = 1 (J = 1, D = 0), P = 1; R = 8 will work (same outcome as Q = 2)
*/
unsigned long AudioControlTLV320AIC3104::setPllClkIn(long sampRate)
{
	// TDM and I2S BCLK frequencies are different, LRCLK frequencies are the same
	// Fs ref = K*R/8*P; K =J.D
	if (_i2sMode == AICMODE_TDM)
		switch (sampRate) // TDM
		{
			case 48000 : // p26
				_baseRate = 48000;
				pll.clk = 12288000; //BCLK 12.288 MHz
				pll.p = 1;
				pll.r = 1;
				pll.j = 8;
				pll.d = 0;
				break;
				
			case 44100 :
			default :
			_baseRate = 44100;
				pll.clk = 11289600; // 11.2896 MHz
				pll.p = 1;
				pll.r = 1;
				pll.j = 8;
				pll.d = 0;
		}
	else
		switch (sampRate) // I2S
		{
			case 48000 : 
				_baseRate = 48000;
				pll.clk = 1536000; // 15.36 MHz
				pll.p = 1;
				pll.r = 4;
				pll.j = 16;
				pll.d = 0;
				break;
				
			case 44100 :
			default :
			_baseRate = 44100;
				pll.clk = 1411200; // 14.112 MHz
				pll.p = 1;
				pll.r = 4;
				pll.j = 16;
				pll.d = 0;
		}
			
	return pll.clk;
}

// Set up pll variables, (q defaults to 2) but don't write changes to CODECs.
bool AudioControlTLV320AIC3104::setPll(uint32_t clk, uint32_t p, uint32_t r, uint32_t j, uint32_t d, uint32_t q)
{
		if(j > 63 || j < 1)
			return false;
		if(d  > 9999)
			return false;
		if(p > 8 || p < 1)
			return false;
		if(r > 16) 
			return false;
		if (q < 2 || q > 17)
			return false; 
		pll.clk = clk;
		if(p < 8)
			pll.p = p;
		else
			pll.p = 0;
		pll.j = j;
		pll.d = d;
		if(q < 16)
			pll.q = q;
		else
			pll.q = q - 16;
		setPllK();
		return true;
}

// Write  CODEC PLL values from struct pll and enable PLL if required.
// Call after setPll() or setPllClkIn()
void AudioControlTLV320AIC3104::enablePll(bool enabled, int codec)
{
	uint8_t r3 = (enabled) ? 0x80 : 0 | pll.q << 3 | pll.p;
	uint8_t r102 = (enabled) ? 0xA2 : 0x02; // p75 - BCLK for PLLCLK_IN and CLKDIV_IN 
	for(int i = 0; i < _codecs; i++) // All codecs use a common clock source
	{
			writeRegister(4, pll.j << 2, codec);
			writeRegister(5, (pll.d >> 6) & 0xff , codec); // 14-bit value across r5 & r6
			writeRegister(6, (pll.d << 2) & 0xff, codec);
			writeRegister(11, pll.r & 0x0f, codec);
			writeRegister(102, r102, codec);	// set clock sources to BCLK
			writeRegister(3, r3, codec); 			// Set PLL values and conditionally enable PLL
	}
}

aic_pll AudioControlTLV320AIC3104::getPll()
{
	return pll;
}

float AudioControlTLV320AIC3104::setPllK()
{
	float j=pll.j;
	float d=pll.d;
	pll.k = j+d/10000.0f;
	return pll.k;
}
unsigned long AudioControlTLV320AIC3104::getPllFsRef(void)
{
	return ((pll.clk / 2048.0f) * pll.k * pll.r)/(pll.p);
}
