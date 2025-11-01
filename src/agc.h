// AGC code: automatically change PGA gain
// channel = 0 -> left; channel == 1 -> right; channel = -1 -> both
// all parameters set at once
// See datasheet Reg 26-29 definitions for values
// automatically enabled. May be enabled/disabled using AGCenable()
bool AudioControlTLV320AIC3104::AGC(int8_t targetLevel, int8_t attack, int8_t decay, float maxGain,  uint8_t hysteresis,  float noiseThresh, bool clipStep, int8_t channel, int8_t codec)
{
	uint8_t start, end, rA, rB, rC;
	int nt, mg;

	if(codec < 0)
	{
		start = 0;
		end = _codecs;
	}
	else
	{
		start = codec;
		end = codec + 1;
	}
	// AGC max gain is (2 * dB)
	mg = maxGain * 2;
	// AGC Noise Threshold is (dB-28)/2. 0 is OFF
	nt = (-noiseThresh - 28)/2;
	// enable AGC
	rA = 0x80 | ((targetLevel & 0x7) << 4) | ((attack & 0x3) << 2) | (decay & 0x3);
	rB = (mg & 0x7f) << 1;
	rC = ((hysteresis & 0x3) << 6) | ((nt & 0x3f) << 1) | ((clipStep) ? 1 : 0);
	//Serial.printf("AGC channel %i, rA 0x%02X, rB 0x%02X, rC 0x%02X\n", channel, rA, rB, rC); 

	for(int i = start; i < end; i++)
	{
		// set correct ACG channels
		if(channel < 0 || !channel) // L
		{
			writeRegister(26, rA, i);
			writeRegister(27, rB, i);
			writeRegister(28, rC, i);
			//Serial.printf("AGC write L %i\n", i);
		}
		if(channel) // R
		{
			writeRegister(29, rA, i);
			writeRegister(30, rB, i);
			writeRegister(31, rC, i);
			//Serial.printf("AGC write R %i\n", i);
		}
	}
	return true;
}

// enable or disable AGC
// settings are not disturbed
// needs to be 
bool AudioControlTLV320AIC3104::AGCenable(bool enable, int8_t channel, int8_t codec)
{
	uint8_t start, end, xVal;

	if(codec < 0)
	{
		start = 0;
		end = _codecs;
	}
	else
	{
		start = codec;
		end = codec + 1;
	}

	for(int i = start; i < end; i++)
	{
		xVal = readRegister(7, i) & 0x7f;
		writeRegister(7, xVal + 0x80, i);	// set AGC timing to 44.1kHz values
		
		if(channel < 0 || !channel)
		{
			xVal = readRegister(26, i) & 0x7f;
			writeRegister(26, xVal + (enable)? 0x80 : 0, i);
		}
		if(channel)
		{
			xVal = readRegister(29, i) & 0x7f;
			writeRegister(29, xVal + (enable)? 0x80 : 0, i);
		}
	}
	return true;
}