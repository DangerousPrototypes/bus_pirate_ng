
#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/adc.h>
#include "ADC.h"


void initADC(void)
{

#if(0)
rcc_periph_clock_enable(RCC_ADC1);
adc_power_off(ADC1);
rcc_periph_reset_pulse(RST_ADC1);
rcc_set_adcpre(RCC_CFGR_ADCPRE_PCLK2_DIV2);
adc_set_dual_mode(ADC_CR1_DUALMOD_IND);
adc_disable_scan_mode(ADC1);
adc_set_single_conversion_mode(ADC1);
adc_set_sample_time(ADC1, ADC_CHANNEL0, ADC_SMPR1_SMP_1DOT5CYC);
adc_enable_trigger(ADC1, ADC_CR2_EXTSEL_SWSTART);
adc_power_on(ADC1);
adc_reset_calibration(ADC1);
adc_calibration(ADC1);
adc_start_conversion_regular(ADC1);
while (! adc_eoc(ADC1));
reg16 = adc_read_regular(ADC1);
#endif

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


