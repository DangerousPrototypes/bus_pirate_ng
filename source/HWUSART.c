

#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "buspirateNG.h"
#include "cdcacm.h"
#include "HWUSART.h"
#include "UI.h"

static uint32_t returnval;

static uint32_t br, nbits, sbits, parity, block;


void HWUSART_start(void)
{
	cdcprintf("HWUSART start()");
}
void HWUSART_startr(void)
{
	cdcprintf("HWUSART startr()");
}
void HWUSART_stop(void)
{
	cdcprintf("HWUSART stop()");
}
void HWUSART_stopr(void)
{
	cdcprintf("HWUSART stopr()");
}
uint32_t HWUSART_send(uint32_t d)
{
	if(block)
		usart_send_blocking(BPUSART, d);
	else
		usart_send(BPUSART, d);

	HWUSART_printerror();

	return 0;
}
uint32_t HWUSART_read(void)
{
	uint32_t received;

	if(block)
		received=usart_recv_blocking(BPUSART);
	else
		received=usart_recv(BPUSART);

	HWUSART_printerror();

	return received;
}
void HWUSART_clkh(void)
{
	cdcprintf("HWUSART clkh()");
}
void HWUSART_clkl(void)
{
	cdcprintf("HWUSART clkl()");
}
void HWUSART_dath(void)
{
	cdcprintf("HWUSART dath()");
}
void HWUSART_datl(void)
{
	cdcprintf("HWUSART datl()");
}
uint32_t HWUSART_dats(void)
{
	cdcprintf("HWUSART dats()=%08X", returnval);
	return returnval;
}
void HWUSART_clk(void)
{
	cdcprintf("HWUSART clk()");
}
uint32_t HWUSART_bitr(void)
{
	cdcprintf("HWUSART bitr()=%08X", returnval);
	return returnval;
}
uint32_t HWUSART_period(void)
{
	cdcprintf("HWUSART period()=%08X", returnval);
	return returnval;
}
void HWUSART_macro(uint32_t macro)
{
	cdcprintf("HWUSART macro(%08X)", macro);
}



void HWUSART_setup(void)
{
	// did the user leave us arguments?
	// baudrate
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	br=getint();
	if(br==0) 			// any value is ok for us, but for the other side? :)
		modeConfig.error=1;

	// parity
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	parity=getint()-1;
	if(parity<=2)
	{
		switch (parity)
		{
			case 0:	parity=USART_PARITY_NONE;
				break;
			case 1:	parity=USART_PARITY_EVEN;
				break;
			case 2:	parity=USART_PARITY_ODD;
				break;
		}
	}
		else modeConfig.error=1;

	// numbits
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	nbits=getint();
	if(!((nbits==8)||(nbits==9)))
		modeConfig.error=1;

	//stopbits
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	sbits=getint()-1;
	if(sbits<=4)
		sbits<<=12;
	else
		modeConfig.error=1;

	// block
	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	consumewhitechars();
	block=getint();
	if(block>=1)
		block-=1;
	else
		modeConfig.error=1;

	// did the user did it right?
	if(modeConfig.error)			// go interactive 
	{

		br=(askint(UARTSPEEDMENU, 1, 115200, 115200));
		parity=(askint(UARTPARITYMENU, 1, 3, 1));

		switch (parity)
		{
			case 0:	parity=USART_PARITY_NONE;
				break;
			case 1:	parity=USART_PARITY_EVEN;
				break;
			case 2:	parity=USART_PARITY_ODD;
				break;
		}
		nbits=(askint(UARTNUMBITSMENU, 8, 9, 8));
		sbits=((askint(UARTSTOPBITSMENU, 1, 4, 1)-1)<<12);
		block=(askint(UARTBLOCKINGMENU, 1, 2, 2)-1);
	}

}



void HWUSART_setup_exc(void)
{
	// enable clock
	rcc_periph_clock_enable(BPUSARTCLK);

	// set gpio
	gpio_set_mode(BPUSARTTXPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BPUSARTTXPIN);
	gpio_set_mode(BPUSARTRXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BPUSARTRXPIN);

	// setup USART
	usart_set_baudrate(BPUSART, br);
	usart_set_databits(BPUSART, nbits);
	usart_set_stopbits(BPUSART, sbits);
	usart_set_parity(BPUSART, parity);

	// standard
	usart_set_mode(BPUSART, USART_MODE_TX_RX);
	usart_set_flow_control(BPUSART, USART_FLOWCONTROL_NONE);

	// enable USART
	usart_enable(BPUSART);

	// update modeConfig pins
	modeConfig.misoport=BPUSARTRXPORT;
	modeConfig.mosiport=BPUSARTTXPORT;
	modeConfig.misopin=BPUSARTRXPIN;
	modeConfig.mosipin=BPUSARTTXPIN;

}



void HWUSART_cleanup(void)
{
	cdcprintf("HWUSART cleanup()");

	//disable usart
	usart_enable(BPUSART);

	//disable clock
	rcc_periph_clock_disable(BPUSARTCLK);

	// set pins to HiZ
	gpio_set_mode(BPUSARTTXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPUSARTTXPIN);
	gpio_set_mode(BPUSARTRXPORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BPUSARTRXPIN);

	// update modeConfig pins
	modeConfig.misoport=0;
	modeConfig.mosiport=0;
	modeConfig.misopin=0;
	modeConfig.mosipin=0;

}
void HWUSART_pins(void)
{
	cdcprintf("-\tRXD\t-\tTXD");
}
void HWUSART_settings(void)
{
	uint32_t par=0;

	switch(parity)
	{
		case 	USART_PARITY_NONE: par=1;
			break;
		case 	USART_PARITY_EVEN: par=2;
			break;
		case 	USART_PARITY_ODD: par=3;
			break;
	}
	cdcprintf("HWUSART (br parity numbits stopbits block)=(%d %d %d %d %d)", br, par, nbits, ((sbits>>12)+1), (block+1));
}

void HWUSART_printerror(void)
{
	uint32_t error;

	error=USART_SR(BPUSART)&USARTERRORS;	// not all are errors

	if(error)
	{
		cdcprintf("flags: ");
		if(error&USART_SR_PE)		// parity error
			cdcprintf("PE ");
		if(error&USART_SR_FE)		// framing error
			cdcprintf("FE ");
		if(error&USART_SR_NE)		// noise error
			cdcprintf("NE ");
		if(error&USART_SR_ORE)		// overrun
			cdcprintf("ORE ");
		if(error&USART_SR_IDLE)		// idle 
			cdcprintf("IDLE ");
		if(error&USART_SR_RXNE)		// RX register not empty
			cdcprintf("RXNE ");
		if(error&USART_SR_TC)		// transmission complete
			cdcprintf("TC ");
		if(error&USART_SR_TXE)		// TX buff empty
			cdcprintf("TXE ");
		if(error&USART_SR_LBD)		// LIN break 
			cdcprintf("LBD ");
		if(error&USART_SR_CTS)		// CTS set
			cdcprintf("CTS ");
		USART_SR(BPUSART)=error;	// clear error

	}
}










