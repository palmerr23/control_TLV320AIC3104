/* DAC biquad filters - derived from Teensy audio library biquad code
	https://github.com/PaulStoffregen/Audio/blob/master/filter_biquad.h
	
Note (RP):
	The transfer function is not standard Biquad (p32):
	(N0 + 2*N1*z1 + N2*z2)/(32768 - 2*D1*Z1 - D2 *Z2)
	
	TLV codecs double the value of N1 and D1, so they need to be set to half the normal value.
	D1 & D2 are also negative.
	
Note (PS):
	Biquad filters with low corner frequency (under about 400 Hz) can run into trouble 
	with limited numerical precision, causing the filter to perform poorly. 
	For very low corner frequency, the State Variable (Chamberlin) filter should be used.
*/

#define xscalex 32768.0 // 16-bits 

void AudioControlTLV320AIC3104::setHighpass(int stage, float frequency, float q, int8_t channel, int8_t codec) 
{
		int coef[5];
		frequency = frequency / 2.0;
		double w0 = frequency * (2.0f * 3.141592654f / AUDIO_SAMPLE_RATE_EXACT);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scalex = xscalex / (1.0 + alpha);
		// normal biquad calcs. Converted in setDACfilter().
		/* b0 */ coef[0] = ((1.0 + cosW0)/2 ) * scalex; 
		/* b1 */ coef[1] = -(1.0 + cosW0) * scalex; 
		/* b2 */ coef[2] = coef[0];
		/* a1 */ coef[3] = (-2.0 * cosW0) * scalex; 
		/* a2 */ coef[4] = (1.0 - alpha) * scalex; 
		/*if(_verbose > 1)
		{
			Serial.printf("+++Teensy Audio: Stage %i\n", stage);
			for(int i = 0; i < 5; i++)
				Serial.printf("%i: 0x%04X [%i]\n", i, coef[i], coef[i]); 	
		}
		*/
	setDACfilter(stage, coef, channel, codec);
}

void AudioControlTLV320AIC3104::setLowpass(int stage, float frequency, float q, int8_t channel, int8_t codec) 
{
		int coef[5];
		frequency = frequency / 2.0;
		double w0 = frequency * (2.0f * 3.141592654f / AUDIO_SAMPLE_RATE_EXACT);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scalex = xscalex / (1.0 + alpha);
		/* b0 */ coef[0] = ((1.0 - cosW0) / 2.0) * scalex;
		/* b1 */ coef[1] = (1.0 - cosW0) * scalex;
		/* b2 */ coef[2] = coef[0];
		/* a1 */ coef[3] = (-2.0 * cosW0) * scalex;
		/* a2 */ coef[4] = (1.0 - alpha) * scalex;
		setDACfilter(stage, coef, channel, codec);
	}	
	void AudioControlTLV320AIC3104::setBandpass(int stage, float frequency, float q, int8_t channel, int8_t codec) 
	{
		int coef[5];
		frequency = frequency / 2.0;
		double w0 = frequency * (2.0f * 3.141592654f / AUDIO_SAMPLE_RATE_EXACT); 
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scalex = xscalex / (1.0 + alpha);
		/* b0 */ coef[0] = alpha * scalex;
		/* b1 */ coef[1] = 0;
		/* b2 */ coef[2] = (-alpha) * scalex;
		/* a1 */ coef[3] = (-2.0 * cosW0) * scalex;
		/* a2 */ coef[4] = (1.0 - alpha) * scalex;
		setDACfilter(stage, coef, channel, codec);
	}
	void AudioControlTLV320AIC3104::setNotch(int stage, float frequency, float q, int8_t channel, int8_t codec) 
	{
		int coef[5];
		frequency = frequency / 2.0;
		double w0 = frequency * (2.0f * 3.141592654f / AUDIO_SAMPLE_RATE_EXACT);
		double sinW0 = sin(w0);
		double alpha = sinW0 / ((double)q * 2.0);
		double cosW0 = cos(w0);
		double scalex = xscalex / (1.0 + alpha);
		/* b0 */ coef[0] = scalex;
		/* b1 */ coef[1] = (-2.0 * cosW0) * scalex;
		/* b2 */ coef[2] = coef[0];
		/* a1 */ coef[3] = (-2.0 * cosW0) * scalex;
		/* a2 */ coef[4] = (1.0 - alpha) * scalex;
		setDACfilter(stage, coef, channel, codec);
	}
	void AudioControlTLV320AIC3104::setLowShelf(int stage, float frequency, float gain, float slope, int8_t channel, int8_t codec) 
	{
		int coef[5];
		frequency = frequency / 2.0;
		double a = pow(10.0, gain/40.0f);
		double w0 = frequency * (2.0f * 3.141592654f / AUDIO_SAMPLE_RATE_EXACT);
		double sinW0 = sin(w0);
		//double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
		double cosW0 = cos(w0);
		//generate three helper-values (intermediate results):
		double sinsq = sinW0 * sqrt( (pow(a,2.0)+1.0)*(1.0/(double)slope-1.0)+2.0*a );
		double aMinus = (a-1.0)*cosW0;
		double aPlus = (a+1.0)*cosW0;
		double scalex = xscalex / ( (a+1.0) + aMinus + sinsq);
		/* b0 */ coef[0] =		a *	( (a+1.0) - aMinus + sinsq	) * scalex;
		/* b1 */ coef[1] =  2.0*a * ( (a-1.0) - aPlus  			) * scalex;
		/* b2 */ coef[2] =		a * ( (a+1.0) - aMinus - sinsq 	) * scalex;
		/* a1 */ coef[3] = -2.0*	( (a-1.0) + aPlus			) * scalex;
		/* a2 */ coef[4] =  			( (a+1.0) + aMinus - sinsq	) * scalex;
		setDACfilter(stage, coef, channel, codec);
	}
	void AudioControlTLV320AIC3104::setHighShelf(int stage, float frequency, float gain, float slope, int8_t channel, int8_t codec) 
	{
		int coef[5];
		frequency = frequency / 2.0;
		double a = pow(10.0, gain/40.0f);
		double w0 = frequency * (2.0f * 3.141592654f / AUDIO_SAMPLE_RATE_EXACT);
		double sinW0 = sin(w0);
		//double alpha = (sinW0 * sqrt((a+1/a)*(1/slope-1)+2) ) / 2.0;
		double cosW0 = cos(w0);
		//generate three helper-values (intermediate results):
		double sinsq = sinW0 * sqrt( (pow(a,2.0)+1.0)*(1.0/(double)slope-1.0)+2.0*a );
		double aMinus = (a-1.0)*cosW0;
		double aPlus = (a+1.0)*cosW0;
		double scalex = xscalex / ( (a+1.0) - aMinus + sinsq);
		/* b0 */ coef[0] =		a *	( (a+1.0) + aMinus + sinsq	) * scalex;
		/* b1 */ coef[1] = -2.0*a * ( (a-1.0) + aPlus  			) * scalex;
		/* b2 */ coef[2] =		a * ( (a+1.0) + aMinus - sinsq 	) * scalex;
		/* a1 */ coef[3] =  2.0*	( (a-1.0) - aPlus			) * scalex;
		/* a2 */ coef[4] =  			( (a+1.0) - aMinus - sinsq	) * scalex;
		setDACfilter(stage, coef, channel, codec);
	}

// Custom biquad filter: which should be scaled to int16 in an int array - see the Teensy calculations above.
// order is N0, N1, N2, D1, D2 (D0 set in hardware)
void AudioControlTLV320AIC3104::setCustomFilter(int stage, const int *coefx, int8_t channel, int8_t codec) 
{
	//Serial.println("---Teensy: set Custom filter");
	setDACfilter(stage, coefx, channel, codec);
}


void AudioControlTLV320AIC3104::setFilterOff (int8_t channel, int8_t codec)
{
	setDACfilter(0, (int *)NULL, channel, codec);
}
void AudioControlTLV320AIC3104::setFlat(int stage, int8_t channel, int8_t codec)
{
	// Default TLV effects filter value are all pass, 0dB gain.
	int16_t coeff[5] = {0X6BE3, 0X9666, 0X675D, 0X7D83, 0X84EE};
	setTIBQFilter(stage, coeff, channel, codec);
}
// Custom biquad filter: as generated by TIBQ (in TLV register format).
// order is N0, N1, N2, D1, D2 (D0 set in hardware)
void AudioControlTLV320AIC3104::setTIBQFilter(int stage, const int16_t *coefx, int8_t channel, int8_t codec) 
{
	//Serial.println("---Teensy: set TIBQ filter");
	int co[5];
	// undo TIBQ coefficient changes, will be re-applied in setDACfilter( )
	co[0] = *coefx;
	co[1] = *(coefx+1) * 2;
	co[2] = *(coefx+2);
	co[3] = *(coefx+3) * -2;
	co[4] = *(coefx+4) * -1;
	setDACfilter(stage, co, channel, codec);
}

/* Left (R1:1 - 1:20) and right (R1:27 - 1:46) register sets 
	LB1 and LB2 registers are interleaved.
	Set filter processing on R0:12
*/
void AudioControlTLV320AIC3104::setDACfilter(int stage, const int *coef, int8_t channel, int8_t codec)
{
	bool setOn = (coef != NULL);
	int16_t coefx[5] = {0,0,0,0,0};
	int cst, cend;
	uint8_t r12;
	stage = constrain(stage, 0, 1); // [0..1]
	if(_verbose > 1)
	{	
		fprintf(stderr, "Teensy Audio setDACfilt: Stage %i, setOn %i, for channel(s) %i\n", stage, setOn, channel);
		if(setOn) // coef == NULL for disable
			for(int i = 0; i < 5; i++)
				fprintf(stderr, "%i: 0x%04X [%i]\n", i, *(coef+i), *(coef+i));
	}
	/* convert coefficients to TI 16-bit model (see above) */
	if(setOn)
	{
		coefx[0] = *coef;
		coefx[1] = *(coef+1) /2;
		coefx[2] = *(coef+2);
		coefx[3] = *(coef+3) / -2;
		coefx[4] = *(coef+4) * -1;
	}
	if(codec < 0)
	{
		cst = 0;
		cend = _codecs;
	}
	else
	{
		cst = codec;
		cend = cst + 1;
	}

	//uint8_t val = 0x30 | (channel & 0x03) << 6;	// top two bits + reserved (p77)
	(_verbose > 1) && fprintf(stderr, "DAC effects filters for codecs %i < %i, channel %i \n",  cst, cend, channel);
	// execute in CODEC order to avoid mux switching delay
	for(int cod = cst; cod < cend; cod++)
	{
		r12 = readRegister(12, cod);	
		r12 = r12 & (AIC_R12_HPF_MASK | AIC_R12_DEMPH_MASK); // get other filter status
		writeRegister(12, r12, cod); // turn off DAC effects filter before changing parameters. Leave ADC HPF and DAC de-emph alone.
		if(setOn) // only need to program coefficients if filter is being turned on 
		{
			setRegPage(1, cod); // DAC effects coefficient registers are on Reg Page 1
			// Left (R1:1 - 1:20) and right (R1:27 - 1:46) register sets 
			// LB1 and LB2 registers are interleaved.
			// LB1 : N0, N1, N2, D1, D2 (D0 set in hardware)
			// LB2 : N3, N4, N5, D4, D5 (D3 set in hardware)
			//Serial.printf("Stage %i\n", stage);
			for(int i = 0; i < 3; i++) // N registers
			{
				if(channel & 1) // left N
				{	// high and low bytes - MSB first
				 // Serial.printf("LN %i(R%i)\n", i, 1+2*i + stage * 6); // R1:1..6 & R1:7..12
					writeRegister(1+2*i + stage * 6, (coefx[i] >> 8) & 0xff, cod); 
					writeRegister(2+2*i + stage * 6, coefx[i] & 0xff, cod);
				}
				if(channel & 2) //right N
				{
					//Serial.printf("RN %i(R%i)\n",i,  27+2*i + stage * 6); // R1:27..32 & R1:33..38
					writeRegister(27+2*i + stage * 6, (coefx[i]>> 8) & 0xff, cod); 
					writeRegister(28+2*i + stage * 6, coefx[i] & 0xff, cod);
				}
			}
			for(int i = 0; i < 2; i++) // D registers
			{
				if(channel & 1) // left D
				{	// high and low bytes - MSB first
				 // Serial.printf("LD %i(%i)\n", i, 13+2*i + stage * 4); // R1:13..16 & R1:17..20
					writeRegister(13+2*i + stage * 4, (coefx[i+3] >> 8) & 0xff, cod); 
					writeRegister(14+2*i + stage * 4, coefx[i+3] & 0xff, cod);
				}
				if(channel & 2) // right d
				{
					// Serial.printf("LD %i(%i)\n", i,39+2*i + stage * 4); // R1:39..42 & R1:43..46
					writeRegister(39+2*i + stage * 4, (coefx[i+3] >> 8) & 0xff, cod); 
					writeRegister(40+2*i + stage * 4, coefx[i+3] & 0xff, cod);
				}
			}
			setRegPage(0, cod); // back to Page 0
			//Serial.println(" - DONE");
		}


		// Leave ADC HPF and DAC de-emph filters alone
		if(setOn)
		{ 
			uint8_t mask = ((channel & 1) ? 0x08 : 00) | ((channel & 2) ? 0x02 : 00);
			writeRegister(12, mask | r12, cod); // turn on DACeffects
			//Serial.printf("On Ch 0x%02x, Mask 0x%02X\n", channel, mask);
		}
		else 
		{
			// DAC filters are already off
			// writeRegister(12, r12, cod); // turn off DAC effects
			//Serial.printf("Off R12 0x%02X\n", r12);
		}
	}
}

#define FILTERREGS 10 // DACD effects filter register pairs
char regs[FILTERREGS][10] ={"S0:N0    ", "S0:N1    ", "S0:N2    ", "S0:D1    ", "S0:D2    ", "S1:N0(N3)", "S1:N1(N4)", "S1:N2(N5)", "S1:D1(D3)", "S1:D2(D4)"};
uint8_t regOrder[FILTERREGS] = {1, 3, 5, 13, 15,   7, 9, 11, 17,19}; // (channel 0: N0 N1, N2, D1, D2 channel 1: ...) offset by 26 for Right channel filters

void AudioControlTLV320AIC3104::printDACfilters(int8_t channels, int8_t codec)
{
	int cst, cend, i;
	if(codec < 0)
	{
		cst = 0;
		cend = _codecs;
	}
	else
	{
		cst = codec;
		cend = cst + 1;
	}
	for(int cod = cst; cod < cend; cod++)
	{
		setRegPage(1, cod);
		if (channels & 1)
		{
			Serial.println("Left");
			for(i = 0; i < FILTERREGS; i++)
			{
				uint16_t val = readRegister(regOrder[i], cod) << 8 | readRegister(regOrder[i]+1, cod);
				Serial.printf("%s [R%2i]: 0x%04X\n", regs[i], regOrder[i], val);
			}
		}
		if (channels & 2)
		{
			Serial.println("Right");
			for(i = 0; i < FILTERREGS; i++)
			{
				uint16_t val = readRegister(regOrder[i]+ 26, cod) << 8 | readRegister(regOrder[i]+27, cod);
				Serial.printf("%s [R%2i]: 0x%04X\n", regs[i], regOrder[i]+26, val);
			}
		}		
		setRegPage(0, cod);
		uint8_t val = readRegister(12, cod);
		Serial.printf("R12: 0x%02X\n", val);
	}	
}
/*
Default effects filter register values
N0:S0: 0x6be3
N1:S0: 0x9666
N2:S0: 0x675d
N0:S1: 0x6be3
N1:S1: 0x9666
N2:S1: 0x675d
D0:S0: 0x7d83
D1:S0: 0x84ee
D0:S1: 0x7d83
D1:S1: 0x84ee

TIBQ test filters - direct register values
Butterworth 2, 200Hz, HPF
N0, N1, N2, D1, D2
0x7D71
0x828F
0x7D71
0x7D6A
0x8510

Butterworth 2, 1000Hz, HPF
N0, N1, N2, D1, D2
0x73B9
0x8C47
0x73B9
0x7323
0x975E

Butterworth 2, 200Hz, LPF
N0, N1, N2, D1, D2
0x0006
0x0006
0x0006
0x7D6A
0x8510

Butterworth 2, 400Hz, Notch
0x7FA1
0x806C
0x7FA1
0x7F94
0x80BC

*/