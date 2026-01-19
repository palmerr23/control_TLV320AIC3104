# Teensy Audio TLV320AIC3104 8x8 CODEC Board Control library

Discussions have been enabled on the associated hardware repository for questions or suggestions about this hardware and software https://github.com/palmerr23/Teensy8x8AudioBoard/discussions

A Teensy Audio Control library for the Texas Instruments TLV320AIC3104 stereo channel CODEC. This CODEC is TDM-compatible as can be programmed to offset its read/write slots to anywhere in a 256 slot TDM cycle and put the DO line in a Hi-Z state when not transmitting.

The library was created specifically for the associated four-CODEC Teensy Audio board (https://github.com/palmerr23/Teensy8x8AudioBoard). As the AIC320 only supports a single I2C address, there is some code to ensure the correct CODEC is being programmed via a multiplexed I2C bus.

The library contains drivers for the PCA9546 I2C multiplexer.

It can also support a single AIC3104 in I2S mode.

Page references are to the TLV320AIC3104 datasheet, Feb 2021.

V1.3

## Compatibility
Arduino 4.x with Teensyduino 1.59 or later and the supplied TDMA driver.

Compatibile with OpenAudioLib F32 as of June 2025, except F32 USB drivers (see example).

## Single CODEC I2S operation 
I2S mode is the default for single codecs, however TDM may be selected using the i2sMode argument.

In I2S mode, the first CODEC on the mux with the lowest I2C address will be programmed. Other CODECs present will be reset on enable().

In I2S mode, avoid the more complex forms of the various function calls, accepting the default arguments where they are available.

The companion 8x8 Audio Board will not operate with only one CODEC enabled, as the I2C multiplexer is disabled. Two or more CODECS should be enabled on this board.

## TDM operation with multiple CODECs
With the TDMA drivers supplied with this library, two stacked boards are practical, providing 16x16 operation.

_As of Teensyduino 1.59, EVEN channel samples are not transferred correctly by the Teensy Audio TDM driver. The TDMA driver supplied with this library corrects this issue._

## Multiple boards
There are three on-board jumpers to theoretically allow up to eight muxes (boards) to be stacked with a single Teensy. 

Boards may have any address within the range. Probing on start-up will assign audio channel TDM slots in increasing order of discovered mux addresses.

For higher bit-depths and sample rates, the OpenAudioLib (F32) library may be used. The input_tdm32 and output_tdm32 TDM drivers should be used with OpenAudioLib. https://github.com/chipaudette/OpenAudio_ArduinoLibrary

While up to eight boards may theoretically be stacked, there is a practical limit with the standard Teensy Audio Library TDM driver which provides a 16 x 16-bit duplex TDM interface - that is two 8x8 boards. (Note 
Alternate Audio libraries, such as that managed by Jonathan Oakley https://github.com/h4yn0nnym0u5e/Audio/tree/feature/multi-TDM, provide the ability to support more channels.

There is a discussion on enabling more than 16x16 channels in the forum: https://forum.pjrc.com/index.php?threads/updated-8x8-and-16x16-audio.75569/

At a hardware level, there are also jumpers on the PCB to allow alternate DI/DO pins to be used. This feature is untested and the Teensy may not be able to drive more than two boards simultaneously, due to reflections and distortion of the high frequency signals by multiple long PCB tracks. 

Please read the hardware notes in this repo before attempting to stack more than two boards or use alternate DI/DO pins.

## Available Hardware

The companion hardware is described at https://github.com/palmerr23/Teensy8x8AudioBoard

# Function Reference

The TDMA driver operates in the same way as the standard Teensy Audio TDM driver. A 'fixed' TDM2 driver is available in the multi-TDM library (see ablove).

### Constructor

For 8x8 or 16x16 TDM operation the simplest form is: 

```
AudioControlTLV320AIC3104 aic(n, true, AICMODE_TDM); // (n > 1)
```

The full form should be used when using TDM mode or different sampleRates/sampleLengths with the OpenAudio Library:

```
AudioControlTLV320AIC3104 aic(uint8_t codecs = 1, bool useMCLK = true, uint8_t i2sMode = AICMODE_I2S,  long sampleRate = 44100, int sampleLength = 16);
```

The simplest form for a single codec defaults to I2S mode with standard teensy audio parameters:

```
AudioControlTLV320AIC3104 aic( );
```

### AudioMemory( )

Two AudioMemory blocks are required for each provisioned input or output for stable operation. This is independent of the number of channels with active patchcords.

Thus, a single board (4 CODECS) will require 16 audio blocks to be allocated for the 8 input and output buffers.

### Wire

Defining and initialising the Wire library is the responsibility of the user application. 

This must be completed before begin( ) is called.

### begin( )

Resets the CODECs and muxes and then probes for muxes.

begin( ) may be called with no arguments if the standard reset pin (22) is used.

### enable( )

Called without arguments, enable( ) configures all attached CODEC control registers.

enable(codec) may be useful when debugging hardware.

## ADC
### inputMode(inputModes mode, int8_t channel, int8_t codec)
### inputMode(inputModes mode, int8_t codec) {both channels set}

Set single-ended (AIC_SINGLE) or differential (AIC_DIFF) input mode.

When called before enable( ), inputMode sets the default input mode and the second argument is ignored.

When called after enable( ), all CODECS (codec = -1) or a single CODEC may be affected.

Channel = 0 -> left; channel == 1 -> right; channel > 1 -> both

In differential mode '-' inputs should be grounded for unbalanced signals to reduce noise.

When differential input signals are connected in single-ended mode, crosstalk will occur between channels due to the CODEC's internal multiplexing (see Fig 10-13, p36).

The library defaults to differential mode to avoid this issue, at a small cost to noise performance.

### inputSelect(int level, int8_t channel, int8_t codec)

The balanced inputs (L1) are used for both mic and line inputs. PGA gain is adjustable between 0 and 59.5 dB.

Compliant with the Teensy audioControl standard and the SGTL5000 implementation, this sets an input to either MIC (-59.5 dB [-62.5 dBm]) or LINE (0 dB [-2.5 dBm]).

For finer gain control, use inputLevel( ).

### inputLevel(float gainVal, int8_t channel, int8_t codec) 

and

### gain(float gainVal, int8_t channel, int8_t codec)

The two functions are equivalent, setting the maximum input level or gain of an input channel. 

The CODEC's balanced inputs (LINE1Lx and LINE1Rx) are used for both mic and line inputs. PGA gain is adjustable between 0 and 59.5 dB.

For inputLevel( ) the range is -59.9 to 0 dB.

For gain( ) the range is 0 to 59 dB.
 
Values outside these ranges are constrained. 

When called without channel and codec arguments, all codecs and channels are affected. 

### pad(float pad, int8_t channel, int8_t codec)
Enable the ADC input pad.

Value from 0 to -12 in 1.5 dB steps

### adcHPF(int frequency, int8_t channel = -1, int8_t codec = -1)
HPF frequencies may be set between 1Hz and 5kHz.

Less than 1 Hz will turn the HPF off.

The effectiveness of HPF frequencies less than 10Hz is untested.

When called without channel and codec arguments, all codecs and channels are affected. 

channel is a bit map with 1 = left, 2 = right, 3 = both channels.

The function should be called after the CODEC is enabled.

### setHPF(uint8_t option, int8_t channel = -1, int8_t codec = -1) -- DEPRECATED
Input channel DC removal filter. These standard digital filter settings are not very useful for audio use, due to the high corner frequencies.

```
0 = off	- power on default
1 = 0.0045 Fs (198 Hz @ Fs = 44.1kHz) 
2 = 0.0125 Fs (551 Hz)
3 = 0.025  Fs (1102 Hz)
```
###  AGC(int8_t targetLevel, int8_t attack, int8_t decay, float maxGain, uint8_t hysteresis, float noiseThresh, bool clipStep, int8_t channel, int8_t codec)
Set and enable AGC.

See TLV320AIC3104 datasheet: registers 26-29 for settings.

### AGCenable(bool enable, int8_t channel, int8_t codec);
Enable or disable AGC without changing other AGC settings.
	
## DAC
### volume(float value, int8_t channel, int8_t codec)
Sets the volume of an output channel. The range is 0.0 .. 1.0 
 
When called without channel and codec arguments, all codecs and channels are affected.

For most applications, using other means to control the output level is preferable to changing the default volume level using this function, due to the increase in digital noise.

## DAC effects filters
The LTV320AIC3104 has two cascaded BiQuad filters available for each DAC channel.

The filters below follow the Teensy Audio Library BiQuad filter format, wherever appropriate.

Both stages are enabled or disabled together for each channel - see p32.

When called without channel and codec arguments, all codecs and channels are affected. 

The channel argument is a bit map with 1 = left, 2 = right, 3 or -1 = both channels.

The functions should be called after the CODEC is enabled.

Filters with gain must have their input signals attenuated, so the signal does not exceed 1.0

This object implements up to 2 cascaded stages. As both cascaded BiQuad filters are enabled together, the parameters of both sections should be set before enabling. The hardware default settings may have strange results. 

Biquad filters with low corner frequencies (under about 400 Hz) can run into trouble with limited numerical precision, causing the filter to perform poorly. For very low corner frequency, use the Audio Library's State Variable (Chamberlin) filter.

Q > 2 can be problematic for some filter types. For greater depth, cascade two filters.

### setFlat(int stage, int8_t channel = -1, int8_t codec = -1)
Set one stage of the filter (0 or 1) with flat (all-pass) response.

This is the same as the default value when the CODEC is reset.

### setHighpass(int stage, float frequency, float q = 0.7071, int8_t channel = -1, int8_t codec = -1);
Configure a stage of the filter (0 or 1) with high pass response, with the specified corner frequency and Q shape. If Q is higher that 0.7071, be careful of filter gain (see above).

### setLowpass(int stage, float frequency, float q = 0.7071f, int8_t channel = -1, int8_t codec = -1);
Configure a stage of the filter (0 or 1) with low pass response, with the specified corner frequency and Q shape. If Q is higher that 0.7071, be careful of filter gain (see above).

### setBandpass(int stage, float frequency, float q = 1.0, int8_t channel = -1, int8_t codec = -1);
Configure a stage of the filter (0 or 1) with band pass response. The filter has unity gain at the specified frequency. 

Q controls the width of frequencies allowed to pass. 

### setNotch(int stage, float frequency, float q = 1.0, int8_t channel = -1, int8_t codec = -1);
Configure a stage of the filter (0 or 1) with band reject (notch) response. Q controls the width of rejected frequencies. Lower Q = wider frequency range, deeper notch.

### setLowShelf(int stage, float frequency, float gain, float slope = 1.0f, int8_t channel = -1, int8_t codec = -1); 
Configure a stage of the filter (0 or 1) with low shelf response. A low shelf filter attenuates or amplifies signals below the specified frequency. 

Frequency controls the slope midpoint, gain is in dB and can be both positive or negative. 

The slope parameter controls steepness of gain transition. A slope of 1 yields maximum steepness without overshoot, lower values yield a less steep slope. 

See the picture below for a visualization of the slope parameter's effect. 

Be careful with positive gains and slopes higher than 1 as they introduce gain (see warning below).

### setHighShelf(int stage, float frequency, float gain, float slope = 1.0f, int8_t channel = -1, int8_t codec = -1);

Configure a stage of the filter (0 or 1) with high shelf response. A high shelf filter attenuates or amplifies signals above the specified frequency. 

Frequency controls the slope midpoint, gain is in dB and can be both positive or negative. 

The slope parameter controls steepness of gain transition. A slope of 1 yields maximum steepness without overshoot, lower values yield a less steep slope. 

See the picture below for a visualization of the slope parameter's effect. 

Be careful with positive gains and slopes higher than 1 as they introduce gain (see warning below).

![Shelf filter characteristics](images/shelf_filter.png)

### setCustomFilter(int stage, const int *coefx, int8_t channel = -1, int8_t codec = -1) 
A custom  biquad filter: which should be scaled to int16 in an int array.

The order is N0, N1, N2, D1, D2 (D0 set in hardware)

### setTIBQFilter(int stage, const int16_t *coefx, int8_t channel = -1, int8_t codec = -1) 
A custom  biquad filter as generated by TIBQ.

The order is N0, N1, N2, D1, D2 (D0 set in hardware).

## Hardware validation and debugging

### setVerbose(int verbosity)
Sets the level of messages on stderr. 

```
0: turns messages off.

1: will provide some error messages on startup if the hardware doesn't match the supplied number of CODECs, etc.

2: provides additional messages as hardware is enabled or values changed 
```

Should be left at the default (0) for production, as the writes may block execution if USB isn't connected.

### listMuxes( )
Useful for checking that the board jumpers are set as required.

Should be called after begin( ), where the muxes are probed and recorded.

## Examples
- Basic operation 
- Dynamic patching of inputs and outputs
- DAC filter capabilities
- A matrix mixer: all inputs * all outputs 
- USB connectivity
- Network transport using the VBAN protocol https://vb-audio.com/Voicemeeter/vban.htm
- OpenAudioLib F32 
- OpenAudioLib F32_ USB incompatibility workaround
- AGC (Compressor)

## CPU Load

CPU load for the basic TDM transfers is low. 

* 16 output channels driven by a single sine generator, and no inputs processed consumes 0.12% CPU on a T4.0

* Adding 2 channels of peak readings increases it to 0.35%.

* 8 channels = 0.66%

* 16 channels = 1.1%



