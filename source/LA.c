

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "buspirateNG.h"
#include "LA.h"
#include "cdcacm.h"
#include "UI.h"

#define CMDREAD		0x03
#define CMDWRITE	0x02
#define CMDQUADMODE	0x38
#define CMDRESETSPI	0xFF


static volatile uint8_t stop;
static volatile uint32_t counts;
static uint32_t returnval;
static uint8_t lamode;
static uint16_t period;
static uint32_t samples;
static uint8_t triggers[8];

static uint8_t labuff[BP_LA_BUFFSIZE];			// is this french?!


static void displaybuff(void);
static uint8_t spixferx1(uint8_t d);
static uint8_t spixferx4(uint8_t d);
static void getbuff(void);
static void setup_spix1rw(void);
static void setup_spix4w(void);
static void setup_spix4r(void);


const char spinner[]={'-', '\\', '|', '/'};
char triggermodes[][4]={
"_/\"",
"\"\\_",
"#X#",
"N/A"
};

void LA_start(void)
{
	int i;

	// send command to write sram at addr 0x0
	setup_spix4w();
	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	spixferx4(CMDWRITE);
	spixferx4(0x00);
	spixferx4(0x00);
	spixferx4(0x00);

	// turn spi bus around
	setup_spix4r();

	// setup triggers
	exti_enable_request(EXTI0);
	exti_enable_request(EXTI1);
	exti_enable_request(EXTI2);
	exti_enable_request(EXTI3);
	exti_enable_request(EXTI4);
	exti_enable_request(EXTI5);
	exti_enable_request(EXTI6);
	exti_enable_request(EXTI7);


	// show the current settings
	cdcprintf("Sampling clock=%dHz, #samples: %dK\r\n", (36000000/period), samples/1024);
	cdcprintf("CHAN1 trigger: %s\r\n", triggermodes[triggers[0]]);
	cdcprintf("CHAN2 trigger: %s\r\n", triggermodes[triggers[1]]);
	cdcprintf("CHAN3 trigger: %s\r\n", triggermodes[triggers[2]]);
	cdcprintf("CHAN4 trigger: %s\r\n", triggermodes[triggers[3]]);
	cdcprintf("CHAN5 trigger: %s\r\n", triggermodes[triggers[4]]);
	cdcprintf("CHAN6 trigger: %s\r\n", triggermodes[triggers[5]]);
	cdcprintf("CHAN7 trigger: %s\r\n", triggermodes[triggers[6]]);
	cdcprintf("CHAN8 trigger: %s\r\n", triggermodes[triggers[7]]);
	cdcprintf("\r\npress any key to exit\r\n\r\n");

	// timer controls clock line
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_LA_SRAM_CLK_PIN); // CLK

	// timer
	timer_set_oc_value(BP_LA_TIMER, BP_LA_TIM_CHAN, (period/2));							// set match value
	timer_set_period(BP_LA_TIMER, period);									// set period 
	counts=0;
	timer_enable_counter(BP_LA_TIMER);									// enable the timer

	// go!
	gpio_clear(BP_LA_LATCH_PORT, BP_LA_LATCH_PIN);
	stop=0;

	i=0;
	while(!stop)
	{
		cdcprintf("\r%c Waiting for trigger %c", spinner[i],	 spinner[i]);
		i++;
		i&=0x03;
		delayms(100);

		if(cdcbyteready()) stop=11;		//user interrupt
	}

	// disable clk
	timer_disable_counter(BP_LA_TIMER);									// enable the timer

	// disable latch
	gpio_set(BP_LA_LATCH_PORT, BP_LA_LATCH_PIN);

	// reease cs
	gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	
	// disable triggers
	exti_disable_request(EXTI0);
	exti_disable_request(EXTI1);
	exti_disable_request(EXTI2);
	exti_disable_request(EXTI3);
	exti_disable_request(EXTI4);
	exti_disable_request(EXTI5);
	exti_disable_request(EXTI6);
	exti_disable_request(EXTI7);

	if(stop<10)
		cdcprintf("\r\nCapture stopped on trigger chan %d after %d samples", stop, counts);
	else if(stop==10)
		cdcprintf("\r\nCapture stopped max. samples (%d)", counts);
	else if(stop==11)
		cdcprintf("\r\nCapture interrupted by user at %d samples", counts);
	else cdcprintf("\r\nCapture stopped for unknown reasons at %d samples", counts);
}

uint32_t LA_send(uint32_t d)
{
	cdcprintf("-logic analyzer send(%08X)=%08X", d, returnval);

	returnval=d;

	return d;
}

uint32_t LA_read(void)
{
	cdcprintf("logic analyzer read()=%08X", returnval);
	return returnval;
}

void LA_macro(uint32_t macro)
{
	int i;

	switch(macro)
	{
		case 0:		cdcprintf("1. fill buffer with random data\r\n2. display buffer in ASCII art");
				break;
		case 1:		for(i=0; i<BP_LA_BUFFSIZE; i++)
				{
					labuff[i]=i;
				}
				break;
		case 2:		displaybuff();
				break;
		case 3:		getbuff();
				break;
		default:	modeConfig.error=1;
				cdcprintf("no such macro");
				break;
	}
}
void LA_setup(void)
{
	cdcprintf("Please use macro system for setting and viewing parameters");
}
void LA_setup_exc(void)
{
	int i;

	// setup GPIOs
	gpio_set_mode(BP_LA_LATCH_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_LATCH_PIN); 	// 373 latch
	gpio_set_mode(BP_LA_SRAM_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CS_PIN);	// CS
	gpio_set(BP_LA_LATCH_PORT, BP_LA_LATCH_PIN);

	// softspi setup sram chip into quad mode
	setup_spix1rw();
	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	spixferx1(CMDQUADMODE);
	gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);

	// program timer as much as possible
	rcc_periph_clock_enable(BP_LA_TIM_CLOCK);								// enable clock
	timer_reset(BP_LA_TIMER);										// reset peripheral
	timer_set_mode(BP_LA_TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);			// count up
	timer_set_oc_mode(BP_LA_TIMER, BP_LA_TIM_CHAN, TIM_OCM_PWM1);						// PWM1 == high/low; PWM2= low/high
	timer_enable_oc_output(BP_LA_TIMER, BP_LA_TIM_CHAN);							// output channel
	timer_enable_break_main_output(BP_LA_TIMER);								// need to set break
	timer_enable_irq(BP_LA_TIMER, TIM_DIER_CC2IE);
	nvic_enable_irq(BP_LA_TIM_NVIC);									// enable timer irq

	// setup triggers
	exti_select_source(EXTI0, GPIOC);
	exti_select_source(EXTI1, GPIOC);
	exti_select_source(EXTI2, GPIOA);
	exti_select_source(EXTI3, GPIOA);
	exti_select_source(EXTI4, GPIOC);
	exti_select_source(EXTI5, GPIOC);
	exti_select_source(EXTI6, GPIOA);
	exti_select_source(EXTI7, GPIOA);

	// defaults
	period=4500;
	samples=4096;
	for(i=0; i<8; i++) triggers[i]=3; 	// no triggers

}
void LA_cleanup(void)
{
	// back to regular spi
	spixferx4(CMDRESETSPI);

	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN1_PIN);		// MOSI
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN2_PIN);		// MISO
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_SRAM_CS_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_SRAM_CS_PIN);		// CS
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_SRAM_CLK_PIN);		// CLK
	gpio_set_mode(BP_LA_LATCH_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_LATCH_PIN); 		// 373 latch

	// timer
	rcc_periph_clock_disable(BP_LA_TIM_CLOCK);					// turn peripheral off
	timer_disable_counter(BP_LA_TIMER);
	nvic_disable_irq(BP_LA_TIM_NVIC);

	// triggers
	exti_disable_request(EXTI0);
	exti_disable_request(EXTI1);
	exti_disable_request(EXTI2);
	exti_disable_request(EXTI3);
	exti_disable_request(EXTI4);
	exti_disable_request(EXTI5);
	exti_disable_request(EXTI6);
	exti_disable_request(EXTI7);

}
void LA_pins(void)
{
	cdcprintf("CH2\tCH0\tCH1\tCH4");
}
void LA_settings(void)
{
	cdcprintf("LA ()=()");
}



void displaybuff(void)
{
	int i, j, stop;
	uint8_t mask;
	uint32_t offset;

	stop=0;
	offset=0;

	while(!stop)
	{
		cdcprintf("\x1B[2J\x1B[H");

		for(j=0; j<8; j++)
		{
			mask=0x01<<j;
			cdcprintf("\x1B[0;9%dmCH%d:\t", j, j);

			for(i=0; i<70; i++)
			{
				if(labuff[offset+i]&mask)
					cdcprintf("\x1B[0;10%dm \x1B[0;9%dm", j, j);
				else
					cdcprintf("_");
			}
			cdcprintf("\r\n");
		}
		cdcprintf("Offset=%d\r\n", offset);
		cdcprintf(" -: previous +: next x:quit\r\n");
		delayms(100);
		switch(cdcgetc())
		{
			case '+':	offset+=70;
					break;
			case '-':	offset-=70;
					break;
			case 'x':	stop=1;
					break;
		}
		
	}
}


void setup_spix1rw(void)
{
	// channnels/SPI
	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN1_PIN);	// MOSI
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN2_PIN);		// MISO
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CLK_PIN); // CLK
}


void setup_spix4w(void)
{
	// set SIO pins to output to enable quadspi
	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN1_PIN);
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN2_PIN);
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CLK_PIN);
}

void setup_spix4r(void)
{
	// set SIO pins to input
	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN1_PIN);
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN2_PIN);
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CLK_PIN);
}

static uint8_t spixferx1(uint8_t d)
{
	int i;
	uint8_t received, mask;

	gpio_clear(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN);
	mask=0x80;

	for(i=0; i<8; i++)
	{
		if(d&mask) gpio_set(BP_LA_CHAN1_PORT, BP_LA_CHAN1_PIN);
			else gpio_clear(BP_LA_CHAN1_PORT, BP_LA_CHAN1_PIN);

		delayms(1);
		received|=(gpio_get(BP_LA_CHAN2_PORT, BP_LA_CHAN2_PIN)?1:0);
		gpio_set(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN);
		delayms(1);
		gpio_clear(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN);
		mask>>=1;
		received<<=1;
	}

	return received;
}

static uint8_t spixferx4(uint8_t d)
{
	int i;
	uint8_t received, mask;

	gpio_clear(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN);
	mask=0x80;
	received=0;

	for(i=0; i<2; i++)
	{
		if(d&mask) gpio_set(BP_LA_CHAN4_PORT, BP_LA_CHAN4_PIN);
			else gpio_clear(BP_LA_CHAN4_PORT, BP_LA_CHAN4_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_LA_CHAN3_PORT, BP_LA_CHAN3_PIN);
			else gpio_clear(BP_LA_CHAN3_PORT, BP_LA_CHAN3_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_LA_CHAN2_PORT, BP_LA_CHAN2_PIN);
			else gpio_clear(BP_LA_CHAN2_PORT, BP_LA_CHAN2_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_LA_CHAN1_PORT, BP_LA_CHAN1_PIN);
			else gpio_clear(BP_LA_CHAN1_PORT, BP_LA_CHAN1_PIN);
		mask>>=1;

		delayms(1);
		received|=(gpio_get(BP_LA_CHAN4_PORT, BP_LA_CHAN4_PIN)?1:0);
		received<<=1;
		received|=(gpio_get(BP_LA_CHAN3_PORT, BP_LA_CHAN3_PIN)?1:0);
		received<<=1;
		received|=(gpio_get(BP_LA_CHAN2_PORT, BP_LA_CHAN2_PIN)?1:0);
		received<<=1;
		received|=(gpio_get(BP_LA_CHAN1_PORT, BP_LA_CHAN1_PIN)?1:0);
		received<<=1;
		gpio_set(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN);

		delayms(1);
		gpio_clear(BP_LA_SRAM_CLK_PORT, BP_LA_SRAM_CLK_PIN);
	}

	return received;
}

static void getbuff(void)
{
	int i;
	uint8_t temp;

	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);

	setup_spix4w();

	spixferx4(CMDREAD);
	spixferx4(0x00);
	spixferx4(0x00);
	spixferx4(0x00);

	setup_spix4r();

	for(i=0; i<(BP_LA_BUFFSIZE); i+=2)
	{
		temp=spixferx4(0xff);
		labuff[i]=temp&0xF0;
		labuff[i+1]=((temp<<4)&0xF0);
		
	}

	gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);



}



// interrupt service routines
// tim supplies the clock to the sram
void tim1_cc_isr(void)
{

	if (timer_get_flag(TIM1, TIM_SR_CC2IF))					// did we overflow?
	{
		counts++;
		timer_clear_flag(TIM1, TIM_SR_CC2IF);

		if(counts==samples)						// smaples enough
		{
			stop=10;
			timer_disable_counter(BP_LA_TIMER);			// enable the timer
		}	
	}
}


#if(0)
void exti4_isr(void)
{
	stop=1;
	exti_reset_request(EXTI4);
}

void exti5_isr(void)
{
	stop=1;
	exti_reset_request(EXTI5);
}

void exti6_isr(void)
{
	stop=1;
	exti_reset_request(EXTI6);
}

void exti7_isr(void)
{
	stop=1;
	exti_reset_request(EXTI7);
}
#endif

