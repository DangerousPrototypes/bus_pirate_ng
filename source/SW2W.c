

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"	
#include "UI.h"
#include "SW2W.h"
#include "cdcacm.h"

static uint32_t returnval;


static uint32_t	clockfreq;
static uint8_t	mode;
static uint8_t	hiz;

enum
{
	SW2WIRE=2,
	SW3WIRE
};	

void SW2W_start(void)
{
	cdcprintf("SW2W start()");
}
void SW2W_startr(void)
{
	cdcprintf("SW2W startr()");
}
void SW2W_stop(void)
{
	cdcprintf("SW2W stop()");
}
void SW2W_stopr(void)
{
	cdcprintf("SW2W stopr()");
}
uint32_t SW2W_send(uint32_t d)
{
	cdcprintf("SW2W send(%08X)=%08X", d, returnval);

	returnval=d;

	return d;
}
uint32_t SW2W_read(void)
{
	cdcprintf("SW2W read()=%08X", returnval);
	return returnval;
}
void SW2W_clkh(void)
{
	cdcprintf("SW2W clkh()");
}
void SW2W_clkl(void)
{
	cdcprintf("SW2W clkl()");
}
void SW2W_dath(void)
{
	cdcprintf("SW2W dath()");
}
void SW2W_datl(void)
{
	cdcprintf("SW2W datl()");
}
uint32_t SW2W_dats(void)
{
	cdcprintf("SW2W dats()=%08X", returnval);
	return returnval;
}
void SW2W_clk(void)
{
	cdcprintf("SW2W clk()");
}
uint32_t SW2W_bitr(void)
{
	cdcprintf("SW2W bitr()=%08X", returnval);
	return returnval;
}
uint32_t SW2W_period(void)
{
	cdcprintf("SW2W period()=%08X", returnval);
	return returnval;
}
void SW2W_macro(uint32_t macro)
{
	cdcprintf("SW2W macro(%08X)", macro);
}
void SW2W_setup(void)
{
	cdcprintf("SW2W setup()");


	mode=2;		
	clockfreq=1000;
	hiz=0;
}
void SW2W_setup_exc(void)
{
	cdcprintf("SW2W setup_exc()");

	if(hiz==0)
	{
		gpio_set_mode(BPSW2WMOSIPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPSW2WMOSIPIN);
		gpio_set_mode(BPSW2WCLKPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPSW2WCLKPIN);
	}
	else
	{
		gpio_set_mode(BPSW2WMOSIPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BPSW2WMOSIPIN);
		gpio_set_mode(BPSW2WCLKPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BPSW2WCLKPIN);
	}

	// update modeConfig pins
	modeConfig.mosiport=BPSW2WMOSIPORT;
	modeConfig.clkport=BPSW2WCLKPORT;

}
void SW2W_cleanup(void)
{
	cdcprintf("SW2W cleanup()");

	// make all GPIO input
	gpio_set_mode(BPSW2WMOSIPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW2WMOSIPIN);
	gpio_set_mode(BPSW2WCLKPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW2WCLKPIN);

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
	cdcprintf("SW2W (mode speed hiz)=(%d, %d, %d)", mode, clockfreq, hiz);
}


