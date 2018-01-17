
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

	if(csidle) spi_set_nss_low(BPSPI);
		else spi_set_nss_high(BPSPI);

	modeConfig.wwr=0;
}

void HWSPI_startr(void)
{
	cdcprintf("set CS=%d", !csidle);

	if(csidle) spi_set_nss_low(BPSPI);
		else spi_set_nss_high(BPSPI);

	modeConfig.wwr=1;
}

void HWSPI_stop(void)
{
	cdcprintf("set CS=%d", csidle);

	if(csidle) spi_set_nss_high(BPSPI);
		else spi_set_nss_low(BPSPI);

	modeConfig.wwr=0;

}

void HWSPI_stopr(void)
{
	cdcprintf("set CS=%d", csidle);

	if(csidle) spi_set_nss_high(BPSPI);
		else spi_set_nss_low(BPSPI);

	modeConfig.wwr=0;
}

uint32_t HWSPI_send(uint32_t d)
{
	uint16_t returnval;

	//TODO: lsb ??
	if((modeConfig.numbits==8)||(modeConfig.numbits==16))
	{
		if(modeConfig.numbits==8) spi_set_dff_8bit(BPSPI);			// is there a less overhead way of doing this?
		if(modeConfig.numbits==16) spi_set_dff_16bit(BPSPI);
	
		returnval=spi_xfer(BPSPI, (uint16_t)d);
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
		if(modeConfig.numbits==8) spi_set_dff_8bit(BPSPI);			// is there a less overhead way of doing this?
		if(modeConfig.numbits==16) spi_set_dff_16bit(BPSPI);

		returnval = spi_xfer(BPSPI, 0xFF);					// is 0xFF ok?
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
	rcc_periph_clock_enable(BPSPICLK);

	// setup gpio asalternate function
	gpio_set_mode(BPSPIMOSIPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPSPIMOSIPIN);
	gpio_set_mode(BPSPICSPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPSPICSPIN);
	gpio_set_mode(BPSPICLKPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPSPICLKPIN);
	gpio_set_mode(BPSPIMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPIMISOPIN);

	// reset all registers
	spi_reset(BPSPI);

	// init BPSPI
	spi_init_master(BPSPI, br, cpol, cpha, dff, lsbfirst);

	// enable fullduplex
	spi_set_full_duplex_mode(BPSPI);

	// we use software control of /cs
	spi_enable_software_slave_management(BPSPI);
	spi_enable_ss_output(BPSPI);

	// cs=1 
	if(csidle) spi_set_nss_high(BPSPI);
		else spi_set_nss_low(BPSPI);
	
	// unleash the beast
	spi_enable(BPSPI);

	// update modeConfig pins
	modeConfig.misoport=BPSPIMISOPORT;
	modeConfig.mosiport=BPSPIMOSIPORT;
	modeConfig.csport=BPSPICSPORT;
	modeConfig.clkport=BPSPICLKPORT;
	modeConfig.misopin=BPSPIMISOPIN;
	modeConfig.mosipin=BPSPIMOSIPIN;
	modeConfig.cspin=BPSPICSPIN;
	modeConfig.clkpin=BPSPICLKPIN;

}

void HWSPI_cleanup(void)
{
	// disable SPI peripheral
	spi_disable(BPSPI);		// spi_clean_disable??

	// set all used pins to input
	gpio_set_mode(BPSPIMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPIMISOPIN);
	gpio_set_mode(BPSPIMOSIPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPIMOSIPIN);
	gpio_set_mode(BPSPICSPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPICSPIN);
	gpio_set_mode(BPSPICLKPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPICLKPIN);

	// disable clock to save the planet warming up
	rcc_periph_clock_disable(BPSPICLK);

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

	temp=SPI_SR(BPSPI);

	if(temp&SPI_SR_BSY) cdcprintf(" BSY");
	if(temp&SPI_SR_OVR) cdcprintf(" OVR");
	if(temp&SPI_SR_MODF) cdcprintf(" MODF");
	if(temp&SPI_SR_CRCERR) cdcprintf(" CRCERR");
	if(temp&SPI_SR_UDR) cdcprintf(" USR");
	if(temp&SPI_SR_CHSIDE) cdcprintf(" CHSIDE");
//	if(temp&SPI_SR_TXE) cdcprintf(" TXE");
//	if(temp&SPI_SR_RXNE) cdcprintf(" RXNE");

}


