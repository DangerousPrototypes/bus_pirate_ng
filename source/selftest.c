#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>

#include "debug.h"
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"
#include "ADC.h"
#include "selftest.h"
#include "LA.h"



// pins directly tested
#define DIRECT_PIN_TESTS_PP 9
#define DIRECT_PIN_TESTS_OD 7		// AUX isn't connected through the 4066
struct _testpin testpins[]={
// MOSI
{ GPIOB, GPIOB, GPIO7, GPIO10 },	// mosi PB10
{ GPIOB, GPIOB, GPIO10, GPIO15 },	// mosi PB15
{ GPIOB, GPIOB, GPIO15, GPIO7 },	// mosi PB7
// MISO
{ GPIOB, GPIOB, GPIO11, GPIO14 },	// MISO PB14
{ GPIOB, GPIOB, GPIO14, GPIO11 },	// MISO PB11
// CLOCK
{ GPIOB, GPIOB, GPIO6, GPIO13 },	// CLK PB13
{ GPIOB, GPIOB, GPIO13, GPIO6 },	// CLK PB6
// CS cant be checked now (place jumper between aux and CS??)
// AUX
{ GPIOB, GPIOD, GPIO0, GPIO2 },		// AUX PD2
{ GPIOD, GPIOB, GPIO2, GPIO0 },		// AUX PB0
};

// pins tested through latch
#define LA_PIN_TESTS 10
struct _testpin latestpins[]={
// MOSI
{ GPIOB, GPIOA, GPIO7, GPIO0 },		// MOSI PB7
{ GPIOB, GPIOA, GPIO10, GPIO0 },	// MOSI PB10
{ GPIOB, GPIOA, GPIO15, GPIO0 },	// MOSI PB15
// MISO
{ GPIOB, GPIOA, GPIO11, GPIO2 },	// MISO PB11
{ GPIOB, GPIOA, GPIO14, GPIO2 },	// MISO PB14
// CLOCK
{ GPIOB, GPIOA, GPIO6, GPIO1 },		// CLK PB6
{ GPIOB, GPIOA, GPIO13, GPIO1 },	// CLK PB13
// CS
{ GPIOB, GPIOA, GPIO12, GPIO3 },	// CS PB12
// AUX
{ GPIOB, GPIOA, GPIO0, GPIO4 },		// AUX PB0
{ GPIOD, GPIOA, GPIO2, GPIO4 },		// AUX PD2
};




void selftest(void)
{
	int i, errors;
	uint16_t volt;

	errors=0;

	if(modeConfig.mode!=0)
	{
		cdcprintf("Selftest must be run in HiZ mode!!\r\n");
		modeConfig.error=1;
		return;
	}

	cdcprintf("Checking ADC:\r\n");

	gpio_set(BP_PSUEN_PORT, BP_PSUEN_PIN);	// turn PSU on
	delayms(10);

	volt=voltage(BP_3V3_CHAN, 1);
	cdcprintf(" 3V3=%d.%02dV ", volt / 1000, (volt % 1000) / 10);
	if((volt<3200)||(volt>3400))
	{	
		cdcprintf("NOK\r\n");
		errors++;
	}
	else
	{
		cdcprintf("OK\r\n");
	}

	volt=voltage(BP_5V0_CHAN, 1);;
	cdcprintf(" 5V0=%d.%03dV ", volt / 1000, (volt % 1000) / 10);
	if((volt<4900)||(volt>5100))
	{	
		cdcprintf("NOK\r\n");
		errors++;
	}
	else
	{
		cdcprintf("OK\r\n");
	}


	volt=voltage(BP_VPU_CHAN, 1); 		// voltage on resistor
	cdcprintf(" Vpu=%d.%02dV ", volt / 1000, (volt % 1000) / 10);
	if((volt<3200)||(volt>3400)) 		// TODO: which voltages are ok?
	{	
		cdcprintf("NOK\r\n");
		errors++;
	}
	else
	{
		cdcprintf("OK\r\n");
	}

	volt=voltage(BP_VEXT_CHAN, 1); 		// voltage on Vpu pin
	cdcprintf(" Vext=%d.%02dV ", volt / 1000, (volt % 1000) / 10);
	if((volt<3200)||(volt>3400)) 		// TODO: whihc voltages are ok?
	{	
		cdcprintf("NOK\r\n");
		errors++;
	}
	else
	{
		cdcprintf("OK\r\n");
	}

	volt=voltage(BP_ADC_CHAN, 1);
	cdcprintf(" ADC=%d.%02dV ", volt / 1000, (volt % 1000) / 10);
	if((volt<3200)||(volt>3400)) 		// TODO: which votlage are ok?
	{	
		cdcprintf("NOK\r\n");
		errors++;
	}
	else
	{
		cdcprintf("OK\r\n");
	}

	gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);// turn PSU off
	delayms(10);

	cdcprintf("Checking pins pushpull ");

	BP_LA_LATCH_SETUP();			// TODO: needed?
	BP_LA_LATCH_CLOSE();			// TODO: needed?

	for(i=0; i<DIRECT_PIN_TESTS_PP; i++)
	{
		if(checkpin(testpins[i].portout, testpins[i].pinout, testpins[i].portin, testpins[i].pinin, MODE_PP))
		{
			cdcputc('.');
		}
		else
		{
			errors++;
			cdcputc('!');
		}
	}
	cdcprintf("\r\n");

	cdcprintf("Checking pins opendrain ");

	gpio_set(BP_VPUEN_PORT, BP_VPUEN_PIN);			// enable pullups

	for(i=0; i<DIRECT_PIN_TESTS_OD; i++)			// AUX isn't connected to the 4066 and can;t be pulled high
	{
		if(checkpin(testpins[i].portout, testpins[i].pinout, testpins[i].portin, testpins[i].pinin, MODE_OD))
		{
			cdcputc('.');
		}
		else
		{
			errors++;
			cdcputc('!');
		}
	}
	cdcprintf("\r\n");

	gpio_clear(BP_VPUEN_PORT, BP_VPUEN_PIN);		// disable pullups

	cdcprintf("Checking pins through LA LATCH ");

	BP_LA_LATCH_SETUP();
	BP_LA_LATCH_OPEN();

	for(i=0; i<LA_PIN_TESTS; i++)
	{
		if(checkpin(latestpins[i].portout, latestpins[i].pinout, latestpins[i].portin, latestpins[i].pinin, MODE_PP))
		{
			cdcputc('.');
		}
		else
		{
			errors++;
			cdcputc('!');
		}
	}

	BP_LA_LATCH_CLOSE();
	BP_LA_LATCH_CLEANUP();

	cdcprintf("\r\n");

	cdcprintf("Checking SRAM\r\n");

	BP_LA_SRAM_CS_SETUP();			// TODO: needed?
	BP_LA_SRAM_CLOCK_SETUP();		// TODO: needed?

	errors+=checksram(0x00);
	errors+=checksram(0xff);
	errors+=checksram(0xAA);
	errors+=checksram(0x55);

	BP_LA_SRAM_CS_CLEANUP();
	BP_LA_SRAM_CLOCK_CLEANUP();

	cleanup_spi();				// TODO: needed?

	cdcprintf("Selftest ended, found %d errors\r\n", errors);
	cdcprintf("Press any key to continue\r\n");

	if(cdcgetc()!='t')
		return;

	cdcprintf("HiZ test on CS\r\n");
	
	gpio_set(BP_VPUEN_PORT, BP_VPUEN_PIN);			// enable pullups

	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO12);
	cdcprintf("input-float\r\n");
	cdcgetc();
	
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO12);
	gpio_set(GPIOB, GPIO12);
	cdcprintf("input-pull-up\r\n");
	cdcgetc();
	
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO12);
	gpio_clear(GPIOB, GPIO12);
	cdcprintf("input-pull-down\r\n");
	cdcgetc();
	
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, GPIO12);
	cdcprintf("input-analog\r\n");
	cdcgetc();
	
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO12);
	gpio_set(GPIOB, GPIO12);
	cdcprintf("output-od-high\r\n");
	cdcgetc();
	
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, GPIO12);
	gpio_clear(GPIOB, GPIO12);
	cdcprintf("output-od-low\r\n");
	cdcgetc();
	
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO12);
	gpio_clear(BP_VPUEN_PORT, BP_VPUEN_PIN);			// disable pullups
}


// does a check between connected pins
// mode=0 pushpull
// mode=1 opendrain
int checkpin(uint32_t portout, uint16_t pinout, uint32_t portin, uint16_t pinin, int mode)
{
	int errors=0;

	// set to output
	if(mode==0)
		gpio_set_mode(portout, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, pinout);
	else
		gpio_set_mode(portout, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, pinout);

	// set to 1, check if pin under test=1
	gpio_set(portout, pinout);

delayms(1);

	if(gpio_get(portin, pinin)==0)
	{
		cdcputc('1');
		errors++;
	}

	// set to 0, check if pin under test=0
	gpio_clear(portout, pinout);

delayms(1);

	if(gpio_get(portin, pinin) != 0)
	{
		cdcputc('0');
		errors++;
	}

	// reset pin to input
	gpio_set_mode(portout, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, pinout);

	// return 1 if no errors

	if(errors) return 0;

	return 1;
}

int checksram(uint8_t fillbyte)
{
	int i, errors;

	errors=0;

	//quad mode
	setup_spix1rw();
	BP_LA_SRAM_SELECT();  
	spiWx1(CMDQUADMODE);
	BP_LA_SRAM_DESELECT();

 	setup_spix4w();
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address

	// fill sram with fillbyte
	for(i=0; i<BP_LA_SRAM_SIZE; i++)
	{
		spiWx4(fillbyte);

		if((i&0x0FF)==0)
			cdcprintf(" Write address 0x%08X\r", i);
	}
	BP_LA_SRAM_DESELECT();

	cdcprintf("\r\n");

	// read back result and compare it
// 	setup_spix4w();
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDREAD); //read command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	setup_spix4r(); //read
	spiRx4(); //dummy byte
	spiRx4(); //dummy byte (need two reads to clear one byte)

	for(i=0; i<BP_LA_SRAM_SIZE; i++)
	{
		if(spiRx4()!=fillbyte)
		{
			errors++;
			cdcprintf("\r\n  ERROR @ 0x%08X\r\n", i);
		}

		if((i&0x0FF)==0)
			cdcprintf(" Read address 0x%08X\r", i);
	}

	//send mode reset command just in case
	setup_spix4w(); //write
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDRESETSPI); //write command
	BP_LA_SRAM_DESELECT();  

	cdcprintf("\r\n");

	cleanup_spi();

	return errors;
}
