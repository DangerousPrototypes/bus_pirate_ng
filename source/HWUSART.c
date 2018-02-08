

#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "buspirateNG.h"
#include "cdcacm.h"
#include "HWUSART.h"
#include "UI.h"


static uint32_t br, nbits, sbits, parity, block;

uint32_t HWUSART_send(uint32_t d)
{
	if(block)
		usart_send_blocking(BP_USART, d);
	else
		usart_send(BP_USART, d);

	HWUSART_printerror();

	return 0;
}
uint32_t HWUSART_read(void)
{
	uint32_t received;

	if(block)
		received=usart_recv_blocking(BP_USART);
	else
		received=usart_recv(BP_USART);

	HWUSART_printerror();

	return received;
}

void HWUSART_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf("No macros available");
				break;
		default:	cdcprintf("Macro not defined");
				modeConfig.error=1;
	}
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
			case 1:	parity=USART_PARITY_NONE;
				break;
			case 2:	parity=USART_PARITY_EVEN;
				break;
			case 3:	parity=USART_PARITY_ODD;
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
	rcc_periph_clock_enable(BP_USART_CLK);

	// set gpio
	gpio_set_mode(BP_USART_TX_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_USART_TX_PIN);
	gpio_set_mode(BP_USART_RX_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_USART_RX_PIN);

	// setup USART
	usart_set_baudrate(BP_USART, br);
	usart_set_databits(BP_USART, nbits);
	usart_set_stopbits(BP_USART, sbits);
	usart_set_parity(BP_USART, parity);

	// standard
	usart_set_mode(BP_USART, USART_MODE_TX_RX);
	usart_set_flow_control(BP_USART, USART_FLOWCONTROL_NONE);

	// enable USART
	usart_enable(BP_USART);

	// update modeConfig pins
	modeConfig.misoport=BP_USART_RX_PORT;
	modeConfig.mosiport=BP_USART_TX_PORT;
	modeConfig.misopin=BP_USART_RX_PIN;
	modeConfig.mosipin=BP_USART_TX_PIN;

}



void HWUSART_cleanup(void)
{
	//disable usart
	usart_enable(BP_USART);

	//disable clock
	rcc_periph_clock_disable(BP_USART_CLK);

	// set pins to HiZ
	gpio_set_mode(BP_USART_TX_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_USART_TX_PIN);
	gpio_set_mode(BP_USART_RX_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,BP_USART_RX_PIN);

	// update modeConfig pins
	modeConfig.misoport=0;
	modeConfig.mosiport=0;
	modeConfig.misopin=0;
	modeConfig.mosipin=0;

}
void HWUSART_pins(void)
{
	cdcprintf("-\t-\tRXD\tTXD");
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

	error=USART_SR(BP_USART)&USARTERRORS;	// not all are errors

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
		USART_SR(BP_USART)=error;	// clear error(s)

	}
}

void HWUSART_help(void)
{
	cdcprintf("Peer to peer asynchronous protocol.\r\n");
	cdcprintf("\r\n");

	if(parity==USART_PARITY_NONE)
	{
		cdcprintf("BPCMD\t     |                      DATA(8 bits)               |\r\n");
		cdcprintf("\tIDLE |STRT| D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |STOP|IDLE\r\n");
		cdcprintf("TXD\t\"\"\"\"\"|____|{##}|{##}|{##}|{##}|{##}|{##}|{##}|{##}|\"\"\"\"|\"\"\"\"\"\r\n");
		cdcprintf("RXD\t\"\"\"\"\"|____|{##}|{##}|{##}|{##}|{##}|{##}|{##}|{##}|\"\"\"\"|\"\"\"\"\"\r\n");
	}
	else
	{
		cdcprintf("BPCMD\t     |                      DATA(8/9 bits)                  |\r\n");
		cdcprintf("\tIDLE |STRT| D7 | D6 | D5 | D4 | D3 | D2 | D1 | D0 |PRTY|STOP|IDLE\r\n");
		cdcprintf("TXD\t\"\"\"\"\"|____|{##}|{##}|{##}|{##}|{##}|{##}|{##}|{##}|{##}|\"\"\"\"|\"\"\"\"\"\r\n");
		cdcprintf("RXD\t\"\"\"\"\"|____|{##}|{##}|{##}|{##}|{##}|{##}|{##}|{##}|{##}|\"\"\"\"|\"\"\"\"\"\r\n");
	}

	cdcprintf("\t              ^sample moment\r\n");
	cdcprintf("\r\n");
	cdcprintf("Connections:\r\n");
	cdcprintf("\tTXD\t------------------ RXD\r\n");
	cdcprintf("{BP}\tRXD\t------------------ TXD\t{DUT}\r\n");
	cdcprintf("\tGND\t------------------ GND\r\n");
}







	
