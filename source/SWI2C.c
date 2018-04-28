#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"
#include "UI.h"
#include "SWI2C.h"
#include "SW2W.h"
#include "cdcacm.h"

static uint32_t	period;
static uint8_t	speed;

void SWI2C_start(void)
{
    /*cdcprintf("I2C START");

    SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output

    SW2W_DATA_HIGH();
    SW2W_CLOCK_HIGH();

    delayus(period/2);

    SW2W_DATA_LOW();
    SW2W_CLOCK_HIGH();

    delayus(period/2);

    SW2W_DATA_LOW();
    SW2W_CLOCK_LOW();*/

    SW2W_start();

    //reset read/write mode
}

void SWI2C_stop(void)
{
    /*cdcprintf("I2C STOP");

    SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output

    SW2W_DATA_LOW();
    SW2W_CLOCK_HIGH();

    delayus(period/2);

    SW2W_DATA_HIGH();
    SW2W_DATA_HIGH();

    delayus(period/2);*/

    SW2W_stop();
}

uint32_t SWI2C_write(uint32_t d)
{
    int i;
    uint32_t mask;

    //if read/write mode tracker reset, use last bit to set read/write mode
    //if read mode is set warn write mode may be invalid

    SW2W_setDATAmode(SW2W_OUTPUT);					// SDA output
    SW2W_CLOCK_LOW();

    mask=0x80000000>>(32-modeConfig.numbits);

    for(i=0; i<modeConfig.numbits; i++)
    {
        if(d&mask) SW2W_DATA_HIGH();
        else SW2W_DATA_LOW(); //setup the data to write


        delayus(period/2); //delay low

        SW2W_CLOCK_HIGH(); //clock high
        delayus(period/2); //delay high

        SW2W_CLOCK_LOW(); //low again, will delay at begin of next bit or byte

        //TODO: ack/nack management

    }

    return 0;
}

uint32_t SWI2C_read(void)
{
    int i;
    uint32_t returnval;

    SW2W_setDATAmode(SW2W_INPUT);						// SDA input
    SW2W_CLOCK_LOW();

    returnval=0;

    //check read/write mode, warn if in write mode...

    for(i=0; i<modeConfig.numbits; i++)
    {

        delayus(period/2); //delay low

        SW2W_CLOCK_HIGH(); //high
        delayus(period/2); //delay high
        //read data
        if(SW2W_DATA_READ()) returnval|=1;
        returnval<<=1;

        SW2W_CLOCK_LOW();//low again, will delay at begin of next bit or byte...

        //TODO: ack/nack management

    }

    return returnval;
}

void SWI2C_macro(uint32_t macro)
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

void SWI2C_setup(void)
{
    // speed
    if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
    consumewhitechars();
    speed=getint();
    if((speed>0)&&(speed<=2)) speed-=1;
    else modeConfig.error=1;

    // did the user did it right?
    if(modeConfig.error)			// go interactive
    {
        speed=(askint(SWI2CSPEEDMENU, 1, 2, 1));
    }

}

void SWI2C_setup_exc(void)
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
    modeConfig.hiz=1;

    //a guess... 72 period in the PWM is .99999uS. Multiply the period in uS * 72, divide by 4 four 4* over sample
    //TODO: used fixed calculated period for I2C
    modeConfig.logicanalyzerperiod=((period*72)/4);

}

void SWI2C_cleanup(void)
{
    /*cdcprintf("SWI2C cleanup()");

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
     */
    SW2W_cleanup();
}
void SWI2C_pins(void)
{
    cdcprintf("-\t-\tCLK\tDAT");
}
void SWI2C_settings(void)
{
    cdcprintf("SWI2C (period hiz)=(%d %d)", period, hiz);
}
void SWI2C_help(void)
{
    cdcprintf("Muli-Master-multi-slave 2 wire protocol using a CLOCK and an bidirectional DATA\r\n");
    cdcprintf("line in opendrain configuration. Standard clock frequencies are 100KHz, 400KHz\r\n");
    cdcprintf("and 1MHz.\r\n");
    cdcprintf("\r\n");
    cdcprintf("More info: https://en.wikipedia.org/wiki/I2C\r\n");
    cdcprintf("\r\n");
    cdcprintf("Electrical:\r\n");
    cdcprintf("\r\n");
    cdcprintf("BPCMD\t   { |            ADDRES(7bits+R/!W bit)             |\r\n");
    cdcprintf("CMD\tSTART| A6  | A5  | A4  | A3  | A2  | A1  | A0  | R/!W| ACK* \r\n");
    cdcprintf("\t-----|-----|-----|-----|-----|-----|-----|-----|-----|-----\r\n");
    cdcprintf("SDA\t\"\"___|_###_|_###_|_###_|_###_|_###_|_###_|_###_|_###_|_###_ ..\r\n");
    cdcprintf("SCL\t\"\"\"\"\"|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__ ..\r\n");
    cdcprintf("\r\n");
    cdcprintf("BPCMD\t   |                      DATA (8bit)              |     |  ]  |\r\n");
    cdcprintf("CMD\t.. | D7  | D6  | D5  | D4  | D3  | D2  | D1  | D0  | ACK*| STOP|  \r\n");
    cdcprintf("\t  -|-----|-----|-----|-----|-----|-----|-----|-----|-----|-----|\r\n");
    cdcprintf("SDA\t.. |_###_|_###_|_###_|_###_|_###_|_###_|_###_|_###_|_###_|___\"\"|\r\n");
    cdcprintf("SCL\t.. |__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|__\"__|\"\"\"\"\"|\r\n");
    cdcprintf("\r\n");
    cdcprintf("* Receiver needs to pull SDA down when address/byte is received correctly\r\n");
    cdcprintf("\r\n");
    cdcprintf("Connection:\r\n");
    cdcprintf("\t\t  +--[4k7]---+--- +3V3 or +5V0\r\n");
    cdcprintf("\t\t  | +-[4k7]--|\r\n");
    cdcprintf("\t\t  | |\r\n");
    cdcprintf("\tSDA \t--+-|------------- SDA\r\n");
    cdcprintf("{BP}\tSCL\t----+------------- SCL  {DUT}\r\n");
    cdcprintf("\tGND\t------------------ GND\r\n");
}
