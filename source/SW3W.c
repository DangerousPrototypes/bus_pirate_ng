

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"	
#include "UI.h"
#include "SW3W.h"
#include "cdcacm.h"

static uint32_t	period;
static uint8_t	csmode;
static uint8_t	opendrain;
static uint8_t	cpha, cpol;



void SW3W_start(void)
{
	cdcprintf("set CS=%d", !csmode);

	if(csmode)
		gpio_clear(BPSW3WCSPORT, BPSW3WCSPIN);
	else
		gpio_set(BPSW3WCSPORT, BPSW3WCSPIN);

	modeConfig.wwr=0;
}

void SW3W_startr(void)
{
	cdcprintf("set CS=%d", !csmode);

	if(csmode)
		gpio_clear(BPSW3WCSPORT, BPSW3WCSPIN);
	else
		gpio_set(BPSW3WCSPORT, BPSW3WCSPIN);

	modeConfig.wwr=1;
}

void SW3W_stop(void)
{
	cdcprintf("set CS=%d", csmode);

	if(csmode)
		gpio_set(BPSW3WCSPORT, BPSW3WCSPIN);
	else
		gpio_clear(BPSW3WCSPORT, BPSW3WCSPIN);

	modeConfig.wwr=0;
}

void SW3W_stopr(void)
{
	cdcprintf("set CS=%d", csmode);

	if(csmode)
		gpio_set(BPSW3WCSPORT, BPSW3WCSPIN);
	else
		gpio_clear(BPSW3WCSPORT, BPSW3WCSPIN);

	modeConfig.wwr=0;
}

uint32_t SW3W_send(uint32_t d)
{
	int i;

	uint32_t mask, returnval;

	mask=0x80000000>>(32-modeConfig.numbits);
	returnval=0;

	// set clock to right idle level
	if(cpol)
		gpio_set(BPSW3WCLKPORT, BPSW3WCLKPIN);
	else
		gpio_clear(BPSW3WCLKPORT, BPSW3WCLKPIN);

	// let it settle?

	for(i=0; i<modeConfig.numbits; i++)
	{
		if(cpha)							// CPHA=1 change CLK before MOSI
		{
			gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set

			if(d&mask)						// write MSB first (UI.c takes care of endianess)
				gpio_set(BPSW3WMOSIPORT, BPSW3WMOSIPIN);
			else
				gpio_clear(BPSW3WMOSIPORT, BPSW3WMOSIPIN);

			delayus(period/2);					// wait half period

			gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set

			returnval<<=1;
			mask>>=1;

			if(gpio_get(BPSW3WMISOPORT, BPSW3WMISOPIN))		// directly read the MISO
			returnval|=0x00000001;

			delayus(period/2);					// wait half period
		}
		else
		{
			if(d&mask)						// write MSB first (UI.c takes care of endianess)
				gpio_set(BPSW3WMOSIPORT, BPSW3WMOSIPIN);
			else
				gpio_clear(BPSW3WMOSIPORT, BPSW3WMOSIPIN);

			delayus(period/2);					// wait half period

			gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set

			returnval<<=1;
			mask>>=1;

			if(gpio_get(BPSW3WMISOPORT, BPSW3WMISOPIN))		// directly read the MISO
			returnval|=0x00000001;

			delayus(period/2);					// wait half period

			gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set
		}
	}

	return returnval;
}

uint32_t SW3W_read(void)
{
	uint32_t returnval;

	returnval=SW3W_send(0xFFFFFFFF);

	return returnval;
}

void SW3W_clkh(void)
{
	cdcprintf("set CLK=1");

	gpio_set(BPSW3WCLKPORT, BPSW3WCLKPIN);
}

void SW3W_clkl(void)
{
	cdcprintf("set CLK=0");

	gpio_clear(BPSW3WCLKPORT, BPSW3WCLKPIN);
}

void SW3W_dath(void)
{
	cdcprintf("set MOSI=1");

	gpio_set(BPSW3WMOSIPORT, BPSW3WMOSIPIN);
}

void SW3W_datl(void)
{
	cdcprintf("set MOSI=0");

	gpio_clear(BPSW3WMOSIPORT, BPSW3WMOSIPIN);
}

uint32_t SW3W_dats(void) 
{
	uint32_t returnval;

	returnval=(gpio_get(BPSW3WMISOPORT, BPSW3WMISOPIN)?1:0);

	cdcprintf("MISO=%d", returnval);

	return returnval;
}

void SW3W_clk(void)
{
	cdcprintf("set CLK=%d", cpol);

	if(cpol)
		gpio_clear(BPSW3WCLKPORT, BPSW3WCLKPIN);
	else
		gpio_set(BPSW3WCLKPORT, BPSW3WCLKPIN);

	delayus(period/2);

	cdcprintf("\r\nset CLK=%d", !cpol);

	if(cpol)
		gpio_set(BPSW3WCLKPORT, BPSW3WCLKPIN);
	else
		gpio_clear(BPSW3WCLKPORT, BPSW3WCLKPIN);

	delayus(period/2);
}


// assumes CLK is in the right state!
uint32_t SW3W_bitr(void)
{
	uint32_t returnval;

	returnval=0;

	if(cpha)							// CPHA=1 change CLK before MOSI
	{
		gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set

		gpio_set(BPSW3WMOSIPORT, BPSW3WMOSIPIN);

		delayus(period/2);					// wait half period

		gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set

		if(gpio_get(BPSW3WMISOPORT, BPSW3WMISOPIN))		// directly read the MISO
			returnval=1;

		delayus(period/2);					// wait half period
	}
	else
	{
		gpio_set(BPSW3WMOSIPORT, BPSW3WMOSIPIN);

		delayus(period/2);					// wait half period

		gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set


		if(gpio_get(BPSW3WMISOPORT, BPSW3WMISOPIN))		// directly read the MISO
		returnval=1;

		delayus(period/2);					// wait half period

		gpio_toggle(BPSW3WCLKPORT, BPSW3WCLKPIN);		// toggle is ok as the right polarity is set
	}

	cdcprintf("RX: %d.1", returnval);

	return returnval;
}

uint32_t SW3W_period(void)
{
	uint32_t returnval;

	returnval=0;

	return returnval;
}

void SW3W_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf("No macros available");
				break;
		default:	cdcprintf("Macro not defined");
				modeConfig.error=1;
	}
}

void SW3W_setup(void)
{
	cdcprintf("SW3W setup()");

	// did the user leave us arguments?
	// period
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	period=getint();
	if(period<20) modeConfig.error=1;

	// cpol
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	cpol=getint();
	if((cpol>0)&&(cpol<=2)) cpol-=1;
	else modeConfig.error=1;

	// cpha
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	cpha=getint();
	if((cpha>0)&&(cpha<=2)) cpha-=1;
	else modeConfig.error=1;

	// csmode
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	csmode=getint();
	if((csmode>0)&&(csmode<=2)) csmode-=1;
	else modeConfig.error=1;

	// opendrain?
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	opendrain=getint();
	if((opendrain>0)&&(opendrain<=2)) opendrain-=1;
	else modeConfig.error=1;


	// did the user did it right?
	if(modeConfig.error)			// go interactive 
	{
		period=(askint(SW3WPERIODMENU, 10, 1000, 1000));
		cpha=(askint(SW3WCPHAMENU, 1, 2, 1)-1);
		cpol=(askint(SW3WCPOLMENU, 1, 2, 1)-1);
		csmode=(askint(SW3WCSMENU, 1, 2, 2)-1);
		opendrain=(askint(SW3WODMENU, 1, 2, 1)-1);
	}
}

void SW3W_setup_exc(void)
{
	if(opendrain)
	{
		gpio_set_mode(BPSW3WMOSIPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BPSW3WMOSIPIN);
		gpio_set_mode(BPSW3WCSPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BPSW3WCSPIN);
		gpio_set_mode(BPSW3WCLKPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BPSW3WCLKPIN);
		gpio_set_mode(BPSW3WMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW3WMISOPIN);
	}
	else
	{
		gpio_set_mode(BPSW3WMOSIPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPSW3WMOSIPIN);
		gpio_set_mode(BPSW3WCSPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPSW3WCSPIN);
		gpio_set_mode(BPSW3WCLKPORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BPSW3WCLKPIN);
		gpio_set_mode(BPSW3WMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW3WMISOPIN);
	}

	// update modeConfig pins
	modeConfig.misoport=BPSW3WMISOPORT;
	modeConfig.mosiport=BPSW3WMOSIPORT;
	modeConfig.csport=BPSW3WCSPORT;
	modeConfig.clkport=BPSW3WCLKPORT;
	modeConfig.misopin=BPSW3WMISOPIN;
	modeConfig.mosipin=BPSW3WMOSIPIN;
	modeConfig.cspin=BPSW3WCSPIN;
	modeConfig.clkpin=BPSW3WCLKPIN;
}
void SW3W_cleanup(void)
{
	// make all GPIO input
	gpio_set_mode(BPSW3WMOSIPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW3WMOSIPIN);
	gpio_set_mode(BPSW3WMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW3WMISOPIN);
	gpio_set_mode(BPSW3WCLKPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW3WCLKPIN);
	gpio_set_mode(BPSW3WCSPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSW3WCSPIN);

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

void SW3W_pins(void)
{
	cdcprintf("CS\tMISO\tCLK\tMOSI");
}

void SW3W_settings(void)
{
	cdcprintf("SW3W (holdtime cpol cpha csmode od)=(%d, %d, %d, %d, %d)", period, cpol+1, cpha+1, csmode+1, opendrain+1);
}




