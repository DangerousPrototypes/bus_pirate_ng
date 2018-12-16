
#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include "ADC.h"

//initializes and calibrates the ADC 
void initADC(void)
{
	//enable adcclock
	rcc_periph_clock_enable(BP_ADC_CLK);

	// adc needs to be off whiel configuring
	adc_power_off(BP_ADC);

	// setup for single shot
	adc_disable_scan_mode(BP_ADC);
	adc_set_single_conversion_mode(BP_ADC);
	adc_disable_external_trigger_regular(BP_ADC);

	// right aligned results
	adc_set_right_aligned(BP_ADC);

	//adc_enable_temperature_sensor();

	// set conversion time (xxms)
	adc_set_sample_time_on_all_channels(BP_ADC, ADC_SMPR_SMP_1DOT5CYC);

	// power on after onversion
	adc_power_on(BP_ADC);

	// calibrate the adc
	adc_reset_calibration(BP_ADC);
	adc_calibrate(BP_ADC);
}

// get the raw value from channel chan
uint16_t getADC(uint8_t chan)
{
	uint8_t channels[16];
	
	// lets do 1 chan
	channels[0] = chan;

	// set the sequence of 1
	adc_set_regular_sequence(BP_ADC, 1, channels);

	// start single conversion
	adc_start_conversion_direct(BP_ADC);

	// wait for result
	while (! adc_eoc(BP_ADC));

	return ADC_DR(BP_ADC);
}


// calculates the voltage
// hi=0 no voltage divider 0 .. 3V3
// hi=1 1/2 voltage divider 0 .. 6V6
// output is in mV
uint16_t voltage(uint8_t chan, uint8_t hi)
{
	uint32_t voltage;
	uint32_t temp;

	temp=getADC(chan);
	if(hi)
		voltage=6600*temp;
	else
		voltage=3300*temp;
	voltage/=4096;

	return voltage;
}



