

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"	
#include "UI.h"
#include "SW2W.h"
#include "cdcacm.h"

static uint32_t	period;

void SW2W_start(void)
{
	cdcprintf("I2C START");

	SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output

	SW2W_DATA_HIGH();
	SW2W_CLOCK_HIGH();

	delayus(period/2);

	SW2W_DATA_LOW();
	SW2W_CLOCK_HIGH();

	delayus(period/2);

	SW2W_DATA_LOW();
	SW2W_CLOCK_LOW();

	//reset read/write mode
}

void SW2W_stop(void)
{
	cdcprintf("I2C STOP");

	SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output

	SW2W_DATA_LOW();
	SW2W_CLOCK_HIGH();

	delayus(period/2);

	SW2W_DATA_HIGH();
	SW2W_DATA_HIGH();

	delayus(period/2);
}


uint32_t SW2W_send(uint32_t d)
{
	int i;
	uint32_t mask;

	SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output
	SW2W_CLOCK_LOW();

	mask=0x80000000>>(32-modeConfig.numbits);

	for(i=0; i<modeConfig.numbits; i++)
	{
		if(d&mask) SW2W_DATA_HIGH();
		else SW2W_DATA_LOW(); //setup the data to write
		mask>>1;

		delayus(period/2); //delay low

		SW2W_CLOCK_HIGH(); //clock high
		delayus(period/2); //delay high

		SW2W_CLOCK_LOW(); //low again, will delay at begin of next bit or byte

	}
	
	return 0;
}

uint32_t SW2W_read(void)
{
	int i;
	uint32_t returnval;

	SW2W_setDATAmode(SW2W_INPUT);						// SDA input

	returnval=0;

	for(i=0; i<modeConfig.numbits; i++)
	{
		delayus(period/2); //delay low

		SW2W_CLOCK_HIGH(); //high
		delayus(period/2); //delay high
		//read data
		if(SW2W_DATA_READ()) returnval|=1;
		returnval<<=1;

		SW2W_CLOCK_LOW();//low again, will delay at begin of next bit or byte...
	}

	return returnval;
}
void SW2W_clkh(void)
{
	cdcprintf("set CLK=1");

	SW2W_CLOCK_HIGH(); //high
}
void SW2W_clkl(void)
{
	cdcprintf("set CLK=0");

	SW2W_CLOCK_LOW();//low
}
void SW2W_dath(void)
{
	cdcprintf("set SDA=1");

	SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output
	SW2W_DATA_HIGH();
}
void SW2W_datl(void)
{
	cdcprintf("set SDA=0");

	SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output
	SW2W_DATA_LOW();
}
uint32_t SW2W_dats(void)
{
	uint32_t dat;

	SW2W_setDATAmode(SW2W_INPUT);						// SDA input
	dat=(SW2W_DATA_READ()?1:0);

	cdcprintf("SDA=%d", dat);

	return dat;
}
void SW2W_clk(void)
{
	delayus(period/1); //delay low

	SW2W_CLOCK_HIGH(); //clock high
	delayus(period/2); //delay high

	SW2W_CLOCK_LOW(); //low again
	delayus(period/1); //delay low

	cdcprintf("set CLK=0");
	cdcprintf("set CLK=1");
	cdcprintf("set CLK=0");
}
uint32_t SW2W_bitr(void)
{
	uint32_t dat;

	SW2W_setDATAmode(SW2W_INPUT);						// SDA input

	delayus(period/1); //delay low

	SW2W_CLOCK_HIGH(); //clock high
	delayus(period/2); //delay high

	dat=(SW2W_DATA_READ()?1:0);

	SW2W_CLOCK_LOW(); //low again
	delayus(period/1); //delay low

	cdcprintf("SW2W bitr()=%08X", dat);
	return dat;
}
uint32_t SW2W_period(void)
{
	uint32_t returnval;
	returnval=0;
	cdcprintf("SW2W period()=%08X", returnval);
	return returnval;
}
void SW2W_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf(" 1. I2C Address search\r\n");
//				cdcprintf(" 2. I2C sniffer\r\n";
			break;
		case 1:		SWI2C_search();
			break;
		default:	cdcprintf("Macro not defined");
			modeConfig.error=1;
	}
}
void SW2W_setup(void)
{

	//todo: offer speeds, get HiZ

	period=1000;
	hiz=0;
}
void SW2W_setup_exc(void)
{

	if(modeConfig.hiz)
	{
		SW2W_SETUP_OPENDRAIN();
	}
	else
	{
		SW2W_SETUP_PUSHPULL();
	}

	// update modeConfig pins
	modeConfig.mosiport=BP_SW2W_SDA_PORT;
	modeConfig.clkport=BP_SW2W_CLK_PORT;

	//a guess... 72 period in the PWM is .99999uS. Multiply the period in uS * 72, divide by 4 four 4* over sample
	modeConfig.logicanalyzerperiod=((period*72)/4);

}

void SW2W_cleanup(void)
{
	cdcprintf("SW2W cleanup()");

	// make all GPIO input
	SW2W_SETUP_HIZ();

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

void SW2W_setDATAmode(uint8_t input)
{

	if(input)
	{
		SW2W_DATA_INPUT();
	}
	else
	{
		// set SDA as output
		if(modeConfig.hiz)
		{
			SW2W_DATA_OPENDRAIN();
		}
		else
		{
			SW2W_DATA_PUSHPULL();
		}
	}

}

void I2C_search(void){

	/*bbH(MOSI + CLK, 0); //clock and data high

	BPMSG1070; //search

	if (BP_CLK == 0 || BP_MOSI == 0) {
		BPMSG1019; //warning
		BPMSG1020; //short or no pullups
		bpBR;
		return;
	}
    for (i = 0; i < 0x100; i++) {
        bbI2Cstart(); //send start
        bbWriteByte(i); //send address
        c = bbReadBit(); //look for ack

        if (c == 0) {//0 is ACK
            bpWbyte(i);
            bpWchar('('); //bpWstring("(");
            bpWbyte((i >> 1));
            if ((i & 0b1) == 0) {//if the first bit is set it's a read address, send a byte plus nack to clean up
                bpWstring(" W");
            } else {
                bbReadByte();
                bbI2Cnack(); //bbWriteBit(1);//high bit is NACK
                bpWstring(" R");
            }
            bpWstring(")");
            bpSP;
        }
        bbI2Cstop();
    }
    bpWBR;*/

}


