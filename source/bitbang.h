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
//setup the library first
void bbSetup(unsigned char pins, unsigned char speed);

//read/write number of bits in modeConfig.bits
unsigned int bbReadWrite(unsigned int c);
void bbWrite(unsigned int c);
unsigned int bbRead(void);

//bit read and write functions
unsigned char bbReadBit(void);
void bbWriteBit(unsigned char c);
void bbClockTicks(unsigned char c);

//generic pin direction functions including delays for bitwise pin functions
void bbMOSI(unsigned char dir);
void bbCLK(unsigned char dir);
void bbCS(unsigned char dir);
unsigned char bbMISO (void);

//pin twiddling functions with delays
void bbH(unsigned int pins, unsigned char delay);
void bbL(unsigned int pins, unsigned char delay);
void bbPins(unsigned int dir, unsigned int pins, unsigned char delay);
unsigned char bbR(unsigned int pin);

//protocol helper functions
int bbI2Cstart(void);
int bbI2Cstop(void);

//protocol specific pseudo functions
#define bbI2Cack()  bbWriteBit(0) //low bit is ACK
#define bbI2Cnack()  bbWriteBit(1) //high bit is NACK

#define BB_SET(port,pins) gpio_set(port,pins)
#define BB_CLEAR(port,pins) gpio_clear(port, pins)
#define BB_GET(port,pin) gpio_get(port,pin)

#define BB_HIGH(pins) BB_SET(BP_BB_PORT, pins)
#define BB_LOW(pins) BB_CLEAR(BP_BB_PORT, pins)

#define BB_HIGH_DELAY(pins, delay) BB_HIGH(pins); bpDelayUS(delay)
#define BB_LOW_DELAY(pins, delay) BB_LOW(pins); bpDelayUS(delay)

#define BB_DATA_HIGH() BB_SET(BP_BB_PORT,BP_BB_MOSI_PIN)
#define BB_DATA_LOW() BB_CLEAR(BP_BB_PORT,BP_BB_MOSI_PIN)

#define BB_CLOCK_HIGH() BB_SET(BP_BB_PORT, BP_BB_CLK_PIN)
#define BB_CLOCK_LOW() BB_CLEAR(BP_BB_PORT, BP_BB_CLK_PIN)

//COULD BE MISO OR MOSI IN 2WIRE MODE
#define BB_DATA_READ(pin) BB_GET(BP_BB_PORT, pin)

#define BB_CS_HIGH() BB_SET(BP_BB_PORT,BP_BB_CS_PIN)
#define BB_CS_LOW() BB_GET(BP_BB_PORT,BP_BB_CS_PIN)

#define BB_OUTPUT_OPENDRAIN(pins) gpio_set_mode(BP_BB_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, pins)
#define BB_OUTPUT_PUSHPULL(pins) gpio_set_mode(BP_BB_PORT, GPIO_MODE_OUTPUT_10_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, pins)
#define BB_INPUT(pins) gpio_set_mode(BP_BB_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,pins)

#define BB_SETUP_OPENDRAIN() BB_OUTPUT_OPENDRAIN(BP_BB_PORT,BP_BB_MOSI_PIN|BP_BB_CLK_PIN|BP_BB_CS_PIN); BB_INPUT(BP_BB_PORT,BP_BB_MISO_PIN)
#define BB_SETUP_PUSHPULL() BB_OUTPUT_PUSHPULL(BP_BB_PORT,BP_BB_MOSI_PIN|BP_BB_CLK_PIN|BP_BB_CS_PIN); BB_INPUT(BP_BB_PORT,BP_BB_MISO_PIN)
#define BB_SETUP_HIZ() BB_INPUT(BP_BB_PORT,BP_BB_MOSI_PIN|BP_BB_CLK_PIN|BP_BB_CS_PIN|BP_BB_MISO_PIN)

