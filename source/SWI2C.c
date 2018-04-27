#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"
#include "UI.h"
#include "SWI2C.h"
#include "cdcacm.h"

void SWI2C_search(void);

static uint32_t	period;
static uint8_t	hiz;
static uint8_t	speed;

void SWI2C_start(void)
{
    cdcprintf("I2C START");

    SWI2C_setDATAmode(SWI2C_OUTPUT);					// SDA output

    SWI2C_DATA_HIGH();
    SWI2C_CLOCK_HIGH();

    delayus(period/2);

    SWI2C_DATA_LOW();
    SWI2C_CLOCK_HIGH();

    delayus(period/2);

    SWI2C_DATA_LOW();
    SWI2C_CLOCK_LOW();

    //reset read/write mode
}

void SWI2C_stop(void)
{
    cdcprintf("I2C STOP");

    SWI2C_setDATAmode(SWI2C_OUTPUT);					// SDA output

    SWI2C_DATA_LOW();
    SWI2C_CLOCK_HIGH();

    delayus(period/2);

    SWI2C_DATA_HIGH();
    SWI2C_DATA_HIGH();

    delayus(period/2);
}

uint32_t SWI2C_write(uint32_t d)
{
    int i;
    uint32_t mask;

    //if read/write mode tracker reset, use last bit to set read/write mode
    //if read mode is set warn write mode may be invalid

    SWI2C_setDATAmode(SWI2C_OUTPUT);					// SDA output
    SWI2C_CLOCK_LOW();

    mask=0x80000000>>(32-modeConfig.numbits);

    for(i=0; i<modeConfig.numbits; i++)
    {
        if(d&mask) SWI2C_DATA_HIGH();
        else SWI2C_DATA_LOW(); //setup the data to write


        delayus(period/2); //delay low

        SWI2C_CLOCK_HIGH(); //clock high
        delayus(period/2); //delay high

        SWI2C_CLOCK_LOW(); //low again, will delay at begin of next bit or byte

        //TODO: ack/nack management

    }



    return 0;
}

uint32_t SWI2C_read(void)
{
    int i;
    uint32_t returnval;

    SWI2C_setDATAmode(SWI2C_INPUT);						// SDA input
    SWI2C_CLOCK_LOW();

    returnval=0;

    //check read/write mode, warn if in write mode...

    for(i=0; i<modeConfig.numbits; i++)
    {

        delayus(period/2); //delay low

        SWI2C_CLOCK_HIGH(); //high
        delayus(period/2); //delay high
        //read data
        if(SWI2C_DATA_READ()) returnval|=1;
        returnval<<=1;

        SWI2C_CLOCK_LOW();//low again, will delay at begin of next bit or byte...

        //TODO: ack/nack management

    }

    return returnval;
}
/*
void SWI2C_validate_user_input(){
    //loop through user input, make sure has start and stop, preparse NACK for reads?????

}
 */

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
    cdcprintf("SWI2C setup_exc()");

    if(hiz)
    {
        SWI2C_SETUP_OPENDRAIN();
    }
    else
    {
        SWI2C_SETUP_PUSHPULL();
     }

    // update modeConfig pins
    modeConfig.mosiport=BP_SW2W_SDA_PORT;
    modeConfig.clkport=BP_SW2W_CLK_PORT;

    //a guess... 72 period in the PWM is .99999uS. Multiply the period in uS * 72, divide by 4 four 4* over sample
    modeConfig.logicanalyzerperiod=((period*72)/4);

}

void SWI2C_cleanup(void)
{
    cdcprintf("SWI2C cleanup()");

    // make all GPIO input
    SWI2C_SETUP_HIZ();

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
void SWI2C_pins(void)
{
    cdcprintf("-\t-\tCLK\tDAT");
}
void SWI2C_settings(void)
{
    cdcprintf("SWI2C (period hiz)=(%d %d)", period, hiz);
}

void SWI2C_setDATAmode(uint8_t input)
{

    if(input)
    {
        SWI2C_DATA_INPUT();
    }
    else
    {
        // set SDA as output
        if(hiz)
        {
            SWI2C_DATA_OPENDRAIN();
        }
        else
        {
            SWI2C_DATA_PUSHPULL();
        }
    }

}

void SWI2C_search(void){
/*bbH(MOSI + CLK, 0);
//bpWline(OUMSG_I2C_MACRO_SEARCH);
BPMSG1070;
#ifdef BUSPIRATEV4
if (((i2cinternal == 0) && (BP_CLK == 0 || BP_MOSI == 0)) || ((i2cinternal == 1) && (BP_EE_SDA == 0 && BP_EE_SCL == 0))) {
#else
    if (BP_CLK == 0 || BP_MOSI == 0) {
#endif
        BPMSG1019; //warning
        BPMSG1020; //short or no pullups
        bpBR;
        return;
    }
    for (i = 0; i < 0x100; i++) {
#ifdef BP_USE_I2C_HW
        hwi2cstart();
                        hwi2cwrite(i);
                        c = hwi2cgetack();
#else
        bbI2Cstart(); //send start
        bbWriteByte(i); //send address
        c = bbReadBit(); //look for ack
#endif
        if (c == 0) {//0 is ACK

            bpWbyte(i);
            bpWchar('('); //bpWstring("(");
            bpWbyte((i >> 1));
            if ((i & 0b1) == 0) {//if the first bit is set it's a read address, send a byte plus nack to clean up
                bpWstring(" W");
            } else {
#ifdef BP_USE_I2C_HW
                hwi2cread();
                                        hwi2csendack(1); //high bit is NACK
#else
                bbReadByte();
                bbI2Cnack(); //bbWriteBit(1);//high bit is NACK
#endif
                bpWstring(" R");
            }
            bpWstring(")");
            bpSP;
        }
#ifdef BP_USE_I2C_HW
        hwi2cstop();
#else
        bbI2Cstop();
#endif
    }
    bpWBR;
    */
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
