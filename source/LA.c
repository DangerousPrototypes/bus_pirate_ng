

#include <stdint.h>
#include "buspirateNG.h"
#include "LA.h"
#include "cdcacm.h"
#include "UI.h"

static uint32_t returnval;
static uint8_t lamode;

static uint8_t labuff[BPLABUFFSIZE];			// is this french?!


static void displaybuff(void);

void LA_start(void)
{
	cdcprintf("logic analyzer start()");
}
void LA_startr(void)
{
	cdcprintf("logic analyzer startr()");
}
void LA_stop(void)
{
	cdcprintf("logic analyzer stop()");
}
void LA_stopr(void)
{
	cdcprintf("logic analyzer stopr()");
}
uint32_t LA_send(uint32_t d)
{
	cdcprintf("-logic analyzer send(%08X)=%08X", d, returnval);

	returnval=d;

	return d;
}
uint32_t LA_read(void)
{
	cdcprintf("logic analyzer read()=%08X", returnval);
	return returnval;
}
void LA_clkh(void)
{
	cdcprintf("logic analyzer clkh()");
}
void LA_clkl(void)
{
	cdcprintf("logic analyzer clkl()");
}
void LA_dath(void)
{
	cdcprintf("logic analyzer dath()");
}
void LA_datl(void)
{
	cdcprintf("logic analyzer datl()");
}
uint32_t LA_dats(void)
{
	cdcprintf("logic analyzer dats()=%08X", returnval);
	return returnval;
}
void LA_clk(void)
{
	cdcprintf("logic analyzer clk()");
}
uint32_t LA_bitr(void)
{
	cdcprintf("logic analyzer bitr()=%08X", returnval);
	return returnval;
}
uint32_t LA_period(void)
{
	cdcprintf("logic analyzer period()=%08X", returnval);
	return returnval;
}
void LA_macro(uint32_t macro)
{
	int i;

	switch(macro)
	{
		case 0:		cdcprintf("1. fill buffer with random data\r\n2. display buffer in ASCII art");
				break;
		case 1:		for(i=0; i<BPLABUFFSIZE; i++)
				{
					labuff[i]=i;
				}
				break;
		case 2:		displaybuff();
				break;
		default:	modeConfig.error=1;
				cdcprintf("no such macro");
				break;
	}
}
void LA_setup(void)
{
	cdcprintf("logic analyzer setup()");
}
void LA_setup_exc(void)
{
	cdcprintf("Buffer size=%d", BPLABUFFSIZE);
}
void LA_cleanup(void)
{
	cdcprintf("logic analyzer cleanup()");
}
void LA_pins(void)
{
	cdcprintf("pin1\tpin2\tpin3\tpin4");
}
void LA_settings(void)
{
	cdcprintf("LA (mode)=(%d)", lamode);
}



void displaybuff(void)
{
	int i, j, stop;
	uint8_t mask;
	uint32_t offset;

	stop=0;
	offset=0;

	while(!stop)
	{
		cdcprintf("\x1B[2J\x1B[H");

		for(j=0; j<8; j++)
		{
			mask=0x01<<j;
			cdcprintf("\x1B[0;9%dmCH%d:\t", j, j);

			for(i=0; i<70; i++)
			{
				if(labuff[offset+i]&mask)
					cdcprintf("\x1B[0;10%dm \x1B[0;9%dm", j, j);
				else
					cdcprintf("_");
			}
			cdcprintf("\r\n");
		}
		cdcprintf("Offset=%d\r\n", offset);
		cdcprintf(" -: previous +: next x:quit\r\n");
		delayms(100);
		switch(cdcgetc())
		{
			case '+':	offset+=70;
					break;
			case '-':	offset-=70;
					break;
			case 'x':	stop=1;
					break;
		}
		
	}
}


