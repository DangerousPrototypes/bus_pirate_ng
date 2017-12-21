

#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include "UI.h"
#include "AUXpin.h"


void initAUX(void)
{
	gpio_set_mode(BPAUXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPAUXPIN);
}

void setAUX(uint8_t state)
{
	gpio_set_mode(BPAUXPORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPAUXPIN);
	
	if(state)
		gpio_set(BPAUXPORT, BPAUXPIN);
	else
		gpio_clear(BPAUXPORT, BPAUXPIN);

}

uint8_t getAUX(void)
{
	gpio_set_mode(BPAUXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPAUXPIN);

	return (gpio_get(BPAUXPORT, BPAUXPIN)?1:0);
} 


void setPWM(uint32_t period, uint32_t oc)
{
	if(period!=0)
	{
		rcc_periph_clock_enable(RCC_TIM1);				// enable clock
		gpio_set_mode(BPPWMPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL , BPPWMPIN);	//output pushpull
		timer_reset(BPPWMTIMER);					// reset peripheral
		timer_set_mode(BPPWMTIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);	//?
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



