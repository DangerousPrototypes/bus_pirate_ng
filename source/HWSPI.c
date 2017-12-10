
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
	cdcprintf("HWSPI: start()");

	if(csidle) spi_set_nss_low(BPSPIPORT);
		else spi_set_nss_high(BPSPIPORT);
}
void HWSPI_startr(void)
{
	cdcprintf("HWSPI: startr()");

	if(csidle) spi_set_nss_low(BPSPIPORT);
		else spi_set_nss_high(BPSPIPORT);
}
void HWSPI_stop(void)
{
	cdcprintf("HWSPI: stop()");

	if(csidle) spi_set_nss_high(BPSPIPORT);
		else spi_set_nss_low(BPSPIPORT);
}
void HWSPI_stopr(void)
{
	cdcprintf("HWSPI: stopr()");

	if(csidle) spi_set_nss_high(BPSPIPORT);
		else spi_set_nss_low(BPSPIPORT);

}
uint32_t HWSPI_send(uint32_t d)
{
	uint16_t returnval;

	//TODO: check numbits, lsb
	
	returnval=spi_xfer(BPSPIPORT, (uint16_t)d);

	cdcprintf("HWSPI: send(%08X)=%08X", d, returnval);

	return (uint16_t) returnval;
}
uint32_t HWSPI_read(void)
{
	uint16_t returnval;

	//TODO: check numbits, lsb


	returnval = spi_read(BPSPIPORT);
	cdcprintf("HWSPI: read()=%08X", returnval);

	return (uint16_t) returnval;
}
void HWSPI_clkh(void)
{
	cdcprintf("HWSPI: clkh()");
}
void HWSPI_clkl(void)
{
	cdcprintf("HWSPI: clkl()");
}
void HWSPI_dath(void)
{
	cdcprintf("HWSPI: dath()");
}
void HWSPI_datl(void)
{
	cdcprintf("HWSPI: datl()");
}
uint32_t HWSPI_dats(void)
{
	uint32_t returnval;
	returnval=0;
	cdcprintf("HWSPI: dats()=%08X", returnval);
	return returnval;
}
void HWSPI_clk(void)
{
	cdcprintf("HWSPI: clk()");
}
uint32_t HWSPI_bitr(void)
{
	uint32_t returnval;
	returnval=0;
	cdcprintf("HWSPI: bitr()=%08X", returnval);
	return returnval;
}
uint32_t HWSPI_period(void)
{
	uint32_t returnval;
	returnval=0;
	cdcprintf("HWSPI: period()=%08X", returnval);
	return returnval;
}
void HWSPI_macro(uint32_t macro)
{
	cdcprintf("HWSPI: macro(%08X)", macro);
}
void HWSPI_setup(void)
{
	cdcprintf("HWSPI: setup()");

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
	cdcprintf("HWSPI: setup_exc()");

	// assuming BPSPIPORT for now until HW is finished
	// start the clock
	rcc_periph_clock_enable(BPSPICLK);

	// setup gpio asalternate function
	gpio_set_mode(BPSPIMOSIPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPSPIMOSIPIN);
	gpio_set_mode(BPSPICSPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPSPICSPIN);
	gpio_set_mode(BPSPICLKPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPSPICLKPIN);
	gpio_set_mode(BPSPIMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPIMISOPIN);

	// reset all registers
	spi_reset(BPSPIPORT);

	// init BPSPIPORT
	spi_init_master(BPSPIPORT, br, cpol, cpha, dff, lsbfirst);

	// we use software control of /cs
	spi_enable_software_slave_management(BPSPIPORT);

	// cs=1 
	if(csidle) spi_set_nss_high(BPSPIPORT);
		else spi_set_nss_low(BPSPIPORT);
	
	// unleash the beast
	spi_enable(BPSPIPORT);

	// update modeConfig pins
	modeConfig.misoport=BPSPIMISOPORT;
	modeConfig.mosiport=BPSPIMISOPORT;
	modeConfig.csport=BPSPICSPORT;
	modeConfig.clkport=BPSPICLKPORT;
	modeConfig.misopin=BPSPIMISOPIN;
	modeConfig.mosipin=BPSPIMOSIPIN;
	modeConfig.cspin=BPSPICSPIN;
	modeConfig.clkpin=BPSPICLKPIN;

}
void HWSPI_cleanup(void)
{
	cdcprintf("HWSPI: cleanup()");

	// disable SPI peripheral
	spi_disable(BPSPIPORT);		// spi_clean_disable??

	// set all used pins to input
	gpio_set_mode(BPSPIMISOPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPIMISOPIN);
	gpio_set_mode(BPSPIMOSIPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPIMOSIPIN);
	gpio_set_mode(BPSPICSPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPICSPIN);
	gpio_set_mode(BPSPICLKPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPSPICLKPIN);

	// disable clock to save the planet warming up
	rcc_periph_clock_enable(BPSPICLK);

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

