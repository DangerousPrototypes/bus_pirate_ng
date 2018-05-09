#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "buspirateNG.h"
#include "LA.h"
#include "cdcacm.h"
#include "UI.h"
#include "sniffer.h"

static volatile uint8_t armed, state;
static volatile uint8_t csidle;
static uint8_t miso, mosi, clock, data;


enum {
	WAITFORSTART,
	GET8BITS,
	GETACK,
};


void (*sniffisr)(void);

void sniffI2C(void)
{
	exti_set_trigger(BP_EXTI_CLK, EXTI_TRIGGER_RISING);
	exti_set_trigger(BP_EXTI_MOSI, EXTI_TRIGGER_BOTH);
	
	data=0;
	clock=0;
	state=WAITFORSTART;
	sniffisr=exti15_10_isr_i2c;

	exti_select_source(BP_EXTI_CLK, BP_EXTI_PORT);
	exti_select_source(BP_EXTI_MOSI, BP_EXTI_PORT);
	exti_enable_request(BP_EXTI_CLK);
	exti_enable_request(BP_EXTI_MOSI);
//	nvic_set_priority(NVIC_BP_EXTI_MOSI_10_IRQ, 0);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);

	cdcgetc();

	nvic_disable_irq(NVIC_EXTI15_10_IRQ);
	exti_disable_request(BP_EXTI_CLK);
	exti_disable_request(BP_EXTI_MOSI);
}

void sniffSPI(uint8_t cpol, uint8_t cpha, uint8_t cs)
{
	if(cpol==0)
	{
		if(cpha==0)
			exti_set_trigger(BP_EXTI_CLK, EXTI_TRIGGER_RISING);
		else
			exti_set_trigger(BP_EXTI_CLK, EXTI_TRIGGER_FALLING);
	}
	else
	{
		if(cpha==0)
			exti_set_trigger(BP_EXTI_CLK, EXTI_TRIGGER_FALLING);
		else
			exti_set_trigger(BP_EXTI_CLK, EXTI_TRIGGER_RISING);
	}


	if(cs)
	{
		exti_set_trigger(BP_EXTI_CS, EXTI_TRIGGER_FALLING);
	}
	else
	{
		exti_set_trigger(BP_EXTI_CS, EXTI_TRIGGER_RISING);
	}

	csidle=cs;
	miso=0;
	mosi=0;
	armed=0;

	sniffisr=exti15_10_isr_spi;

	exti_select_source(BP_EXTI_CS, BP_EXTI_PORT);
	exti_select_source(BP_EXTI_CLK, BP_EXTI_PORT);
	exti_enable_request(BP_EXTI_CS);
	exti_enable_request(BP_EXTI_CLK);
//	nvic_set_priority(NVIC_EXTI15_10_IRQ, 0);
	nvic_enable_irq(NVIC_EXTI15_10_IRQ);

	cdcgetc();

	nvic_disable_irq(NVIC_EXTI15_10_IRQ);
	exti_disable_request(BP_EXTI_CS);
	exti_disable_request(BP_EXTI_CLK);
}


//
void exti15_10_isr(void)
{
	sniffisr();
}

// SPi isr
void exti15_10_isr_spi(void)
{
	if(exti_get_flag_status(EXTI10))		// not used
	{
		exti_reset_request(EXTI10);
	}
	if(exti_get_flag_status(EXTI11))		// not used
	{
		exti_reset_request(EXTI11);
	}
	if(exti_get_flag_status(BP_EXTI_CS))		// CS
	{
		if(armed)
		{
			if(csidle)
				exti_set_trigger(BP_EXTI_CS, EXTI_TRIGGER_FALLING);
			else
				exti_set_trigger(BP_EXTI_CS, EXTI_TRIGGER_RISING);
			armed=0;
			cdcputc(']');
		}
		else
		{
			if(csidle)
				exti_set_trigger(BP_EXTI_CS, EXTI_TRIGGER_RISING);
			else
				exti_set_trigger(BP_EXTI_CS, EXTI_TRIGGER_FALLING);
			armed=1;
			cdcputc('[');
		}
		cdcputc(' ');
		exti_reset_request(BP_EXTI_CS);
	}
	if(exti_get_flag_status(BP_EXTI_CLK))		// CLOCK/SCL
	{
		if(armed)
		{
			miso<<=1;
			mosi<<=1;
			if(gpio_get(BP_SNIFF_PORT, BP_SNIFF_MISO))
				miso|=1;
			if(gpio_get(BP_SNIFF_PORT, BP_SNIFF_MOSI))
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
		exti_reset_request(BP_EXTI_CLK);
	}
	if(exti_get_flag_status(BP_EXTI_MISO))		// MISO/-
	{
		exti_reset_request(BP_EXTI_MISO);
	}
	if(exti_get_flag_status(BP_EXTI_MOSI))		// MOSI/SDA
	{
		exti_reset_request(BP_EXTI_MOSI);
	}
}



// i2c isr
void exti15_10_isr_i2c(void)
{
	if(exti_get_flag_status(EXTI10))		// not used
	{
		exti_reset_request(EXTI10);
	}
	if(exti_get_flag_status(EXTI11))		// not used
	{
		exti_reset_request(EXTI11);
	}
	if(exti_get_flag_status(BP_EXTI_CS))		// CS
	{
		exti_reset_request(BP_EXTI_CS);
	}
	if(exti_get_flag_status(BP_EXTI_CLK))		// CLOCK/SCL
	{
		switch(state)
		{
			case GETACK:
				if(gpio_get(BP_SNIFF_PORT, BP_SNIFF_MOSI)) cdcputc('.');
					else cdcputc('A');
				state=GET8BITS;
				break;
			case GET8BITS:
				data<<=1;
				if(gpio_get(BP_SNIFF_PORT, BP_SNIFF_MOSI)) data|=1;
				clock++;
				if(clock==8)
				{
					state=GETACK;
					clock=0;
					cdcprintf("0x%02X", data);
				}
			default:
				break;
		}
		exti_reset_request(BP_EXTI_CLK);
	}
	if(exti_get_flag_status(BP_EXTI_MISO))		// MISO/-
	{
		exti_reset_request(BP_EXTI_MISO);
	}
	if(exti_get_flag_status(BP_EXTI_MOSI))		// MOSI/SDA
	{
		if((state!=GETACK))	//&&(clock==1))	//	(!((state==GETACK)||(clock!=0)))
		{
			if(gpio_get(BP_SNIFF_PORT, BP_SNIFF_CLK))
			{
				if(gpio_get(BP_SNIFF_PORT, BP_SNIFF_MOSI))
				{
					cdcputc(']');
					state=WAITFORSTART;
					clock=0;
				}
				else
				{
					cdcputc('[');
					state=GET8BITS;
					clock=0;
				}
			}
		}
		exti_reset_request(BP_EXTI_MOSI);
	}
}
