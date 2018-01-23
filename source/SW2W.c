

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"	
#include "UI.h"
#include "SW2W.h"
#include "cdcacm.h"

static uint32_t	period;
static uint8_t	hiz;

void SW2W_start(void)
{
	cdcprintf("I2C START");

	setSDAmode(SW2W_OUTPUT);					// SDA output

	gpio_set(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN); 
	gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN); 

	delayus(period/2);

	gpio_clear(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN); 
	gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN); 

	delayus(period/2);

	gpio_clear(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN); 
	gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN); 
}

void SW2W_startr(void)
{
	cdcprintf("SW2W startr()");
}

void SW2W_stop(void)
{
	cdcprintf("I2C STOP");

	setSDAmode(SW2W_OUTPUT);					// SDA output

	gpio_clear(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN); 
	gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN); 

	delayus(period/2);

	gpio_set(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN); 
	gpio_set(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN); 

	delayus(period/2);
}
void SW2W_stopr(void)
{
	cdcprintf("SW2W stopr()");
}
uint32_t SW2W_send(uint32_t d)
{
	int i;
	uint32_t mask;

	setSDAmode(SW2W_OUTPUT);					// SDA output

	mask=0x80000000>>(32-modeConfig.numbits);

	for(i=0; i<modeConfig.numbits; i++)
	{
		// level checking TODO: edge ?
		if(d&mask) gpio_set(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN);
			else gpio_clear(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN);
		gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);

		delayus(period/4);

		gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);

		delayus(period/2);

		gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);

		delayus(period/4);

	}
	
	return 0;
}

uint32_t SW2W_read(void)
{
	int i;
	uint32_t returnval;

	setSDAmode(SW2W_INPUT);						// SDA output

	returnval=0;

	for(i=0; i<modeConfig.numbits; i++)
	{
		gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
		delayus(period/4);
		gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
		delayus(period/4);

		if(gpio_get(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN)) returnval|=1;
		returnval<<=1;

		delayus(period/4);
		gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
		delayus(period/4);
	}

	return returnval;
}
void SW2W_clkh(void)
{
	cdcprintf("set CLK=1");

	gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
}
void SW2W_clkl(void)
{
	cdcprintf("set CLK=0");

	gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
}
void SW2W_dath(void)
{
	cdcprintf("set SDA=1");

	setSDAmode(SW2W_OUTPUT);					// SDA output
	gpio_set(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN);
}
void SW2W_datl(void)
{
	cdcprintf("set SDA=0");

	setSDAmode(SW2W_OUTPUT);					// SDA output
	gpio_clear(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN);
}
uint32_t SW2W_dats(void)
{
	uint32_t dat;

	setSDAmode(SW2W_INPUT);						// SDA input
	dat=(gpio_get(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN)?1:0);

	cdcprintf("SDA=%d", dat);

	return dat;
}
void SW2W_clk(void)
{
	gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
	delayus(period/4);
	gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
	delayus(period/2);
	gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN);
	delayus(period/4);

	cdcprintf("set CLK=0");
	cdcprintf("set CLK=1");
	cdcprintf("set CLK=0");
}
uint32_t SW2W_bitr(void)
{
	uint32_t returnval;
	cdcprintf("SW2W bitr()=%08X", returnval);
	return returnval;
}
uint32_t SW2W_period(void)
{
	uint32_t returnval;
	cdcprintf("SW2W period()=%08X", returnval);
	return returnval;
}
void SW2W_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf("No macros available");
				break;
		default:	cdcprintf("Macro not defined");
				modeConfig.error=1;
	}
}
void SW2W_setup(void)
{
	cdcprintf("SW2W setup()");


	period=1000;
	hiz=0;
}
void SW2W_setup_exc(void)
{
	cdcprintf("SW2W setup_exc()");

	if(hiz)
	{
		gpio_set_mode(BP_SW2W_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SW2W_SDA_PIN);
		gpio_set_mode(BP_SW2W_CLK_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SW2W_CLK_PIN);
	}
	else
	{
		gpio_set_mode(BP_SW2W_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SW2W_SDA_PIN);
		gpio_set_mode(BP_SW2W_CLK_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SW2W_CLK_PIN);
	}

	// update modeConfig pins
	modeConfig.mosiport=BP_SW2W_SDA_PORT;
	modeConfig.clkport=BP_SW2W_CLK_PORT;

}

void SW2W_cleanup(void)
{
	cdcprintf("SW2W cleanup()");

	// make all GPIO input
	gpio_set_mode(BP_SW2W_SDA_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW2W_SDA_PIN);
	gpio_set_mode(BP_SW2W_CLK_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW2W_CLK_PIN);

	// update modeConfig pins
	modeConfig.misoport=0;
	modeConfig.mosiport=0;
	modeConfig.csport=0;
	modeConfig.clkport=0;
	modeConfig.misopin=0;
	modeConfig.mosipin=0;
	modeConfig.cspin=0;
	modeConfig.clkpin=0;

}
void SW2W_pins(void)
{
	cdcprintf("-\t-\tCLK\tDAT");
}
void SW2W_settings(void)
{
	cdcprintf("SW2W (period hiz)=(%d %d)", period, hiz);
}

void setSDAmode(uint8_t input)
{

	if(input)
	{
		gpio_set_mode(BP_SW2W_SDA_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW2W_SDA_PIN);
	}
	else
	{
		// set SDA as output
		if(hiz)
		{
			gpio_set_mode(BP_SW2W_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SW2W_SDA_PIN);
		}
		else
		{
			gpio_set_mode(BP_SW2W_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SW2W_SDA_PIN);
		}
	}

}


