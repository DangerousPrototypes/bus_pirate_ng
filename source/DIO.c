

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"	
#include "UI.h"
#include "DIO.h"
#include "cdcacm.h"

static uint8_t opendrain, outputmask;

uint32_t DIO_send(uint32_t d)
{
	uint32_t returnval;

	if(modeConfig.numbits!=4)
	{
		cdcprintf("Only 4 bits is supported!!");
		modeConfig.error=1;
		return 0;
	}

	returnval=0;

	//DIO0
	if(outputmask&0x01)
	{
		if(d&0x01) gpio_set(BP_DIO_DIO0_PORT, BP_DIO_DIO0_PIN);
			else gpio_clear(BP_DIO_DIO0_PORT, BP_DIO_DIO0_PIN);
	}
	else
	{
		returnval|=(gpio_get(BP_DIO_DIO0_PORT, BP_DIO_DIO0_PIN)?0x01:0);
	}

	//DIO1
	if(outputmask&0x02)
	{
		if(d&0x02) gpio_set(BP_DIO_DIO1_PORT, BP_DIO_DIO1_PIN);
			else gpio_clear(BP_DIO_DIO1_PORT, BP_DIO_DIO1_PIN);
	}
	else
	{
		returnval|=(gpio_get(BP_DIO_DIO1_PORT, BP_DIO_DIO1_PIN)?0x02:0);
	}

	//DIO2
	if(outputmask&0x04)
	{
		if(d&0x04) gpio_set(BP_DIO_DIO2_PORT, BP_DIO_DIO2_PIN);
			else gpio_clear(BP_DIO_DIO2_PORT, BP_DIO_DIO2_PIN);
	}
	else
	{
		returnval|=(gpio_get(BP_DIO_DIO2_PORT, BP_DIO_DIO2_PIN)?0x04:0);
	}

	//DIO3
	if(outputmask&0x08)
	{
		if(d&0x08) gpio_set(BP_DIO_DIO3_PORT, BP_DIO_DIO3_PIN);
			else gpio_clear(BP_DIO_DIO3_PORT, BP_DIO_DIO3_PIN);
	}
	else
	{
		returnval|=(gpio_get(BP_DIO_DIO3_PORT, BP_DIO_DIO3_PIN)?0x08:0);
	}


	return returnval;
}

uint32_t DIO_read(void)
{
	uint32_t returnval;

	returnval=0;

	if(!(outputmask&0x01)) returnval|=(gpio_get(BP_DIO_DIO0_PORT, BP_DIO_DIO0_PIN)?0x01:0);
	if(!(outputmask&0x02)) returnval|=(gpio_get(BP_DIO_DIO1_PORT, BP_DIO_DIO1_PIN)?0x02:0);
	if(!(outputmask&0x04)) returnval|=(gpio_get(BP_DIO_DIO2_PORT, BP_DIO_DIO2_PIN)?0x04:0);
	if(!(outputmask&0x08)) returnval|=(gpio_get(BP_DIO_DIO3_PORT, BP_DIO_DIO3_PIN)?0x08:0);

	return returnval;
}

void DIO_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf("1. Set outputmask");
				break;
		case 1:		if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
				consumewhitechars();
				outputmask=getint();
				if(outputmask>16) modeConfig.error=1;

				if(modeConfig.error) outputmask=(askint(DIOOMMENU, 0, 16, 0));
				modeConfig.error=0;

				if(opendrain)
				{
					if(outputmask&0x01) gpio_set_mode(BP_DIO_DIO0_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_DIO_DIO0_PIN);	
					if(outputmask&0x02) gpio_set_mode(BP_DIO_DIO1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_DIO_DIO1_PIN);	
					if(outputmask&0x04) gpio_set_mode(BP_DIO_DIO2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_DIO_DIO2_PIN);	
					if(outputmask&0x08) gpio_set_mode(BP_DIO_DIO3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_DIO_DIO3_PIN);
				}
				else	
				{
					if(outputmask&0x01) gpio_set_mode(BP_DIO_DIO0_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_DIO_DIO0_PIN);	
					if(outputmask&0x02) gpio_set_mode(BP_DIO_DIO1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_DIO_DIO1_PIN);	
					if(outputmask&0x04) gpio_set_mode(BP_DIO_DIO2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_DIO_DIO2_PIN);	
					if(outputmask&0x08) gpio_set_mode(BP_DIO_DIO3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_DIO_DIO3_PIN);
				}
	
				break;
		default:	modeConfig.error=1;
				cdcprintf("no such macro");
				break;
	}

}

void DIO_setup(void)
{
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	opendrain=getint()-1;
	if(opendrain>1) modeConfig.error=1;

	if(modeConfig.error) opendrain=(askint(DIOODMENU, 1, 2, 1)-1);
	modeConfig.error=0;	
}

void DIO_setup_exc(void)
{
	// update modeConfig pins
	modeConfig.csport=BP_DIO_DIO0_PORT;
	modeConfig.misoport=BP_DIO_DIO1_PORT;
	modeConfig.clkport=BP_DIO_DIO2_PORT;
	modeConfig.mosiport=BP_DIO_DIO3_PORT;
	modeConfig.cspin=BP_DIO_DIO0_PIN;
	modeConfig.misopin=BP_DIO_DIO1_PIN;
	modeConfig.clkpin=BP_DIO_DIO2_PIN;
	modeConfig.mosipin=BP_DIO_DIO3_PIN;

	modeConfig.wwr=1;
	outputmask=0x00;				// all inputs
}

void DIO_cleanup(void)
{
	gpio_set_mode(BP_DIO_DIO0_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_DIO_DIO0_PIN);	
	gpio_set_mode(BP_DIO_DIO1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_DIO_DIO1_PIN);	
	gpio_set_mode(BP_DIO_DIO2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_DIO_DIO2_PIN);
	gpio_set_mode(BP_DIO_DIO3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_DIO_DIO3_PIN);

	// update modeConfig pins
	modeConfig.misoport=0;
	modeConfig.mosiport=0;
	modeConfig.csport=0;
	modeConfig.clkport=0;
	modeConfig.misopin=0;
	modeConfig.mosipin=0;
	modeConfig.cspin=0;
	modeConfig.clkpin=0;
	
	modeConfig.wwr=0;
}

void DIO_pins(void)
{
	cdcprintf("DIO1\tDIO2\tDIO3\tDIO4");
}

void DIO_settings(void)
{
	cdcprintf("DIO (od)=(%d)", opendrain+1);
}

void DIO_help(void)
{
	cdcprintf("General digital I/O mode\r\n");

}


