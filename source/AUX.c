

#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/gpio.h>
#include "AUX.h"


void initAUX(void)
{
	gpio_set_mode(AUXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, AUXPIN);
}

void setAUX(uint8_t state)
{
	gpio_set_mode(AUXPORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, AUXPIN);
	
	if(state)
		gpio_set(AUXPORT, AUXPIN);
	else
		gpio_clear(AUXPORT, AUXPIN);

}

uint8_t getAUX(void)
{
	gpio_set_mode(AUXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, AUXPIN);

	return (gpio_get(AUXPORT, AUXPIN)?1:0);
} 

