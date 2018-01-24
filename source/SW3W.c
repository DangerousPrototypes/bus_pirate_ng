

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
		gpio_clear(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);
	else
		gpio_set(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);

	modeConfig.wwr=0;
}

void SW3W_startr(void)
{
	cdcprintf("set CS=%d", !csmode);

	if(csmode)
		gpio_clear(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);
	else
		gpio_set(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);

	modeConfig.wwr=1;
}

void SW3W_stop(void)
{
	cdcprintf("set CS=%d", csmode);

	if(csmode)
		gpio_set(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);
	else
		gpio_clear(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);

	modeConfig.wwr=0;
}

void SW3W_stopr(void)
{
	cdcprintf("set CS=%d", csmode);

	if(csmode)
		gpio_set(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);
	else
		gpio_clear(BP_SW3W_CS_PORT, BP_SW3W_CS_PIN);

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
		gpio_set(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);
	else
		gpio_clear(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);

	// let it settle?

	for(i=0; i<modeConfig.numbits; i++)
	{
		if(cpha)							// CPHA=1 change CLK before MOSI
		{
			gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set

			if(d&mask)						// write MSB first (UI.c takes care of endianess)
				gpio_set(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);
			else
				gpio_clear(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);

			delayus(period/2);					// wait half period

			gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set

			returnval<<=1;
			mask>>=1;

			if(gpio_get(BP_SW3W_MISO_PORT, BP_SW3W_MISO_PIN))	// directly read the MISO
			returnval|=0x00000001;

			delayus(period/2);					// wait half period
		}
		else
		{
			if(d&mask)						// write MSB first (UI.c takes care of endianess)
				gpio_set(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);
			else
				gpio_clear(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);

			delayus(period/2);					// wait half period

			gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set

			returnval<<=1;
			mask>>=1;

			if(gpio_get(BP_SW3W_MISO_PORT, BP_SW3W_MISO_PIN))	// directly read the MISO
			returnval|=0x00000001;

			delayus(period/2);					// wait half period

			gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set
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

	gpio_set(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);
}

void SW3W_clkl(void)
{
	cdcprintf("set CLK=0");

	gpio_clear(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);
}

void SW3W_dath(void)
{
	cdcprintf("set MOSI=1");

	gpio_set(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);
}

void SW3W_datl(void)
{
	cdcprintf("set MOSI=0");

	gpio_clear(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);
}

uint32_t SW3W_dats(void) 
{
	uint32_t returnval;

	returnval=(gpio_get(BP_SW3W_MISO_PORT, BP_SW3W_MISO_PIN)?1:0);

	cdcprintf("MISO=%d", returnval);

	return returnval;
}

void SW3W_clk(void)
{
	cdcprintf("set CLK=%d", cpol);

	if(cpol)
		gpio_clear(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);
	else
		gpio_set(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);

	delayus(period/2);

	cdcprintf("\r\nset CLK=%d", !cpol);

	if(cpol)
		gpio_set(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);
	else
		gpio_clear(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);

	delayus(period/2);
}


// assumes CLK is in the right state!
uint32_t SW3W_bitr(void)
{
	uint32_t returnval;

	returnval=0;

	if(cpha)							// CPHA=1 change CLK before MOSI
	{
		gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set

		gpio_set(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);

		delayus(period/2);					// wait half period

		gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set

		if(gpio_get(BP_SW3W_MISO_PORT, BP_SW3W_MISO_PIN))	// directly read the MISO
			returnval=1;

		delayus(period/2);					// wait half period
	}
	else
	{
		gpio_set(BP_SW3W_MOSI_PORT, BP_SW3W_MOSI_PIN);

		delayus(period/2);					// wait half period

		gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set


		if(gpio_get(BP_SW3W_MISO_PORT, BP_SW3W_MISO_PIN))	// directly read the MISO
		returnval=1;

		delayus(period/2);					// wait half period

		gpio_toggle(BP_SW3W_CLK_PORT, BP_SW3W_CLK_PIN);		// toggle is ok as the right polarity is set
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
		cpha=(askint(SW3WCPHAMENU, 1, 2, 2)-1);
		cpol=(askint(SW3WCPOLMENU, 1, 2, 2)-1);
		csmode=(askint(SW3WCSMENU, 1, 2, 2)-1);
		opendrain=(askint(SW3WODMENU, 1, 2, 1)-1);
	}
}

void SW3W_setup_exc(void)
{
	if(opendrain)
	{
		gpio_set_mode(BP_SW3W_MOSI_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SW3W_MOSI_PIN);
		gpio_set_mode(BP_SW3W_CS_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SW3W_CS_PIN);
		gpio_set_mode(BP_SW3W_CLK_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SW3W_CLK_PIN);
		gpio_set_mode(BP_SW3W_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW3W_MISO_PIN);
	}
	else
	{
		gpio_set_mode(BP_SW3W_MOSI_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SW3W_MOSI_PIN);
		gpio_set_mode(BP_SW3W_CS_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SW3W_CS_PIN);
		gpio_set_mode(BP_SW3W_CLK_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SW3W_CLK_PIN);
		gpio_set_mode(BP_SW3W_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW3W_MISO_PIN);
	}

	// update modeConfig pins
	modeConfig.misoport=BP_SW3W_MISO_PORT;
	modeConfig.mosiport=BP_SW3W_MOSI_PORT;
	modeConfig.csport=BP_SW3W_CS_PORT;
	modeConfig.clkport=BP_SW3W_CLK_PORT;
	modeConfig.misopin=BP_SW3W_MISO_PIN;
	modeConfig.mosipin=BP_SW3W_MOSI_PIN;
	modeConfig.cspin=BP_SW3W_CS_PIN;
	modeConfig.clkpin=BP_SW3W_CLK_PIN;
}
void SW3W_cleanup(void)
{
	// make all GPIO input
	gpio_set_mode(BP_SW3W_MOSI_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW3W_MOSI_PIN);
	gpio_set_mode(BP_SW3W_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW3W_MISO_PIN);
	gpio_set_mode(BP_SW3W_CLK_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW3W_CLK_PIN);
	gpio_set_mode(BP_SW3W_CS_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SW3W_CS_PIN);

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

void SW3W_help(void)
{
	cdcprintf("Peer to peer 3 or 4 wire full duplex protocol. Very\r\n");
	cdcprintf("high clockrates upto 20MHz are possible.\r\n");
	cdcprintf("\r\n");
	cdcprintf("More info: https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus\r\n");
	cdcprintf("\r\n");


	cdcprintf("BPCMD\t {,] |                 DATA (1..32bit)               | },]\r\n");
	cdcprintf("CMD\tSTART| D7  | D6  | D5  | D4  | D3  | D2  | D1  | D0  | STOP\r\n");

	if(cpha)
	{	
		cdcprintf("MISO\t-----|{###}|{###}|{###}|{###}|{###}|{###}|{###}|{###}|------\r\n");
		cdcprintf("MOSI\t-----|{###}|{###}|{###}|{###}|{###}|{###}|{###}|{###}|------\r\n");
	}
	else
	{
		cdcprintf("MISO\t---{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}--|------\r\n");
		cdcprintf("MOSI\t---{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}--|------\r\n");
	}

	if(cpol)
		cdcprintf("CLK     \"\"\"\"\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"\"\"\"\"\r\n");
	else
		cdcprintf("CLK\t_____|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|______\r\n");

	if(csmode)
		cdcprintf("CS\t\"\"___|_____|_____|_____|_____|_____|_____|_____|_____|___\"\"\"\r\n");
	else
		cdcprintf("CS\t__\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"___\r\n");

	cdcprintf("\r\nCurrent mode is CPHA=%d and CPOL=%d\r\n",cpha, cpol);
	cdcprintf("\r\n");
	cdcprintf("Connection:\r\n");
	cdcprintf("\tMOSI \t------------------ MOSI\r\n");
	cdcprintf("\tMISO \t------------------ MISO\r\n");
	cdcprintf("{BP}\tCLK\t------------------ CLK\t{DUT}\r\n");
	cdcprintf("\tCS\t------------------ CS\r\n");
	cdcprintf("\tGND\t------------------ GND\r\n");
}


