

#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/gpio.h>
#include "AUX.h"


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

