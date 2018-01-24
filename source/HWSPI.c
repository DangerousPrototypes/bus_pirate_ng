
#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "HWSPI.h"
#include "cdcacm.h"
#include "UI.h"


static uint32_t cpol, cpha, br, dff, lsbfirst, csidle;

void HWSPI_start(void)
{
	cdcprintf("set CS=%d", !csidle);

	if(csidle) spi_set_nss_low(BP_SPI);
		else spi_set_nss_high(BP_SPI);

	modeConfig.wwr=0;
}

void HWSPI_startr(void)
{
	cdcprintf("set CS=%d", !csidle);

	if(csidle) spi_set_nss_low(BP_SPI);
		else spi_set_nss_high(BP_SPI);

	modeConfig.wwr=1;
}

void HWSPI_stop(void)
{
	cdcprintf("set CS=%d", csidle);

	if(csidle) spi_set_nss_high(BP_SPI);
		else spi_set_nss_low(BP_SPI);

	modeConfig.wwr=0;

}

void HWSPI_stopr(void)
{
	cdcprintf("set CS=%d", csidle);

	if(csidle) spi_set_nss_high(BP_SPI);
		else spi_set_nss_low(BP_SPI);

	modeConfig.wwr=0;
}

uint32_t HWSPI_send(uint32_t d)
{
	uint16_t returnval;

	//TODO: lsb ??
	if((modeConfig.numbits==8)||(modeConfig.numbits==16))
	{
		if(modeConfig.numbits==8) spi_set_dff_8bit(BP_SPI);			// is there a less overhead way of doing this?
		if(modeConfig.numbits==16) spi_set_dff_16bit(BP_SPI);
	
		returnval=spi_xfer(BP_SPI, (uint16_t)d);
	}
	else
	{
		cdcprintf("Only 8 or 16 bits are allowed, use SW3W instead");
		modeConfig.error=1;
		returnval=0;
	}

	return (uint16_t) returnval;
}

uint32_t HWSPI_read(void)
{
	uint16_t returnval;

	//TODO: check lsb??
	if((modeConfig.numbits==8)||(modeConfig.numbits==16))
	{
		if(modeConfig.numbits==8) spi_set_dff_8bit(BP_SPI);			// is there a less overhead way of doing this?
		if(modeConfig.numbits==16) spi_set_dff_16bit(BP_SPI);

		returnval = spi_xfer(BP_SPI, 0xFF);					// is 0xFF ok?
	}
	else
	{
		cdcprintf("Only 8 or 16 bits are allowed, use SW3W instead");
		modeConfig.error=1;
		returnval=0;
	}

	return (uint16_t) returnval;
}

void HWSPI_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf("No macros available");
				break;
		default:	cdcprintf("Macro not defined");
				modeConfig.error=1;
	}
}

void HWSPI_setup(void)
{
	// did the user leave us arguments?
	// baudrate
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	br=getint();
	if((br>=1)&&(br<=7)) br<<=3;
		else modeConfig.error=1;

	// clock polarity
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	cpol=getint()-1;
	if(cpol<=1) cpol<<=1;
		else modeConfig.error=1;

	// clock phase
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	cpha=getint()-1;
	if(cpha<=1) cpha=cpha;
		else modeConfig.error=1;

	// cs behauviour
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	csidle=getint()-1;
	if(csidle<=1) csidle=csidle;
		else modeConfig.error=1;

	// did the user did it right?
	if(modeConfig.error)			// go interactive 
	{

		br=(askint(SPISPEEDMENU, 1, 7, 5))<<3;
		cpol=((askint(SPICPOLMENU, 1, 2, 2)-1)<<1);
		cpha=(askint(SPICPHAMENU, 1, 2, 2)-1);
		csidle=(askint(SPICSIDLEMENU, 1, 2, 2)-1);

		// 8bit and lsb/msb handled in UI.c
		dff=SPI_CR1_DFF_8BIT;
		lsbfirst=SPI_CR1_MSBFIRST;
	}
}

void HWSPI_setup_exc(void)
{
	// start the clock
	rcc_periph_clock_enable(BP_SPI_CLK);

	// setup gpio asalternate function
	gpio_set_mode(BP_SPI_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_SPI_MOSI_PIN);
	gpio_set_mode(BP_SPI_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_SPI_CS_PIN);
	gpio_set_mode(BP_SPI_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_SPI_CLK_PIN);
	gpio_set_mode(BP_SPI_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SPI_MISO_PIN);

	// reset all registers
	spi_reset(BP_SPI);

	// init BPSPI
	spi_init_master(BP_SPI, br, cpol, cpha, dff, lsbfirst);

	// enable fullduplex
	spi_set_full_duplex_mode(BP_SPI);

	// we use software control of /cs
	spi_enable_software_slave_management(BP_SPI);
	spi_enable_ss_output(BP_SPI);

	// cs=1 
	if(csidle) spi_set_nss_high(BP_SPI);
		else spi_set_nss_low(BP_SPI);
	
	// unleash the beast
	spi_enable(BP_SPI);

	// update modeConfig pins
	modeConfig.misoport=BP_SPI_MISO_PORT;
	modeConfig.mosiport=BP_SPI_MOSI_PORT;
	modeConfig.csport=BP_SPI_CS_PORT;
	modeConfig.clkport=BP_SPI_CLK_PORT;
	modeConfig.misopin=BP_SPI_MISO_PIN;
	modeConfig.mosipin=BP_SPI_MOSI_PIN;
	modeConfig.cspin=BP_SPI_CS_PIN;
	modeConfig.clkpin=BP_SPI_CLK_PIN;

}

void HWSPI_cleanup(void)
{
	// disable SPI peripheral
	spi_disable(BP_SPI);		// spi_clean_disable??

	// set all used pins to input
	gpio_set_mode(BP_SPI_MISO_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SPI_MISO_PIN);
	gpio_set_mode(BP_SPI_MOSI_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SPI_MOSI_PIN);
	gpio_set_mode(BP_SPI_CS_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SPI_CS_PIN);
	gpio_set_mode(BP_SPI_CLK_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_SPI_CLK_PIN);

	// disable clock to save the planet warming up
	rcc_periph_clock_disable(BP_SPI_CLK);

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

void HWSPI_pins(void)
{
	cdcprintf("CS\tMISO\tCLK\tMOSI");
}

void HWSPI_settings(void)
{
	cdcprintf("HWSPI (br cpol cpha cs)=(%d %d %d %d)", (br>>3), (cpol>>1)+1, cpha+1, csidle+1);
}

void HWSPI_printSPIflags(void)
{
	uint32_t temp;

	temp=SPI_SR(BP_SPI);

	if(temp&SPI_SR_BSY) cdcprintf(" BSY");
	if(temp&SPI_SR_OVR) cdcprintf(" OVR");
	if(temp&SPI_SR_MODF) cdcprintf(" MODF");
	if(temp&SPI_SR_CRCERR) cdcprintf(" CRCERR");
	if(temp&SPI_SR_UDR) cdcprintf(" USR");
	if(temp&SPI_SR_CHSIDE) cdcprintf(" CHSIDE");
//	if(temp&SPI_SR_TXE) cdcprintf(" TXE");
//	if(temp&SPI_SR_RXNE) cdcprintf(" RXNE");

}

void HWSPI_help(void)
{
	cdcprintf("Peer to peer 3 or 4 wire full duplex protocol. Very\r\n");
	cdcprintf("high clockrates upto 20MHz are possible.\r\n");
	cdcprintf("\r\n");
	cdcprintf("More info: https://en.wikipedia.org/wiki/Serial_Peripheral_Interface_Bus\r\n");
	cdcprintf("\r\n");


	cdcprintf("BPCMD\t {,] |                 DATA (1..32bit)               | },]\r\n");
	cdcprintf("CMD\tSTART| D7  | D6  | D5  | D4  | D3  | D2  | D1  | D0  | STOP\r\n");

	if(cpha)
	{	
		cdcprintf("MISO\t-----|{###}|{###}|{###}|{###}|{###}|{###}|{###}|{###}|------\r\n");
		cdcprintf("MOSI\t-----|{###}|{###}|{###}|{###}|{###}|{###}|{###}|{###}|------\r\n");
	}
	else
	{
		cdcprintf("MISO\t---{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}--|------\r\n");
		cdcprintf("MOSI\t---{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}{#|##}--|------\r\n");
	}

	if(cpol>>1)
		cdcprintf("CLK     \"\"\"\"\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"__\"|\"\"\"\"\"\"\r\n");
	else
		cdcprintf("CLK\t_____|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|__\"\"_|______\r\n");

	if(csidle)
		cdcprintf("CS\t\"\"___|_____|_____|_____|_____|_____|_____|_____|_____|___\"\"\"\r\n");
	else
		cdcprintf("CS\t__\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"\"\"|\"\"\"___\r\n");

	cdcprintf("\r\nCurrent mode is CPHA=%d and CPOL=%d\r\n",cpha, cpol>>1);
	cdcprintf("\r\n");
	cdcprintf("Connection:\r\n");
	cdcprintf("\tMOSI \t------------------ MOSI\r\n");
	cdcprintf("\tMISO \t------------------ MISO\r\n");
	cdcprintf("{BP}\tCLK\t------------------ CLK\t{DUT}\r\n");
	cdcprintf("\tCS\t------------------ CS\r\n");
	cdcprintf("\tGND\t------------------ GND\r\n");
}
