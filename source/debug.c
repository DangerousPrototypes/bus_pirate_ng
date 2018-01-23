
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "buspirateNG.h"
#include "debug.h"

// inits the debug system
// outputs to the uart
// 115200,n,8,1
void debuginit(void)
{
	// enable clock
	rcc_periph_clock_enable(BP_DEBUG_USART_CLK);

	// set gpio
	gpio_set_mode(BP_DEBUG_TX_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_DEBUG_TX_PIN);

	// setup USART
	usart_set_baudrate(BP_DEBUG_USART, 115200);
	usart_set_databits(BP_DEBUG_USART, 8);
	usart_set_stopbits(BP_DEBUG_USART, USART_STOPBITS_1);
	usart_set_mode(BP_DEBUG_USART, USART_MODE_TX);
	usart_set_parity(BP_DEBUG_USART, USART_PARITY_NONE);
	usart_set_flow_control(BP_DEBUG_USART, USART_FLOWCONTROL_NONE);

	// enable USART
	usart_enable(BP_DEBUG_USART);

}

// outputs single char
void dputc(char c)
{
	usart_send_blocking(BP_DEBUG_USART, c);
}

// outust a string
void dputs(char *s)
{
	while(*s) dputc(*(s++));
}

static char  buf[BP_DEBUG_BUFLEN];

// outputs a formatted string
void dprintf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, BP_DEBUG_BUFLEN, fmt, args);
	dputs(buf);
	va_end(args);
}

