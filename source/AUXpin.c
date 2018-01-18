

#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include "UI.h"
#include "AUXpin.h"

static uint32_t overflows;


// init the AUXpin to input
void initAUX(void)
{
	gpio_set_mode(BPAUXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPAUXPIN);
}

// set or clear AUX pin
void setAUX(uint8_t state)
{
	gpio_set_mode(BPAUXPORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPAUXPIN);
	
	if(state)
		gpio_set(BPAUXPORT, BPAUXPIN);
	else
		gpio_clear(BPAUXPORT, BPAUXPIN);

}

// returns the level on AUX pin
uint8_t getAUX(void)
{
	gpio_set_mode(BPAUXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPAUXPIN);

	return (gpio_get(BPAUXPORT, BPAUXPIN)?1:0);
} 


// output a PWM on the AUX pin (1 count=1/36000000s)
// if period=0 PWM is shutdown
void setPWM(uint32_t period, uint32_t oc)
{
	if(period!=0)
	{
		rcc_periph_clock_enable(RCC_TIM1);				// enable clock
		gpio_set_mode(BPPWMPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL , BPPWMPIN);	//output pushpull
		timer_reset(BPPWMTIMER);					// reset peripheral
		timer_set_mode(BPPWMTIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);	// count up
		timer_set_oc_mode(BPPWMTIMER, BPPWMCHAN, TIM_OCM_PWM1);		// PWM1 == high/low; PWM2= low/high
		timer_enable_oc_output(BPPWMTIMER, BPPWMCHAN);			// output channel
		timer_enable_break_main_output(BPPWMTIMER);			// need to set break
		timer_set_oc_value(BPPWMTIMER, BPPWMCHAN, oc);			// set match value
		timer_set_period(BPPWMTIMER, period);				// set period 
		timer_enable_counter(BPPWMTIMER);				// enable the timer
		modeConfig.pwm=1;

	}
	else
	{
		rcc_periph_clock_disable(RCC_TIM1);
		gpio_set_mode(BPPWMPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPPWMPIN);	
		timer_disable_counter(BPPWMTIMER);
		modeConfig.pwm=0;
	}


}

// frequency counter
uint32_t getfreq(void)
{
	uint32_t counts;

	rcc_periph_clock_enable(BPFREQCLK);
	gpio_set_mode(BPFREQPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPFREQPIN);	// RA0 (prolly set this way already after reset
	timer_reset(BPFREQTIMER);
	timer_disable_preload(BPFREQTIMER);
	timer_continuous_mode(BPFREQTIMER);					// keeps running
	timer_set_period(BPFREQTIMER, 65535);
	timer_slave_set_mode(BPFREQTIMER, TIM_SMCR_SMS_ECM1);			// count external ticks
	timer_slave_set_filter(BPFREQTIMER, TIM_IC_OFF);			// no filter
	timer_slave_set_polarity(BPFREQTIMER, TIM_ET_RISING);			// rising edge 
	timer_slave_set_prescaler(BPFREQTIMER, TIM_IC_PSC_OFF);			// dont prescale
	timer_slave_set_trigger(BPFREQTIMER, TIM_SMCR_TS_ETRF);			// use etr input

	// timer is 16 bit so we need to capture overflows
	overflows=0;
	timer_update_on_overflow(BPFREQTIMER);					// we want to see overflows
	nvic_enable_irq(BPFREQNVIC);						// enable timer irq
	timer_enable_irq(BPFREQTIMER, TIM_DIER_CC1IE);
	timer_enable_counter(BPFREQTIMER);					// start counting

	delayms(1000);								// we want to count just 1s

	counts = timer_get_counter(BPFREQTIMER) + overflows*65536;		// counts is the frequency 

	rcc_periph_clock_disable(BPFREQCLK);					// turn peripheral off
	timer_disable_counter(BPFREQTIMER);
	nvic_disable_irq(BPFREQNVIC);

	return counts;
}

// timer isr count the number of overflows
void tim2_isr(void)
{
	if (timer_get_flag(TIM2, TIM_SR_CC1IF))					// did we overflow?
	{
		overflows++;
		timer_clear_flag(TIM2, TIM_SR_CC1IF);
	}
}

