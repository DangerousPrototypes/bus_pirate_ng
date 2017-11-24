#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "debug.h"
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"


//globals
uint32_t usbpolltime;				// usb poll timer

void sys_tick_handler(void)
{
	usbpolltime++;


	// check usb for new data
	if (usbpolltime==5)
	{
		cdcpoll();
		usbpolltime=0;
	}
}



int main(void)
{

	// init vars
	usbpolltime=0;

	// initialize the cmdbuffer
	initUI();

	// init clock
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	// enable clocks
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);

	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;		// disable jtag/enable swd

	// enable debug
	debuginit();

	// setup systick 
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);	// 9000000 Hz
	systick_set_reload(89);				// 10us
	systick_interrupt_enable();
	systick_counter_enable();				// go!

	cdcinit();

	while (1)
	{
		doUI();
	}
}













