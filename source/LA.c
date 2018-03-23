

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

static void setup_spix1rw(void);
static void spiWx1(uint8_t d);

static void setup_spix4w(void);
static void spiWx4(uint8_t d);

static void setup_spix4r(void);
static uint8_t spiRx4(void);

static void displaybuff(void);
static void getbuff(void);

static volatile uint8_t stop;
static volatile uint32_t counts;
static uint32_t returnval;
static uint16_t period;
static uint32_t samples, extrasamples;
static uint8_t triggers[8];
static uint8_t labuff[BP_LA_BUFFSIZE];			// is this french?!
const char spinner[]={'-', '\\', '|', '/'};
char triggermodes[][4]={
"_/\"",
"\"\\_",
"#X#",
"N/A"
};

void logicAnalyzerSetup(void)
{
	//uint32_t i;

	BP_LA_LATCH_SETUP(); // 573 latch
	BP_LA_LATCH_CLOSE();

	//send mode reset command just in case
	setup_spix4w(); //write
	BP_LA_SRAM_SELECT();  
	delayms(1);
	spiWx4(CMDRESETSPI); //write command
	BP_LA_SRAM_DESELECT();  

	//force to sequencial mode just in case	
	setup_spix1rw();
	BP_LA_SRAM_SELECT();  
	delayms(1);
	spiWx1(CMDWRITERREG);//write register
	spiWx1(CMDSEQLMODE);
	BP_LA_SRAM_DESELECT();  
	delayms(1);
	   
	//quad mode
	BP_LA_SRAM_SELECT();  
	delayms(1);
	spiWx1(CMDQUADMODE);
	BP_LA_SRAM_DESELECT();

	//clear the sram for testing purposes
/*	setup_spix4w(); //write
	BP_LA_SRAM_SELECT();  
	delayms(1);
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
for(i=0;i<256000;i++)
	spiWx4(0xff);*/

	setup_spix4r(); //read mode
}

//begin logic capture during user commands 
void logicAnalyzerCaptureStart(void)
{
	BP_LA_LATCH_CLOSE();

	//send mode reset command just in case
	setup_spix4w(); //write
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDRESETSPI); //write command
	BP_LA_SRAM_DESELECT();  

	//quad mode
	BP_LA_SRAM_SELECT();  
	spiWx1(CMDQUADMODE);
	BP_LA_SRAM_DESELECT();

   
	//setup to capture bus activity
	setup_spix4w(); //write
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	setup_spix4r(); //read mode
	
	//open 573 latch (LOW)
	BP_LA_LATCH_OPEN();

	//setup the timer
	// timer controls clock line
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, BP_LA_SRAM_CLK_PIN); // CLK

	// timer
	timer_set_oc_value(BP_LA_TIMER, BP_LA_TIM_CHAN, (period/2));							// set match value
	timer_set_period(BP_LA_TIMER, period);									// set period 
	timer_enable_counter(BP_LA_TIMER);									// enable the timer

}

void logicAnalyzerCaptureStop(void)
{
	BP_LA_SRAM_DESELECT();    
	BP_LA_LATCH_CLOSE();
	// disable clk
	timer_disable_counter(BP_LA_TIMER);
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CLK_PIN); // CLK

}

void logicAnalyzerDumpSamples(uint32_t numSamples){

	uint32_t i;

	BP_LA_LATCH_CLOSE();
	BP_LA_SRAM_DESELECT();  
 	
 	//send mode reset command just in case
	setup_spix4w(); //write
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDRESETSPI); //write command
	BP_LA_SRAM_DESELECT();  

	//quad mode
	setup_spix1rw();
	BP_LA_SRAM_SELECT();  
	spiWx1(CMDQUADMODE);
	BP_LA_SRAM_DESELECT();

	BP_LA_LATCH_CLOSE();
 	setup_spix4w();
	BP_LA_SRAM_SELECT();  
	spiWx4(CMDREAD); //read command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	setup_spix4r(); //read
	spiRx4(); //dummy byte
	for(i=0; i<numSamples; i++){
		cdcputc2(spiRx4());
		//cdcprintf2("%d\t",spiRx4());
	}
	BP_LA_SRAM_DESELECT();//SRAM CS high  

}

void LA_start(void)
{
	int i;
	uint8_t cause;
	uint32_t countto, count;

	// send command to write sram at addr 0x0
	setup_spix4w(); //write
	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	delayms(1);
	spiWx4(CMDWRITE); //write command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address

	// turn spi bus around
	setup_spix4r();

	// setup triggers
	for(i=0; i<8; i++)
	{
		switch(triggers[i])
		{
			case 0:		exti_set_trigger((1<<i), EXTI_TRIGGER_RISING);
					exti_enable_request((1<<i));
					break;
			case 1:		exti_set_trigger((1<<i), EXTI_TRIGGER_FALLING);
					exti_enable_request((1<<i));
					break;
			case 2:		exti_set_trigger((1<<i), EXTI_TRIGGER_BOTH);
					exti_enable_request((1<<i));
					break;
			default:	exti_disable_request((1<<i));
					break;
		}
	}

	// show the current settings
	cdcprintf("Sampling clock=%dHz, #samples: %dK\r\n", (36000000/period), samples/1024);
	cdcprintf(" CHAN1 trigger: %s\r\n", triggermodes[triggers[0]]);
	cdcprintf(" CHAN2 trigger: %s\r\n", triggermodes[triggers[1]]);
	cdcprintf(" CHAN3 trigger: %s\r\n", triggermodes[triggers[2]]);
	cdcprintf(" CHAN4 trigger: %s\r\n", triggermodes[triggers[3]]);
	cdcprintf(" CHAN5 trigger: %s\r\n", triggermodes[triggers[4]]);
	cdcprintf(" CHAN6 trigger: %s\r\n", triggermodes[triggers[5]]);
	cdcprintf(" CHAN7 trigger: %s\r\n", triggermodes[triggers[6]]);
	cdcprintf(" CHAN8 trigger: %s\r\n", triggermodes[triggers[7]]);
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
		cdcprintf("\r%c Waiting for trigger %c", spinner[i], spinner[i]);
		i++;
		i&=0x03;
		delayms(100);

		if(cdcbyteready())
		{
			stop=11;		//user interrupt
			cdcgetc();
		}
			
	}

	// store the 'cause'
	cause=stop;
	count=counts;

	// get extra samples after the trigger unless trigger by overrun/user
	if((cause!=10)&&(cause!=11))
	{
		countto=counts+extrasamples;
		while(counts!=countto);
	}

	// disable clk
	timer_disable_counter(BP_LA_TIMER);

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

	if(cause<10)
		cdcprintf("\r\nCapture stopped on trigger chan %d after %d samples, sampled total %d samples", stop, count, counts);
	else if(cause==10)
		cdcprintf("\r\nCapture stopped max. samples (%d)", count);
	else if(cause==11)
		cdcprintf("\r\nCapture interrupted by user at %d samples", count);
	else cdcprintf("\r\nCapture stopped for unknown reasons at %d samples", count);
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
	switch(macro)
	{
		case 0:		cdcprintf("1..8. Setup trigger on chan 1..8\r\n9. set period\r\n10. set samples\r\n11. show buffer\r\n12. transfer buffer");
				break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:		triggers[macro-1]=(askint(LATRIGGERMENU, 1, 4, 4)-1);
				break;
		case 9:		period=(askint(LAPERIODMENU, 1, 65536, 4500));
				break;
		case 10:	samples=(askint(LASAMPLEMENU, 1024, 256*1024, 4096));
				extrasamples=3*(samples/4);
				break;
		case 11:	displaybuff();
				break;
		case 12:	getbuff();
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
	gpio_set_mode(BP_LA_LATCH_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_LATCH_PIN); 	// 573 latch
	gpio_set_mode(BP_LA_SRAM_CS_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CS_PIN);	// CS
	gpio_set(BP_LA_LATCH_PORT, BP_LA_LATCH_PIN);
	setup_spix1rw();

	//force to sequencial mode just in case
	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	delayms(1);
	spiWx1(CMDWRITERREG);
	spiWx1(0x40);
	gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);

	// softspi setup sram chip into quad mode
	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	delayms(1);
	spiWx1(CMDQUADMODE);
	gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN); //SRAM CS high

	// program timer as much as possible
	rcc_periph_clock_enable(BP_LA_TIM_CLOCK);								// enable clock
	timer_reset(BP_LA_TIMER);										// reset peripheral
	timer_set_mode(BP_LA_TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);			// count up
	timer_set_oc_mode(BP_LA_TIMER, BP_LA_TIM_CHAN, TIM_OCM_PWM1);						// PWM1 == high/low; PWM2= low/high
	timer_enable_oc_output(BP_LA_TIMER, BP_LA_TIM_CHAN);							// output channel
	timer_enable_break_main_output(BP_LA_TIMER);								// need to set break
	timer_enable_irq(BP_LA_TIMER, TIM_DIER_CC1IE);
	nvic_enable_irq(BP_LA_TIM_NVIC);									// enable timer irq

	// setup triggers
	exti_select_source(EXTI0, BP_LA_CHAN1_PORT);
	exti_select_source(EXTI1, BP_LA_CHAN2_PORT);
	exti_select_source(EXTI2, BP_LA_CHAN3_PORT);
	exti_select_source(EXTI3, BP_LA_CHAN4_PORT);
	exti_select_source(EXTI4, BP_LA_CHAN5_PORT);
	exti_select_source(EXTI5, BP_LA_CHAN6_PORT);
	exti_select_source(EXTI6, BP_LA_CHAN7_PORT);
	exti_select_source(EXTI7, BP_LA_CHAN8_PORT);
	nvic_enable_irq(NVIC_EXTI0_IRQ);
	nvic_enable_irq(NVIC_EXTI1_IRQ);
	nvic_enable_irq(NVIC_EXTI2_IRQ);
	nvic_enable_irq(NVIC_EXTI3_IRQ);
	nvic_enable_irq(NVIC_EXTI4_IRQ);
	nvic_enable_irq(NVIC_EXTI9_5_IRQ);


	// defaults
	period=4500;
	samples=4096;
	extrasamples=4096-1024;
	for(i=0; i<8; i++) triggers[i]=3; 	// no triggers
}

void LA_cleanup(void)
{
	// back to regular spi
	spiWx4(CMDRESETSPI);

	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN1_PIN);		// MOSI
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN2_PIN);		// MISO
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_CHAN5_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN5_PIN);		// MOSI
	gpio_set_mode(BP_LA_CHAN6_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN6_PIN);		// MISO
	gpio_set_mode(BP_LA_CHAN7_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN7_PIN);
	gpio_set_mode(BP_LA_CHAN8_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN8_PIN);
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
	int i, j, exit;
	uint8_t mask;
	uint32_t offset;

	exit=0;
	offset=0;

	while(!exit)
	{
		cdcprintf("\x1B[2J\x1B[H");

		for(j=0; j<8; j++)
		{
			mask=0x01<<j;
			cdcprintf("\x1B[0;9%dmCH%d:\t", j, j+1);

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
			case 'x':	exit=1;
					break;
		}
		
	}
}

void setup_spix1rw(void)
{
	// channnels/SPI set all to input...
	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN1_PIN);		
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN2_PIN);		
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_CHAN5_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN5_PIN);
	gpio_set_mode(BP_LA_CHAN6_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN6_PIN);		
	gpio_set_mode(BP_LA_CHAN7_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN7_PIN);
	gpio_set_mode(BP_LA_CHAN8_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN8_PIN);
	
	//outputs now set the two MOSI to output
	gpio_set_mode(BP_SRAM1_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SRAM1_MOSI_PIN);	// MOSI
	gpio_set_mode(BP_SRAM2_MOSI_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_SRAM2_MOSI_PIN);	// MOSI	
	
	// setup GPIOs
	BP_LA_SRAM_CLOCK_SETUP(); // CLK
	BP_LA_LATCH_SETUP();	// 573 latch
	BP_LA_SRAM_CS_SETUP();// CS

	BP_LA_SRAM_CLOCK_LOW();
	BP_LA_LATCH_CLOSE(); //latch high
	BP_LA_SRAM_DESELECT(); //SRAM CS high
}

static void spiWx1(uint8_t d)
{
	int i;
	uint8_t mask;
	mask=0x80;

	for(i=0; i<8; i++)
	{
		if(d&mask) gpio_set(BP_SRAM1_MOSI_PORT, BP_SRAM1_MOSI_PIN);
			else gpio_clear(BP_SRAM1_MOSI_PORT, BP_SRAM1_MOSI_PIN);
		if(d&mask) gpio_set(BP_SRAM2_MOSI_PORT, BP_SRAM2_MOSI_PIN);
			else gpio_clear(BP_SRAM2_MOSI_PORT, BP_SRAM2_MOSI_PIN);
			
		BP_LA_SRAM_CLOCK_HIGH();
		BP_LA_SRAM_CLOCK_LOW();
		mask>>=1;

	}

	
}

void setup_spix4w(void)
{
	// set SIO pins to output to enable quadspi
	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN1_PIN);
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN2_PIN);
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_CHAN5_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN5_PIN);
	gpio_set_mode(BP_LA_CHAN6_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN6_PIN);
	gpio_set_mode(BP_LA_CHAN7_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN7_PIN);
	gpio_set_mode(BP_LA_CHAN8_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_CHAN8_PIN);
	BP_LA_SRAM_CLOCK_SETUP();
	BP_LA_SRAM_CLOCK_LOW();
}

void setup_spix4r(void)
{
	// set SIO pins to input
	gpio_set_mode(BP_LA_CHAN1_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN1_PIN);
	gpio_set_mode(BP_LA_CHAN2_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN2_PIN);
	gpio_set_mode(BP_LA_CHAN3_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN3_PIN);
	gpio_set_mode(BP_LA_CHAN4_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN4_PIN);
	gpio_set_mode(BP_LA_CHAN5_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN5_PIN);
	gpio_set_mode(BP_LA_CHAN6_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN6_PIN);
	gpio_set_mode(BP_LA_CHAN7_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN7_PIN);
	gpio_set_mode(BP_LA_CHAN8_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_LA_CHAN8_PIN);
	BP_LA_SRAM_CLOCK_SETUP();
	BP_LA_SRAM_CLOCK_LOW();
}

static uint8_t spiRx4(void)
{
	uint8_t received;

	received=0;

	BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
		
	//READ 
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN8_PORT, BP_LA_CHAN8_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN7_PORT, BP_LA_CHAN7_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN6_PORT, BP_LA_CHAN6_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN5_PORT, BP_LA_CHAN5_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN4_PORT, BP_LA_CHAN4_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN3_PORT, BP_LA_CHAN3_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN2_PORT, BP_LA_CHAN2_PIN)?1:0);
	received<<=1;
	received|=(gpio_get(BP_LA_CHAN1_PORT, BP_LA_CHAN1_PIN)?1:0);

	BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW

	return received;
}

static void spiWx4(uint8_t d)
{
	int i;
	uint8_t mask;

	mask=0x80;

	for(i=0; i<2; i++)
	{
		if(d&mask) gpio_set(BP_SRAM1_SIO3_PORT, BP_SRAM1_SIO3_PIN);
			else gpio_clear(BP_SRAM1_SIO3_PORT, BP_SRAM1_SIO3_PIN);
		if(d&mask) gpio_set(BP_SRAM2_SIO3_PORT, BP_SRAM2_SIO3_PIN);
			else gpio_clear(BP_SRAM2_SIO3_PORT, BP_SRAM2_SIO3_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_SRAM1_SIO2_PORT, BP_SRAM1_SIO2_PIN);
			else gpio_clear(BP_SRAM1_SIO2_PORT, BP_SRAM1_SIO2_PIN);
		if(d&mask) gpio_set(BP_SRAM2_SIO2_PORT, BP_SRAM2_SIO2_PIN);
			else gpio_clear(BP_SRAM2_SIO2_PORT, BP_SRAM2_SIO2_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_SRAM1_SIO1_PORT, BP_SRAM1_SIO1_PIN);
			else gpio_clear(BP_SRAM1_SIO1_PORT, BP_SRAM1_SIO1_PIN);
		if(d&mask) gpio_set(BP_SRAM2_SIO1_PORT, BP_SRAM2_SIO1_PIN);
			else gpio_clear(BP_SRAM2_SIO1_PORT, BP_SRAM2_SIO1_PIN);
		mask>>=1;
		if(d&mask) gpio_set(BP_SRAM1_SIO0_PORT, BP_SRAM1_SIO0_PIN);
			else gpio_clear(BP_SRAM1_SIO0_PORT, BP_SRAM1_SIO0_PIN);
		if(d&mask) gpio_set(BP_SRAM2_SIO0_PORT, BP_SRAM2_SIO0_PIN);
			else gpio_clear(BP_SRAM2_SIO0_PORT, BP_SRAM2_SIO0_PIN);
		mask>>=1;
		
		BP_LA_SRAM_CLOCK_HIGH(); //CLOCK HIGH
 
		BP_LA_SRAM_CLOCK_LOW(); //CLOCK LOW
	}

	return;
}



static void getbuff(void)
{
	int i;

	setup_spix4w(); //write
	gpio_clear(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);
	delayms(1);
	spiWx4(0x03); //read command
	spiWx4(0);
	spiWx4(0);
	spiWx4(0); //3 byte address
	setup_spix4r(); //read
	spiRx4(); //dummy byte

	for(i=0; i<(BP_LA_BUFFSIZE); i++)
	{
		labuff[i]=spiRx4();
	}

	gpio_set(BP_LA_SRAM_CS_PORT, BP_LA_SRAM_CS_PIN);



}



// interrupt service routines
// tim supplies the clock to the sram
void tim1_cc_isr(void)
{

	if (timer_get_flag(TIM1, TIM_SR_CC1IF))					// did we overflow?
	{
		counts++;
		timer_clear_flag(TIM1, TIM_SR_CC1IF);

		if(counts==samples)						// smaples enough
		{
			stop=10;
			timer_disable_counter(BP_LA_TIMER);			// enable the timer
		}	
	}
}

void exti0_isr(void)
{
	stop=1;
	exti_reset_request(EXTI0);
}

void exti1_isr(void)
{
	stop=2;
	exti_reset_request(EXTI1);
}

void exti2_isr(void)
{
	stop=3;
	exti_reset_request(EXTI2);
}

void exti3_isr(void)
{
	stop=4;
	exti_reset_request(EXTI3);
}

void exti4_isr(void)
{
	stop=5;
	exti_reset_request(EXTI4);
}

void exti9_5_isr(void)
{
	if(exti_get_flag_status(EXTI5))
	{
		stop=6;
		exti_reset_request(EXTI5);
	}

	if(exti_get_flag_status(EXTI6))
	{
		stop=7;
		exti_reset_request(EXTI6);
	}

	if(exti_get_flag_status(EXTI7))
	{
		stop=8;
		exti_reset_request(EXTI7);
	}
}


