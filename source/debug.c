
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include "debug.h"

void debuginit(void)
{
	rcc_periph_clock_enable(RCC_USART1);

	/* Setup GPIO pin GPIO_USART1_TX. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_mode(USART1, USART_MODE_TX);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART1);

}

void dputc(char c)
{
	usart_send_blocking(USART1, c);
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

