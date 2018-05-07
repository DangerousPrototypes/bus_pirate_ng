#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "buspirateNG.h"
#include "LA.h"
#include "cdcacm.h"
#include "UI.h"
#include "sniffer.h"

static volatile uint8_t armed;
static volatile uint8_t csidle;
static uint8_t miso, mosi, clock;




void sniffSPI(uint8_t cpol, uint8_t cpha, uint8_t cs)
{
	if(cpol==0)
	{
		if(cpha==0)
			exti_set_trigger(EXTI13, EXTI_TRIGGER_RISING);
		else
			exti_set_trigger(EXTI13, EXTI_TRIGGER_FALLING);
	}
	else
	{
		if(cpha==0)
			exti_set_trigger(EXTI13, EXTI_TRIGGER_FALLING);
		else
			exti_set_trigger(EXTI13, EXTI_TRIGGER_RISING);
	}


	if(cs)
	{
		exti_set_trigger(EXTI12, EXTI_TRIGGER_FALLING);
	}
	else
	{
		exti_set_trigger(EXTI12, EXTI_TRIGGER_RISING);
	}

	csidle=cs;
	miso=0;
	mosi=0;
	armed=0;

	exti_select_source(EXTI12, GPIOB);
	exti_select_source(EXTI13, GPIOB);
	exti_enable_request(EXTI12);
	exti_enable_request(EXTI13);
	nvic_set_priority(NVIC_EXTI15_10_IRQ, 0);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);

	cdcgetc();

	nvic_disable_irq(NVIC_EXTI15_10_IRQ);
	exti_disable_request(EXTI12);
	exti_disable_request(EXTI13);
}


//
void exti15_10_isr(void)
{
	if(exti_get_flag_status(EXTI10))		// not used
	{
		exti_reset_request(EXTI10);
	}
	if(exti_get_flag_status(EXTI11))		// not used
	{
		exti_reset_request(EXTI11);
	}
	if(exti_get_flag_status(EXTI12))		// CS
	{
		if(armed)
		{
			if(csidle)
				exti_set_trigger(EXTI12, EXTI_TRIGGER_FALLING);
			else
				exti_set_trigger(EXTI12, EXTI_TRIGGER_RISING);
			armed=0;
			cdcputc(']');
		}
		else
		{
			if(csidle)
				exti_set_trigger(EXTI12, EXTI_TRIGGER_RISING);
			else
				exti_set_trigger(EXTI12, EXTI_TRIGGER_FALLING);
			armed=1;
			cdcputc('[');
		}
		cdcputc(' ');
		exti_reset_request(EXTI12);
	}
	if(exti_get_flag_status(EXTI13))		// CLOCK/SCL
	{
		if(armed)
		{
			miso<<=1;
			mosi<<=1;
			if(gpio_get(GPIOB, GPIO14))
				miso|=1;
			if(gpio_get(GPIOB, GPIO15))
				mosi|=1;
			clock++;
			if(clock==8)
			{
				clock=0;
				cdcprintf(">0x%02X <0x%02X ", mosi, miso);
				miso=0;
				mosi=0;
			}
		}
		exti_reset_request(EXTI13);
	}
	if(exti_get_flag_status(EXTI14))		// MISO/-
	{
		exti_reset_request(EXTI14);
	}
	if(exti_get_flag_status(EXTI15))		// MOSI/SDA
	{
		exti_reset_request(EXTI15);
	}
}
