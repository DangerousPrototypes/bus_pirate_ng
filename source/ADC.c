
#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include "ADC.h"

//initializes and calibrates the ADC 
void initADC(void)
{
	//enable adcclock
	rcc_periph_clock_enable(BPADCCLK);

	// adc needs to be off whiel configuring
	adc_power_off(ADC1);

	// setup for single shot
	adc_disable_scan_mode(BPADC);
	adc_set_single_conversion_mode(BPADC);
	adc_disable_external_trigger_regular(BPADC);

	// right aligned results
	adc_set_right_aligned(BPADC);

	//adc_enable_temperature_sensor();

	// set conversion time (xxms)
	adc_set_sample_time_on_all_channels(ADC1, ADC_SMPR_SMP_1DOT5CYC);

	// power on after onversion
	adc_power_on(BPADC);

	// calibrate the adc
	adc_reset_calibration(BPADC);
	adc_calibrate(BPADC);
}

// get the raw value from channel chan
uint16_t getADC(uint8_t chan)
{
	uint8_t channels[16];
	
	// lets do 1 chan
	channels[0] = chan;

	// set the sequence of 1
	adc_set_regular_sequence(BPADC, 1, channels);

	// start single conversion
	adc_start_conversion_direct(BPADC);

	// wait for result
	while (! adc_eoc(BPADC));

	return ADC_DR(BPADC);
}


// calculates the voltage
// hi=0 no voltage divider 0 .. 3V3
// hi=1 1/2 voltage divider 0 .. 6V6
float voltage(uint8_t chan, uint8_t hi)
{
	float voltage;
	uint32_t temp;

	temp=getADC(chan);
	if(hi)
		voltage=6.6*temp;
	else
		voltage=3.3*temp;
	voltage/=4096;

	return voltage;
}



