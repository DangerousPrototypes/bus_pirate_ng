/*
 * This file is part of the Bus Pirate project (http://code.google.com/p/the-bus-pirate/).
 *
 * Written and maintained by the Bus Pirate project.
 *
 * To the extent possible under law, the project has
 * waived all copyright and related or neighboring rights to Bus Pirate. This
 * work is published from United States.
 *
 * For details see: http://creativecommons.org/publicdomain/zero/1.0/.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
//Bus Pirate bitbang base library
//consolidates all bitbang code into one place

// The software i2c routines were written in c from public domain pseudo code:
/// **** I2C Driver V1.1 Written by V.Himpe. Released as Public Domain **** /
// http://www.esacademy.com/faq/i2c/general/i2cpseud.htm
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"
#include "UI.h"
#include "bitbang.h" //need own functions

#define	BB_5KHZSPEED_SETTLE 20 //~5KHz
#define	BB_5KHZSPEED_CLOCK 100

#define	BB_50KHZSPEED_SETTLE 2 //~50KHz
#define	BB_50KHZSPEED_CLOCK 10

#define	BB_100KHZSPEED_SETTLE 1 //~100KHz
#define	BB_100KHZSPEED_CLOCK 5

#define	BB_MAXSPEED_SETTLE 0 //~400KHz
#define	BB_MAXSPEED_CLOCK 0

//extern struct _modeConfig modeConfig;

struct _bitbang{
    unsigned int MOpin;
    unsigned int MIpin;
    unsigned char delaySettle;
    unsigned char delayClock;
} bitbang;

void bbSetup(uint8_t pins, uint8_t speed){

    //define pins for 2 or 3 wire modes (do we use a seperate input pin)
    if(pins==3){ //SPI-like
        bitbang.MOpin=BP_BB_MOSI_PIN;
        bitbang.MIpin=BP_BB_MISO_PIN;
    }else{ //I2C-like
        bitbang.MOpin=BP_BB_MOSI_PIN;
        bitbang.MIpin=BP_BB_MOSI_PIN;
    }

    //define delays for differnt speeds
    // I2C Bus timing in uS
    switch(speed){
        case 0:
            bitbang.delaySettle = BB_5KHZSPEED_SETTLE;
            bitbang.delayClock = BB_5KHZSPEED_CLOCK;
            break;
        case 1:
            bitbang.delaySettle = BB_50KHZSPEED_SETTLE;
            bitbang.delayClock = BB_50KHZSPEED_CLOCK;
            break;
        case 2:
            bitbang.delaySettle = BB_100KHZSPEED_SETTLE;
            bitbang.delayClock = BB_100KHZSPEED_CLOCK;
            break;
        default:
            bitbang.delaySettle = BB_MAXSPEED_SETTLE;
            bitbang.delayClock = BB_MAXSPEED_CLOCK;
            break;
    }

    if(modeConfig.hiz){
        BB_SETUP_OPENDRAIN();
    }else{
        BB_SETUP_PUSHPULL();
    }

    BB_LOW(BP_BB_MOSI_PIN|BP_BB_CLK_PIN|BP_BB_CS_PIN);
}

//
// HELPER functions
//

uint8_t bbI2Cstart(void){
    int error=0;
    //http://www.esacademy.com/faq/i2c/busevents/i2cstast.htm
    //setup both lines high first
    BB_HIGH_DELAY(BP_BB_MOSI_PIN|BP_BB_CLK_PIN, bitbang.delayClock);

    //check bus state, return error if held low
    //if(checkshort) error=1;

    //now take data low while clock is high
    BB_LOW_DELAY(BP_BB_MOSI_PIN, bitbang.delayClock);

    //next take clock low too
    BB_LOW_DELAY(BP_BB_CLK_PIN, bitbang.delayClock);

    //example suggests returning SDA to high
    BB_HIGH_DELAY(BP_BB_MOSI_PIN, bitbang.delayClock);

    return error;

}
/*static uint8_t checkshort(void)
{
    uint8_t temp;

    temp=(gpio_get(BP_I2C_SDA_SENSE_PORT, BP_I2C_SDA_SENSE_PIN)==0?1:0);
    temp|=(gpio_get(BP_I2C_SCL_SENSE_PORT, BP_I2C_SCL_SENSE_PIN)==0?2:0);

    return (temp==3);			// there is only a short when both are 0 otherwise repeated start wont work
}*/

uint8_t bbI2Cstop(void){
    //http://www.esacademy.com/faq/i2c/busevents/i2cstast.htm

    //setup both lines low first
    //example suggests just SDA, but some chips are flakey.
    BB_LOW_DELAY(BP_BB_MOSI_PIN|BP_BB_CLK_PIN, bitbang.delayClock);

    //take clock high
    BB_HIGH_DELAY(BP_BB_CLK_PIN, bitbang.delayClock);

    //with clock high, bring data high too
    BB_HIGH_DELAY(BP_BB_MOSI_PIN, bitbang.delayClock);

    return 0;
}

//
//BYTE functions
//

// ** Read with write for 3-wire protocols ** //

//unsigned char bbReadWriteByte(unsigned char c){
uint32_t bbReadWrite(uint32_t c){
    uint8_t i;
    uint32_t bt,dat=0;

    //begin with clock low...
    bt=1<<(modeConfig.numbits-1);

    for(i=0;i<modeConfig.numbits;i++){
        bbPins((c&bt), BP_BB_MOSI_PIN, bitbang.delaySettle); //set data out
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);//set clock high

        //get MSB first
        dat=dat<<1;  //shift the data input byte bits

        dat|=(BB_DATA_READ(BP_BB_MISO_PIN)?1:0); //read data pin
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);;//set clock low

        c=c<<1;  //shift data output bits

    }

    return dat;
}

// ** Separate read/write for 2-wire protocols ** //
void bbWrite(uint32_t c){
    uint8_t i;
    uint32_t bt;

    bt=1<<(modeConfig.numbits-1);

    for(i=0;i<modeConfig.numbits;i++){
        bbPins((c&bt), BP_BB_MOSI_PIN, bitbang.delaySettle );
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);

        c=c<<1; //next output bit

    }
}

uint32_t bbRead(void){
    uint8_t i;
    uint32_t dat=0;

    //bbi();//prepare for input
    bbR(bitbang.MIpin); //setup for input

    for(i=0;i<modeConfig.numbits;i++){
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);//set clock high

        //get MSB first
        dat=dat<<1;//shift the data input byte bits
        dat|=bbR(bitbang.MIpin); //same as BP_SW_MISO_PIN on 2-wire

        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);;//set clock low


    }
    return dat;
}

//
// BIT functions
//

uint8_t bbReadBit(void){
    unsigned char c;

    bbR(bitbang.MIpin); //setup for input
    BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);//set clock high
    c=bbR(bitbang.MIpin);
    BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);;//set clock low
    return c;
}

void bbWriteBit(uint8_t c){

    bbPins(c,BP_BB_MOSI_PIN, bitbang.delaySettle);

    BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
    BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
}

void bbClockTicks(uint32_t c){
    uint32_t i;

    for(i=0;i<c;i++){
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
    }

}

//
// PIN functions
//
void bbMOSI(uint8_t dir){
    bbPins(dir, BP_BB_MOSI_PIN, bitbang.delaySettle);
}
void bbCLK(uint8_t dir){
    bbPins(dir, BP_BB_CLK_PIN, bitbang.delaySettle);
}
void bbCS(uint8_t dir){
    bbPins(dir, BP_BB_CS_PIN, bitbang.delaySettle);
}
uint8_t bbMISO (void){
    return bbR(bitbang.MIpin);
}

//
// BASE IO functions
//
/*void BB_HIGH_DELAY(unsigned int pins, unsigned char delay){
    BB_HIGH(pins);
    bpDelayUS(delay);//delay
}

void BB_LOW_DELAY(unsigned int pins, unsigned char delay){
    BB_LOW(pins);
    bpDelayUS(delay);//delay
}*/

void bbPins(uint8_t dir, uint32_t pins, uint32_t delay){
    if(dir==0){
        BB_LOW(pins);
    }else{
        BB_HIGH(pins);
    }
    //ensure pin is output from any previous reads...
    if(modeConfig.hiz){
        BB_OUTPUT_OPENDRAIN(pins);
    }else{
        BB_OUTPUT_PUSHPULL(pins);
    }
    delayus(delay);
}

uint8_t bbR(uint32_t pin){
    BB_INPUT(pin); //pin as input
    return (BB_DATA_READ(pin)?1:0);//clear all but pin bit and return result
}



