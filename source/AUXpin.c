

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
	gpio_set_mode(BP_AUX_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_AUX_PIN);
}

// set or clear AUX pin
// also sets pin direction!
void setAUX(uint8_t state)
{
	gpio_set_mode(BP_AUX_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_AUX_PIN);
	
	if(state)
		gpio_set(BP_AUX_PORT, BP_AUX_PIN);
	else
		gpio_clear(BP_AUX_PORT, BP_AUX_PIN);

}

// returns the level on AUX pin
uint8_t getAUX(void)
{
	gpio_set_mode(BP_AUX_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_AUX_PIN);

	return (gpio_get(BP_AUX_PORT, BP_AUX_PIN)?1:0);
} 


// output a PWM on the AUX pin (1 count=1/36000000s)
// if period=0 PWM is shutdown
void setPWM(uint32_t period, uint32_t oc)
{
	if(period!=0)
	{
		rcc_periph_clock_enable(BP_PWM_CLOCK);									// enable clock
		gpio_set_mode(BP_PWM_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL , BP_PWM_PIN);	//output pushpull
		timer_reset(BP_PWM_TIMER);										// reset peripheral
		timer_set_mode(BP_PWM_TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);			// count up
		timer_set_oc_mode(BP_PWM_TIMER, BP_PWM_CHAN, TIM_OCM_PWM1);						// PWM1 == high/low; PWM2= low/high
		timer_enable_oc_output(BP_PWM_TIMER, BP_PWM_CHAN);							// output channel
		timer_enable_break_main_output(BP_PWM_TIMER);								// need to set break
		timer_set_oc_value(BP_PWM_TIMER, BP_PWM_CHAN, oc);							// set match value
		timer_set_period(BP_PWM_TIMER, period);									// set period 
		timer_enable_counter(BP_PWM_TIMER);									// enable the timer
		modeConfig.pwm=1;

	}
	else
	{
		rcc_periph_clock_disable(RCC_TIM1);
		gpio_set_mode(BP_PWM_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_PWM_PIN);	
		timer_disable_counter(BP_PWM_TIMER);
		modeConfig.pwm=0;
	}


}

// frequency counter
uint32_t getfreq(void)
{
	uint32_t counts;

	rcc_periph_clock_enable(BP_FREQ_CLK);
	gpio_set_mode(BP_FREQ_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_FREQ_PIN);	// This is probably already input
	timer_reset(BP_FREQ_TIMER);
	timer_disable_preload(BP_FREQ_TIMER);
	timer_continuous_mode(BP_FREQ_TIMER);					// keeps running
	timer_set_period(BP_FREQ_TIMER, 65535);
	timer_slave_set_mode(BP_FREQ_TIMER, TIM_SMCR_SMS_ECM1);			// count external ticks
	timer_slave_set_filter(BP_FREQ_TIMER, TIM_IC_OFF);			// no filter
	timer_slave_set_polarity(BP_FREQ_TIMER, TIM_ET_RISING);			// rising edge 
	timer_slave_set_prescaler(BP_FREQ_TIMER, TIM_IC_PSC_OFF);		// dont prescale
	timer_slave_set_trigger(BP_FREQ_TIMER, TIM_SMCR_TS_ETRF);		// use etr input

	// timer is 16 bit so we need to capture overflows
	overflows=0;
	timer_update_on_overflow(BP_FREQ_TIMER);				// we want to see overflows
	nvic_enable_irq(BP_FREQ_NVIC);						// enable timer irq
	timer_enable_irq(BP_FREQ_TIMER, TIM_DIER_CC1IE);
	timer_enable_counter(BP_FREQ_TIMER);					// start counting

	delayms(1000);								// we want to count just 1s

	counts = timer_get_counter(BP_FREQ_TIMER) + overflows*65536;		// counts is the frequency 

	rcc_periph_clock_disable(BP_FREQ_CLK);					// turn peripheral off
	timer_disable_counter(BP_FREQ_TIMER);
	nvic_disable_irq(BP_FREQ_NVIC);

	return counts;
}

// timer isr count the number of overflows
void tim3_isr(void)
{
	if (timer_get_flag(BP_FREQ_TIMER, TIM_SR_CC1IF))					// did we overflow?
	{
		overflows++;
		timer_clear_flag(BP_FREQ_TIMER, TIM_SR_CC1IF);
	}
}

