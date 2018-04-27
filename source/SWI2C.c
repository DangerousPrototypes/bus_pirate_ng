#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"
#include "UI.h"
#include "SWI2C.h"
#include "cdcacm.h"

void I2Csearch(void);

static uint32_t	period;
static uint8_t	hiz;

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

    for(i=0; i<modeConfig.numbits; i++)
    {

        delayus(period/2); //delay low

        SWI2C_CLOCK_HIGH(); //high
        delayus(period/2); //delay high
        //read data
        if(gpio_get(BP_SWI2C_SDA_PORT, BP_SWI2C_SDA_PIN)) returnval|=1;
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
        case 1:		SWI2Csearch();
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
        gpio_set_mode(BP_SWI2C_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SWI2C_SDA_PIN);
        gpio_set_mode(BP_SWI2C_CLK_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SWI2C_CLK_PIN);
    }
    else
    {
        gpio_set_mode(BP_SWI2C_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SWI2C_SDA_PIN);
        gpio_set_mode(BP_SWI2C_CLK_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SWI2C_CLK_PIN);
    }

    // update modeConfig pins
    modeConfig.mosiport=BP_SWI2C_SDA_PORT;
    modeConfig.clkport=BP_SWI2C_CLK_PORT;

    //a guess... 72 period in the PWM is .99999uS. Multiply the period in uS * 72, divide by 4 four 4* over sample
    modeConfig.logicanalyzerperiod=((period*72)/4);

}

void SWI2C_cleanup(void)
{
    cdcprintf("SWI2C cleanup()");

    // make all GPIO input
    gpio_set_mode(BP_SWI2C_SDA_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SWI2C_SDA_PIN);
    gpio_set_mode(BP_SWI2C_CLK_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SWI2C_CLK_PIN);

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
        gpio_set_mode(BP_SWI2C_SDA_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SWI2C_SDA_PIN);
    }
    else
    {
        // set SDA as output
        if(hiz)
        {
            gpio_set_mode(BP_SWI2C_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_SWI2C_SDA_PIN);
        }
        else
        {
            gpio_set_mode(BP_SWI2C_SDA_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SWI2C_SDA_PIN);
        }
    }

}

void I2Csearch(void){
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
