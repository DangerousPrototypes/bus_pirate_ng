#include <stdlib.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include "debug.h"
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"
#include "ADC.h"


//globals
uint32_t usbpolltime;				// usb poll timer
volatile uint32_t systicks;

// systick timer 
// handles the USB polling
void sys_tick_handler(void)
{
	usbpolltime++;
	systicks++;

	// check usb for new data
	if (usbpolltime==5)
	{
		cdcpoll();
		usbpolltime=0;
	}
}

// all the fun starts here
int main(void)
{

	// init vars
	usbpolltime=0;

	// initialize the cmdbuffer
	initUI();

	// init clock
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	// enable clocks for IO and alternate functions
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_AFIO);

	AFIO_MAPR |= AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;		// disable jtag/enable swd

	// setup pins (move to a seperate function??)
#ifdef BP_CONTROLS_PU
	gpio_set_mode(BP_USB_PULLUP_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_USB_PULLUP_PIN);	// USB d+ pullup
	gpio_clear(BP_USB_PULLUP_PORT, BP_USB_PULLUP_PIN);							// pull down
#endif

	gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);								// active hi
	gpio_set_mode(BP_PSUEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_PSUEN_PIN);		// PSU disable

	gpio_clear(BP_VPUEN_PORT, BP_VPUEN_PIN);								// active hi
	gpio_set_mode(BP_VPUEN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_VPUEN_PIN);		// VPU disable

	gpio_clear(BP_VPU50EN_PORT, BP_VPU50EN_PIN);								// active low
	gpio_set_mode(BP_VPU50EN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_VPU50EN_PIN);	// VPU3v3 disable

	gpio_clear(BP_VPU33EN_PORT, BP_VPU33EN_PIN);								// active low
	gpio_set_mode(BP_VPU33EN_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_VPU33EN_PIN);	// VPU5v0 disable

	gpio_set_mode(BP_ADC_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_ADC_PIN);				// ADC pin
	gpio_set_mode(BP_3V3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_3V3_PIN);				// ADC 3v3 regulator
	gpio_set_mode(BP_5V0_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_5V0_PIN);				// ADC 5v0 regulator
	gpio_set_mode(BP_VPU_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_VPU_PIN);				// ADC pullup voltage
	gpio_set_mode(BP_VSUP_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_VSUP_PIN);			// ADC usb powersupply
	gpio_set_mode(BP_VSUP_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_ANALOG, BP_VSUP_PIN);			// ADC vpu pin

	gpio_clear(BP_MODE_LED_PORT, BP_MODE_LED_PIN);
	gpio_set_mode(BP_MODE_LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_MODE_LED_PIN);	// mode led
	gpio_clear(BP_USB_LED_PORT, BP_USB_LED_PIN);
	gpio_set_mode(BP_USB_LED_PORT, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_USB_LED_PIN);	// mode led


	// enable debug
	debuginit();

	// setup systick 
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB_DIV8);	// 9000000 Hz
	systick_set_reload(89);					// 10us 89
	systick_interrupt_enable();
	systick_counter_enable();				// go!
	systicks=0;

#ifdef BP_CONTROLS_PU
	// enable USB pullup
	
	delayms(100);
	gpio_set(BP_USB_PULLUP_PORT, BP_USB_PULLUP_PIN);
#else
	//toggle the usb pullup
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO12);
	gpio_clear(GPIOA, GPIO12);
	delayms(100);
#endif

	// setup USB
	cdcinit();

	//setup ADC
	initADC();

	while (1)
	{
		doUI();
	}
}













