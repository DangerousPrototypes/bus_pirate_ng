#include "bitbang.h"

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
#include "base.h"
#include "bitbang.h" //need own functions

#define	BB_5KHZSPEED_SETTLE 20 //~5KHz
#define	BB_5KHZSPEED_CLOCK 100
#define	BB_5KHZSPEED_HALFCLOCK BB_5KHZSPEED_CLOCK/2

#define	BB_50KHZSPEED_SETTLE 2 //~50KHz
#define	BB_50KHZSPEED_CLOCK 10
#define	BB_50KHZSPEED_HALFCLOCK BB_50KHZSPEED_CLOCK/2

#define	BB_100KHZSPEED_SETTLE 1 //~100KHz
#define	BB_100KHZSPEED_CLOCK 5
#define	BB_100KHZSPEED_HALFCLOCK 2

#define	BB_MAXSPEED_SETTLE 0 //~400KHz
#define	BB_MAXSPEED_CLOCK 0
#define	BB_MAXSPEED_HALFCLOCK 0

extern struct _modeConfig modeConfig;

struct _bitbang{
    unsigned char pins;
    unsigned int MOpin;
    unsigned int MIpin;
    unsigned char delaySettle;
    unsigned char delayClock;
    unsigned char delayHalfClock;
} bitbang;

void bbSetup(unsigned char pins, unsigned char speed){

    bitbang.pins=pins;

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
            bitbang.delayHalfClock = BB_5KHZSPEED_HALFCLOCK;
            break;
        case 1:
            bitbang.delaySettle = BB_50KHZSPEED_SETTLE;
            bitbang.delayClock = BB_50KHZSPEED_CLOCK;
            bitbang.delayHalfClock = BB_50KHZSPEED_HALFCLOCK;
            break;
        case 2:
            bitbang.delaySettle = BB_100KHZSPEED_SETTLE;
            bitbang.delayClock = BB_100KHZSPEED_CLOCK;
            bitbang.delayHalfClock = BB_100KHZSPEED_HALFCLOCK;
            break;
        default:
            bitbang.delaySettle = BB_MAXSPEED_SETTLE;
            bitbang.delayClock = BB_MAXSPEED_CLOCK;
            bitbang.delayHalfClock = BB_MAXSPEED_HALFCLOCK;
            break;
    }


}

//
// HELPER functions
//

int bbI2Cstart(void){
    int error=0;
    //http://www.esacademy.com/faq/i2c/busevents/i2cstast.htm
    //setup both lines high first
    BB_HIGH_DELAY(BP_BB_MOSI_PIN|BP_BB_CLK_PIN, bitbang.delayClock);

    //check bus state, return error if held low
    if(checkshort) error=1;

    //now take data low while clock is high
    BB_LOW_DELAY(BP_BB_MOSI_PIN, bitbang.delayClock);

    //next take clock low too
    BB_LOW_DELAY(BP_BB_CLK_PIN, bitbang.delayClock);

    //example suggests returning SDA to high
    BB_HIGH_DELAY(BP_BB_MOSI_PIN, bitbang.delayClock);

    return error;

}
static uint8_t checkshort(void)
{
    uint8_t temp;

    temp=(gpio_get(BP_I2C_SDA_SENSE_PORT, BP_I2C_SDA_SENSE_PIN)==0?1:0);
    temp|=(gpio_get(BP_I2C_SCL_SENSE_PORT, BP_I2C_SCL_SENSE_PIN)==0?2:0);

    return (temp==3);			// there is only a short when both are 0 otherwise repeated start wont work
}

int bbI2Cstop(void){
    //http://www.esacademy.com/faq/i2c/busevents/i2cstast.htm

    //setup both lines low first
    //example suggests just SDA, but some chips are flakey.
    BB_LOW_DELAY(BP_BB_MOSI_PIN|BP_BB_CLK_PIN, bitbang.delayClock);

    //take clock high
    BB_HIGH_DELAY(BP_BB_CLK_PIN, bitbang.delayClock);

    //with clock high, bring data high too
    BB_HIGH_DELAY(BP_BB_MOSI_PIN, bitbang.delayClock);

    //return clock low, important for raw2wire smartcard
    //BB_LOW_DELAY(BP_SW_CLK_PIN, bitbang.delayClock);
    return 0;
}

//
//BYTE functions
//

// ** Read with write for 3-wire protocols ** //

//unsigned char bbReadWriteByte(unsigned char c){
unsigned int bbReadWrite(unsigned int c){
    unsigned int i,bt,di,dat=0;

    //begin with clock low...
    bt=1<<(modeConfig.numbits-1);

    for(i=0;i<modeConfig.numbits;i++){
        bbPins((c&bt), BP_BB_MOSI_PIN, bitbang.delaySettle); //set data out
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);//set clock high
        di=bbR(BP_BB_MISO_PIN); //read data pin
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);;//set clock low

        //get MSB first
        c=c<<1;  //shift data output bits
        dat=dat<<1;  //shift the data input byte bits
        if(di)dat++; //if datapin in is high, set LBS
    }

    return dat;
}

// ** Separate read/write for 2-wire protocols ** //

void bbWrite(unsigned int c){
    unsigned int i,bt;

    //bbo();//prepare for output

    //bt=0x80;
    bt=1<<(modeConfig.numbits-1);

    for(i=0;i<modeConfig.numbits;i++){
        bbPins((c&bt), BP_BB_MOSI_PIN, bitbang.delaySettle );
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);

        tem=tem<<1; //next output bit

    }
}

unsigned int bbRead(void){
    unsigned int i,di,dat=0;

    //bbi();//prepare for input
    bbR(bitbang.MIpin); //setup for input

    for(i=0;i<modeConfig.numbits;i++){
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);//set clock high
        di=bbR(bitbang.MIpin); //same as BP_SW_MISO_PIN on 2-wire
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);;//set clock low

        //get MSB first
        dat=dat<<1;//shift the data input byte bits
        if(di)dat++;//if datapin in is high, set LBS
    }
    return dat;
}

//
// BIT functions
//

unsigned char bbReadBit(void){
    unsigned char c;

    bbR(bitbang.MIpin); //setup for input
    BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);//set clock high
    c=bbR(bitbang.MIpin);
    BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);;//set clock low
    return c;
}

void bbWriteBit(unsigned char c){

    bbPins(c,BP_BB_MOSI_PIN, bitbang.delaySettle);

    BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
    BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
}

void bbClockTicks(unsigned char c){
    unsigned char i;

    for(i=0;i<c;i++){
        BB_HIGH_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
        BB_LOW_DELAY(BP_BB_CLK_PIN,bitbang.delayClock);
    }

}

//
// PIN functions
//
void bbMOSI(unsigned char dir){
    bbPins(dir, BP_BB_MOSI_PIN, bitbang.delaySettle);
}
void bbCLK(unsigned char dir){
    bbPins(dir, BP_BB_CLK_PIN, bitbang.delaySettle);
}
void bbCS(unsigned char dir){
    bbPins(dir, BP_BB_CS_PIN, bitbang.delaySettle);
}
unsigned char bbMISO (void){
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

void bbPins(unsigned int dir, unsigned int pins, unsigned char delay){
    if(dir==0){
        BB_LOW_DELAY(pins,delay);
    }else{
        BB_HIGH_DELAY(pins,delay);
    }
}

unsigned char bbR(unsigned int pin){
    IODIR |= pin; //pin as input
    Nop();
    Nop();
    Nop();
    if(IOPOR & pin) return 1; else return 0;//clear all but pin bit and return result
}




