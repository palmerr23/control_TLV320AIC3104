/*
	TLV320AIC3104
	ADC High pass filters
	* This software is published under the MIT Licence
	* R. Palmer 2025
*/

// https://github.com/BelaPlatform/Bela/blob/master/core/I2c_Codec.cpp
// Enable ADC DC offset removal in CODEC hardware (High-Pass filter)
// Datasheet reference: TLV320AIC3104, SLAS510F - FEB 2007, Rev DEC 2016
// CODEC 1-pole Filter.  Datasheet calls it HPF but it can be any 1-pole filter
// realizable by the following the transfer function:
//           n0 + n1 * z^-1
// H(z) =  ------------------    (10.3.3.2.1 - pg 26)
//           d0 - d1 * z^-1
// d0 =  32768 //defined by hardware (presumably fixed-point unity)
// For the high-pass filter case (bilinear transform method),
// d1 = n0 = -n1
// d1 = d0*e^(-2*pi*fc/fs) = 32768*e^(-2*pi*8/44100) = 32730.7 // (8Hz / 44.1kHz) round up
// d1 =  32731 = 0x7FDB
// n0 =  32731 = 0x7FDB
// n1 =  -32731 = 0x8025 //2s complement

// three coefficient biquad - d0 is fixed in hardware, as above
struct bi_quad {
	int freq;
	uint8_t coeff[6];	// N0 MSB, N0 LSB, N1 MSB, N1 LSB, D1 MSB, D1 LSB
};

// N0, N1 don't change, only D1 is calculated.
bi_quad bb = {0, {0x7f, 0xDB, 0x80, 0x25, 0x00, 0x00}};

/*
ADC HPF
Set registers R1:65-70 and 71-76
disable: freq < 1 
*/
void AudioControlTLV320AIC3104::adcHPF(int freq, int8_t channel, int8_t codec)
{
	int cst, cend;
	uint8_t r12;
	freq = constrain(freq, 0, AIC_HPF_UPPER); 
	double dCalc = 32768.0 * pow(2.71828, -2 * PI * freq / _sampleRate) + 0.5; // round up
	int d1 = dCalc;
	bb.freq = freq;
	bb.coeff[4] = (d1 & 0xff00) >> 8;
	bb.coeff[5] = (d1 & 0xff);
	if(_verbose > 1)
	{
		fprintf(stderr, "BQ D1 (N0, N1 don't change): ");
		for(int i = 4; i < 6; i++)
			fprintf(stderr, "0x%02x ", bb.coeff[i]);
		fprintf(stderr, "\n");
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

	uint8_t val107 = 0x30 | ((channel < 0 || !channel) ? 0x80 : 0) | ((channel) ? 0x40 : 0);	// top two bits + reserved (p77)
	(_verbose > 1) && fprintf(stderr, "%s ADC HPF, freq %i for codecs %i to %i, channel %i, R107 0x%2x\n", (freq > 1) ? "ENABLE" : "DISABLE", freq, cst, cend, channel, val107);
	// execute in CODEC order to avoid mux switching delay
	for(int cod = cst; cod < cend; cod++)
	{
		r12 = readRegister(12, cod);	// get DAC effects filter status
		r12 = r12 & (AIC_R12_EFF_MASK | AIC_R12_DEMPH_MASK);
		writeRegister(12, r12, cod); // turn off HPF before changing parameters. Leave DAC effects and de-emph alone.
		if(freq > 0) // only need to program coefficients if HPF is being turned on 
		{
			setRegPage(1, cod); // ADC HPF coefficient registers are in Reg Page 1
			for(int i = 0; i < 6; i++)
			{
				if(channel < 0 || !channel)
				{
					writeRegister(65+i, bb.coeff[i], cod); 
					//Serial.print("$");
				}
					
				if(channel)
				{
					writeRegister(71+i, bb.coeff[i], cod); 
					//Serial.print("C");
				}
			}
			setRegPage(0, cod); // back to Page 0
		}

		writeRegister(107, val107, cod); // use coefficients instead of defaults (p29)
		// Leave DAC effects and de-emph filters alone
		if(freq > 0)
			writeRegister(12, 0xf0 | r12, cod); // turn on HPF
		else 
			writeRegister(12, r12, cod); // turn off HPF
	}
}
