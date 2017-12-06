
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "buspirateNG.h"
#include "debug.h"

void debuginit(void)
{
	// enable clock
	rcc_periph_clock_enable(DEBUGUSARTCLK);

	// set gpio
	gpio_set_mode(DEBUGTXPORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, DEBUGTXPIN);

	// setup USART
	usart_set_baudrate(DEBUGUSART, 115200);
	usart_set_databits(DEBUGUSART, 8);
	usart_set_stopbits(DEBUGUSART, USART_STOPBITS_1);
	usart_set_mode(DEBUGUSART, USART_MODE_TX);
	usart_set_parity(DEBUGUSART, USART_PARITY_NONE);
	usart_set_flow_control(DEBUGUSART, USART_FLOWCONTROL_NONE);

	// enable USART
	usart_enable(DEBUGUSART);

}

void dputc(char c)
{
	usart_send_blocking(DEBUGUSART, c);
}

void dputs(char *s)
{
	while(*s) dputc(*(s++));
}

#define DEBUGBUFLEN	256
static char  buf[DEBUGBUFLEN];

void dprintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, DEBUGBUFLEN, fmt, args);
	dputs(buf);
	va_end(args);
}

