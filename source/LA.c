

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

static uint8_t stop;
static uint16_t period;
static uint32_t samples,counts;
static uint8_t triggers[8];
char triggermodes[][4]={
"_/\"",
"\"\\_",
"#X#",
"N/A"
};

void logicAnalyzerSetSampleSpeed(uint16_t speed){

	period=speed;
	cdcprintf("\r\nLA period: %d", period);

}

void logicAnalyzerSetup(void)
{
	uint8_t i;

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
	
	//PWM timer, clock for SRAMs	
	rcc_periph_clock_enable(RCC_GPIOA);
	timer_reset(BP_LA_TIMER); // reset PWM timer
	rcc_periph_clock_enable(BP_LA_TIM_CLOCK); // enable clock
	timer_set_mode(BP_LA_TIMER, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE,TIM_CR1_DIR_UP); // PWN, edge update, count up
	timer_set_oc_mode(BP_LA_TIMER, TIM_OC1, TIM_OCM_PWM2); // PWM1 == high/low; PWM2= low/high
	timer_enable_oc_output(BP_LA_TIMER, TIM_OC1); // output channel
	timer_enable_break_main_output(BP_LA_TIMER); // need to set break
	timer_set_master_mode(BP_LA_TIMER,TIM_CR2_MMS_UPDATE); //enable master output to slave

	//slave timer, counts samples
	timer_reset(TIM2);
	rcc_periph_clock_enable(BP_LA_COUNTER_CLOCK);
	timer_set_prescaler(BP_LA_COUNTER,0x00); //counts two ticks per PWM pulse for some reason, still need to debug
	timer_slave_set_polarity(BP_LA_COUNTER, TIM_ET_RISING);
	timer_slave_set_trigger(BP_LA_COUNTER,TIM_SMCR_TS_ITR0); //timer1 ouit to timer2/3/4 in with ITR0
	timer_slave_set_mode(BP_LA_COUNTER,TIM_SMCR_SMS_ECM1); //slave counting mode
	//counter interrupts for overflow detection and sample tracking
	timer_enable_irq(BP_LA_COUNTER, TIM_DIER_CC1IE);
	nvic_enable_irq(BP_LA_COUNTER_NVIC);	// enable timer irq
	
	// setup triggers
	/*exti_select_source(EXTI0, BP_LA_CHAN1_PORT);
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
	for(i=0; i<8; i++) triggers[i]=3; 	// no triggers
	*/

	// defaults
	period=4500;
	samples=4096;
	for(i=0; i<8; i++) triggers[i]=3; 	// triggers (not used in interactive mode)
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
	rcc_periph_clock_enable(BP_LA_TIM_CLOCK);
	
	// timer
	timer_set_oc_value(BP_LA_TIMER, BP_LA_TIM_CHAN, (period/2));				// set match value
	timer_set_period(BP_LA_TIMER, period);									// set period 
	timer_set_counter(BP_LA_TIMER,0);
	timer_set_counter(BP_LA_COUNTER,0);
	
	// setup triggers
	/*for(i=0; i<8; i++)
	{
		switch(triggers[i])
		{
			case 0:	exti_set_trigger((1<<i), EXTI_TRIGGER_RISING);
					exti_enable_request((1<<i));
					break;
			case 1:	exti_set_trigger((1<<i), EXTI_TRIGGER_FALLING);
					exti_enable_request((1<<i));
					break;
			case 2:	exti_set_trigger((1<<i), EXTI_TRIGGER_BOTH);
					exti_enable_request((1<<i));
					break;
			default:	exti_disable_request((1<<i));
					break;
		}
	}*/
	
	timer_enable_counter(BP_LA_COUNTER);	
	timer_enable_counter(BP_LA_TIMER);									// enable the timer

}

void logicAnalyzerCaptureStop(void)
{
	uint32_t capturedsamples;
	
	BP_LA_SRAM_DESELECT();    
	BP_LA_LATCH_CLOSE();
	// disable clk
	timer_disable_counter(BP_LA_TIMER);
	rcc_periph_clock_disable(BP_LA_TIM_CLOCK);					// turn peripheral off
	gpio_set_mode(BP_LA_SRAM_CLK_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, BP_LA_SRAM_CLK_PIN); // CLK
	
	/*// timer
	nvic_disable_irq(BP_LA_COUNTER_NVIC);
	
	// disable triggers
	exti_disable_request(EXTI0);
	exti_disable_request(EXTI1);
	exti_disable_request(EXTI2);
	exti_disable_request(EXTI3);
	exti_disable_request(EXTI4);
	exti_disable_request(EXTI5);
	exti_disable_request(EXTI6);
	exti_disable_request(EXTI7);*/
	
	capturedsamples=timer_get_counter(BP_LA_COUNTER);
	cdcprintf("LA samples: %d\r\n", capturedsamples);

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
	spiRx4(); //dummy byte (need two reads to clear one byte)
	for(i=0; i<numSamples; i++){
		cdcputc2(spiRx4());
	}
	BP_LA_SRAM_DESELECT();//SRAM CS high  

}

void LA_macro(uint32_t macro)
{
	switch(macro)
	{
		case 0:		cdcprintf("1..8. Setup trigger on chan 1..8\r\n9. set period\r\n10. set samples");
				break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:	triggers[macro-1]=(askint(LATRIGGERMENU, 1, 4, 4)-1);
				break;
		case 9:	period=(askint(LAPERIODMENU, 1, 65536, 4500));
				break;
		case 10:	samples=(askint(LASAMPLEMENU, 1024, 256*1024, 4096));
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
}

void LA_cleanup(void)
{
}
void LA_pins(void)
{
}
void LA_settings(void)
{
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


// interrupt service routines
// tim supplies the clock to the sram
void tim2_isr(void)
{

	if (timer_get_flag(BP_LA_COUNTER, TIM_SR_CC1IF))					// did we overflow?
	{
		counts++;
		timer_clear_flag(BP_LA_COUNTER, TIM_SR_CC1IF);

		if(counts==samples)						// smaples enough
		{
			stop=10;
			timer_disable_counter(BP_LA_COUNTER);			// enable the timer
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


