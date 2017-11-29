
#include "buspirateNG.h"
#include <stdint.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "HWSPI.h"
#include "cdcacm.h"
#include "UI.h"


static uint32_t cpol, cpha, cr, dff, lsbfirst, csidle;

void HWSPI_start(void)
{
	cdcprintf("HWSPI: start()");

	if(csidle) spi_set_nss_low(SPI1);
		else spi_set_nss_high(SPI1);
}
void HWSPI_startr(void)
{
	cdcprintf("HWSPI: startr()");

	if(csidle) spi_set_nss_low(SPI1);
		else spi_set_nss_high(SPI1);
}
void HWSPI_stop(void)
{
	cdcprintf("HWSPI: stop()");

	if(csidle) spi_set_nss_high(SPI1);
		else spi_set_nss_low(SPI1);
}
void HWSPI_stopr(void)
{
	cdcprintf("HWSPI: stopr()");

	if(csidle) spi_set_nss_high(SPI1);
		else spi_set_nss_low(SPI1);

}
uint32_t HWSPI_send(uint32_t d)
{
	uint16_t returnval;
	
	returnval=spi_xfer(SPI1, (uint16_t)d);

	cdcprintf("HWSPI: send(%08X)=%08X", d, returnval);

	return (uint16_t) returnval;
}
uint32_t HWSPI_read(void)
{
	uint16_t returnval;

	returnval = spi_read(SPI1);
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
	cdcprintf("HWSPI: bitr()=%08X", returnval);
	return returnval;
}
uint32_t HWSPI_period(void)
{
	uint32_t returnval;
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

	cdcprintf("SPI Clock speed\r\n");
	cdcprintf(" 1. 16Mhz\r\n");
	cdcprintf(" 2. 8Mhz\r\n");
	cdcprintf(" 3. 4Mhz\r\n");
	cdcprintf(" 4. 2Mhz\r\n");
	cdcprintf(" 5. 1Mhz*\r\n");
	cdcprintf(" 6. 500Khz\r\n");
	cdcprintf(" 7. 250Khz\r\n");
//	cr= choice&0x07)<<3;
	cr=5<<3;	//dvi64

	cdcprintf("Clock polarity\r\n");
	cdcprintf(" 1. idle low\r\n");
	cdcprintf(" 2. idle high*\r\n");
	cpol=SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE;

	cdcprintf("Clock phase\r\n");
	cdcprintf(" 1. leading edge\r\n");
	cdcprintf(" 2. trailing edge*\r\n");
	cpha=SPI_CR1_CPHA_CLK_TRANSITION_2;
	
	// 8bit and lsb/msb handled in UI.c
	dff=SPI_CR1_DFF_8BIT;
	lsbfirst=SPI_CR1_MSBFIRST;

	cdcprintf("CS mode\r\n");
	cdcprintf(" 1. CS\r\n");
	cdcprintf(" 2. /CS*\r\n");
	csidle=1;
}
void HWSPI_setup_exc(void)
{
	cdcprintf("HWSPI: setup_exc()");

	// assuming SPI1 for now until HW is finished
	// start the clock
	rcc_periph_clock_enable(RCC_SPI1);

	// setup gpio asalternate function
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO4|GPIO5|GPIO7 );
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,GPIO6);

	// reset all registers
	spi_reset(SPI1);

	// init SPI1
	spi_init_master(SPI1, cr, cpol, cpha, dff, lsbfirst);

	// we use software control of /cs
	spi_enable_software_slave_management(SPI1);

	// cs=1 
	if(csidle) spi_set_nss_high(SPI1);
		else spi_set_nss_low(SPI1);
	
	// unleash the beast
	spi_enable(SPI1);

}
void HWSPI_cleanup(void)
{
	cdcprintf("HWSPI: cleanup()");

	// disable SPI peripheral
	spi_disable(SPI1);		// spi_clean_disable??

	// set all used pins to input
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,GPIO6);

	// disable clock to save the planet warming up
	rcc_periph_clock_enable(RCC_SPI1);
}
void HWSPI_pins(void)
{
	cdcprintf("pin1\tpin2\tpin3\tpin4");
}
void HWSPI_settings(void)
{
	cdcprintf("DUMMY (arg1 arg2)=(%d, %d)", 1, 2);
}

