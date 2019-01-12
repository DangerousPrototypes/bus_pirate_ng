#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/f1/bkp.h>
#include <libopencm3/cm3/scb.h>

#include "debug.h"
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"
#include "protocols.h"
#include "AUXpin.h"
#include "ADC.h"
#include "bpflash.h"
#include "LA.h"
#include "sump.h"
#include "selftest.h"

// globals
uint32_t cmdhead, cmdtail, cursor;		// TODO swap head an tail?
char cmdbuff[CMDBUFFSIZE];
struct _modeConfig modeConfig;

// global constants
const char vpumodes[][4] = {
"EXT\0",
"3V3\0",
"5V0\0"
};

const char bitorders[][4] ={
"MSB\0",
"LSB\0"
};

const char states[][4] ={
"OFF\0",
"ON\0"
};

const char displaymodes[][4] ={
"DEC\0",
"HEX\0",
"OCT\0",
"BIN\0"
};



// eats up the spaces and comma's from the cmdline
void consumewhitechars(void)
{
	while((cmdtail!=cmdhead)&&((cmdbuff[cmdtail]==' ')||(cmdbuff[cmdtail]==',')))
	{
		cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
	}
}

// decodes value from the cmdline
// XXXXXX integer
// 0xXXXX hexadecimal
// 0bXXXX binair
uint32_t getint(void)
{
	int i;
	uint32_t number;

	i=0;
	number=0;

	if((cmdbuff[cmdtail]>=0x31)&&(cmdbuff[cmdtail]<=0x39))			// 1-9 decimal
	{
		number=cmdbuff[cmdtail]-0x30;
		i=1;
		
		while((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39))
		{
			number*=10;
			number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
			i++;
		} 
	}
	else if(cmdbuff[cmdtail]==0x30)
	{
		if((cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]<=0x39))		// 0-9 decimal
		{
			i=2;
		
			while((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39))
			{
				number*=10;
				number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
				i++;
			} 

		}
		else if((cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='x')||(cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='X'))		// 0x hexadecimal
		{
			i=2;

			while(((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39)) || \
				((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>='a')&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<='f')) || \
				((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>='A')&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<='F')))
			{
				number<<=4;
				if((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x39))
				{
					number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
				}
				else
				{
					cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]|=0x20;		// to lowercase
					number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x57;	// 0x61 ('a') -0xa
				}
				i++;
			}

		}
		else if((cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='b')||(cmdbuff[((cmdtail+1)&(CMDBUFFSIZE-1))]=='B'))		// 0b hexadecimal
		{
			i=2;

			while((cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]>=0x30)&&(cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]<=0x31))
			{
				number<<=1;
				number+=cmdbuff[((cmdtail+i)&(CMDBUFFSIZE-1))]-0x30;
				i++;
			} 
		}
		else									// not perse wrong assume user entered 0
		{
			number=0;
			i=1;
			//	modeConfig.error=1;
		}
	}
	else
	{
		modeConfig.error=1;
	}

	// update cmdbuff pointers
	cmdtail=((cmdtail+(i-1))&(CMDBUFFSIZE-1));

	// tell the userinput (0 can be error!!)
	return number;
}

// initializes the variables
void initUI(void)
{
	int i;

	for(i=0; i<CMDBUFFSIZE; i++) cmdbuff[i]=0;
	cmdhead=0;
	cmdtail=0;

	modeConfig.wwr=0;
	modeConfig.numbits=8;
	modeConfig.oc=0;
	modeConfig.mode=0;
	modeConfig.pullups=0;
	modeConfig.vpumode=0;
	modeConfig.psu=0;
	modeConfig.bitorder=0;
	modeConfig.error=0;
	modeConfig.displaymode=0;
	modeConfig.pwm=0;
	modeConfig.init=0;
	modeConfig.mosiport=0;
	modeConfig.mosipin=0;
	modeConfig.misoport=0;
	modeConfig.misopin=0;
	modeConfig.csport=0;
	modeConfig.cspin=0;
	modeConfig.clkport=0;
	modeConfig.clkpin=0;

	modeConfig.subprotocolname=0;
}

int isbuscmd(char c)
{
	switch(c)
	{
		case '[':	// start
		case ']':	// stop
		case '{':	// start 
		case '}':	// stop
		case '/':	// clk h
		case '\\':	// clk l
		case '^':	// clk tick
		case '-':	// dat hi
		case '_':	// dat l
		case '.':	// dat s
		case '!':	// read bit
		case 'r':	// read
		case '0':	
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':	// send value
		case '\"':	// send string
				return 1;
				break;
		default: 	return 0;
	}
}

// one big loop eating all user input. executed/interpretted it when the user presses enter
//
void doUI(void)
{
	int go;
	char c;

	uint32_t temp, temp2, temp3, repeat, received, bits;
	int i;

	go=0;

	// wait for usb ready
//	while(!usbready());
	delayms(500);

	// show welcome
	//versioninfo();
	//cdcprintf("%s> ", protocols[modeConfig.mode].protocol_name);


	while(1)
	{
		getuserinput();
		
		//command received, start logic capture
		//TODO: destinguish between bus and non-bus commands
		logicAnalyzerCaptureStart();


		//cdcprintf2("cmd=%s\r\n", cmdbuff+cmdtail);
//		for(i=0; i<512; i++)
//		{
//			cdcputc2(((cmdbuff[i]>=0x20)&&(cmdbuff[i]<=0x7E))?cmdbuff[i]:'_');
//		}
//		cdcprintf2("\r\n");


		if(protocols[modeConfig.mode].protocol_periodic())
			go=2;
		else
			go=1;

		cdcprintf("\r\n");
		while((go==1)&&(cmdtail!=cmdhead))
		{
			c=cmdbuff[cmdtail];

			// delayed init is handled here
			if((!modeConfig.init)&&(isbuscmd(c)))
			{
				if(modeConfig.mode!=0)
				{
					cdcprintf("running postphoned HWinit()\r\n");
					protocols[modeConfig.mode].protocol_setup_exc();
					modeConfig.init=1;
				}
			}

			switch (c)
			{
				case '(':	
						cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);		// advance 1 position
						temp=getint();
						cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);		// advance 1
						if(cmdbuff[cmdtail]==')')
						{
							protocols[modeConfig.mode].protocol_macro(temp);
						}
						else
						{
							modeConfig.error=1;
							cdcprintf("Error parsing macro");
						}
						break;
				case '[':	modeConfig.wwr=0;
						protocols[modeConfig.mode].protocol_start();
						break;
				case ']':	modeConfig.wwr=0;
						protocols[modeConfig.mode].protocol_stop();
						break;
				case '{':	modeConfig.wwr=1;
						protocols[modeConfig.mode].protocol_startR();
						break;
				case '}':	modeConfig.wwr=0;
						protocols[modeConfig.mode].protocol_stopR();
						break;
				case '/':	protocols[modeConfig.mode].protocol_clkh();
						break;
				case '\\':	protocols[modeConfig.mode].protocol_clkl();
						break;
				case '^':	protocols[modeConfig.mode].protocol_clk();
						break;
				case '-':	protocols[modeConfig.mode].protocol_dath();
						break;
				case '_':	protocols[modeConfig.mode].protocol_datl();
						break;
				case '.':	protocols[modeConfig.mode].protocol_dats();
						break;
				case '!':	protocols[modeConfig.mode].protocol_bitr();
						break;
				case '&':	repeat=getrepeat();
						if(repeat<10) repeat=10;			// minimum is 10us!
						cdcprintf("Delaying %d us", repeat);
						while(repeat--)
						{
							delayus(1);
						}
						break;
				case '%':	repeat=getrepeat();
						cdcprintf("Delaying %d ms", repeat);
						while(repeat--)
						{
							delayms(1);
						}
						break;
				case 'a':	if(modeConfig.mode!=HIZ)
						{
							cdcprintf("SET AUX=0");
							setAUX(0);
						}
						else
						{
							cdcprintf("Can't set AUX in HiZ mode!");
							modeConfig.error=1;
						}
						break;
				case 'A':	if(modeConfig.mode!=HIZ)
						{
							cdcprintf("SET AUX=1");
							setAUX(1);
						}
						else
						{
							cdcprintf("Can't set AUX in HiZ mode!");
							modeConfig.error=1;
						}
						break;
				case '@':	if(modeConfig.mode!=HIZ)
						{
							cdcprintf("AUX=%d", getAUX());
						}
						else
						{
							cdcprintf("Can't read AUX in HiZ mode!");
							modeConfig.error=1;
						}
						break;
				case 'b':	if(modeConfig.mode!=HIZ)
						{
							temp=askint(VPUMENU, 1, 3, 1);
							modeConfig.vpumode=temp-1;
							gpio_clear(BP_VPU50EN_PORT, BP_VPU50EN_PIN);			// Vpu=ext
							gpio_clear(BP_VPU33EN_PORT, BP_VPU33EN_PIN);
						}
						switch(modeConfig.vpumode)
						{
							case 1:								// 3v3
								gpio_set(BP_VPU33EN_PORT, BP_VPU33EN_PIN);
								gpio_clear(BP_VPU50EN_PORT, BP_VPU50EN_PIN);
							case 2:								// 5v0
								gpio_set(BP_VPU33EN_PORT, BP_VPU33EN_PIN);
								gpio_set(BP_VPU50EN_PORT, BP_VPU50EN_PIN);
							default:
								gpio_clear(BP_VPU50EN_PORT, BP_VPU50EN_PIN);
								gpio_clear(BP_VPU33EN_PORT, BP_VPU33EN_PIN);
						}
						break;
				case 'd':	if(modeConfig.mode!=HIZ)
						{
							uint16_t adc = voltage(BP_ADC_CHAN, 1);
							cdcprintf("ADC=%d.%02dV", adc/1000, (adc%1000)/10);
						}
						else
						{
							cdcprintf("Can't read ADC in HiZ mode!");
							modeConfig.error=1;
						}
						break;
				case 'D':	if(modeConfig.mode!=HIZ)
						{
							cdcprintf("Press any key to exit\r\n");
							while(!cdcbyteready())
							{
								uint16_t adc = voltage(BP_ADC_CHAN, 1);
								cdcprintf("ADC=%d.%02dV\r", adc/1000, (adc%1000)/10);
								delayms(250);
							}
							cdcgetc();
						}
						else
						{
							cdcprintf("Can't read ADC in HiZ mode!");
							modeConfig.error=1;
						}
						break;
				case 'f':	cdcprintf("freq=%ld Hz", getfreq());
						break;
				case 'g':	setPWM(0, 0);				// disable PWM
						break;
				case 'G':	if(modeConfig.mode!=HIZ)
						{
							temp=askint(PWMMENUPERIOD, 1, 0xFFFFFFFF, 1000);
							temp2=askint(PWMMENUOC, 1, 0xFFFFFFFF, 200);
							setPWM(temp, temp2);			// enable PWM
							if(modeConfig.pwm)							
								cdcprintf("\r\nPWM on");
							else
								cdcprintf("\r\nPWM off");
						}
						else
						{
							cdcprintf("Can't use PWM in HiZ mode!");
						}
						break;
				case 'h':
				case '?':	printhelp();
						break;
				case 'H':	protocols[modeConfig.mode].protocol_help();
						break;
				case 'i':	versioninfo();
						break;
				case 'l':	modeConfig.bitorder=0;
						cdcprintf("Bitorder: MSB");
						break;
				case 'L':	modeConfig.bitorder=1;
						cdcprintf("Bitorder: LSB");
						break;
				case 'm':	changemode();
						break;
				case 'o':	changedisplaymode();
						break;
				case 'p':	gpio_clear(BP_VPUEN_PORT, BP_VPUEN_PIN);	// always permitted 
						cdcprintf("pullups: disabled");
						modeConfig.pullups=0;
						break;
				case 'P':	if(modeConfig.mode!=0)		// reset vpu mode to externale??
						{
							gpio_set(BP_VPUEN_PORT, BP_VPUEN_PIN);
							cdcprintf("pullups: enabled\r\n");
							delayms(10);
							uint16_t vpu = voltage(BP_VPU_CHAN, 1);
							cdcprintf("Vpu=%d.%02dV (mode=%s)", vpu/1000, (vpu%1000)/10, vpumodes[modeConfig.vpumode]);
							modeConfig.pullups=1; 
						}
						else
						{
							cdcprintf("Cannot enable pullups in HiZ");
							modeConfig.error=1;
						}
						break;
				case 'r':	repeat=getrepeat();
						while(repeat--)
						{
							received=protocols[modeConfig.mode].protocol_read();
							cdcprintf("RX: ");
							printnum(received);
							if(repeat) cdcprintf("\r\n");
						}
						break;
				case 'v':	showstates();
						break;
				case 'w':	gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);	// always permitted to shut power off
						cdcprintf("PSU: disabled");
						modeConfig.psu=0;
						break;
				case 'W':	if(modeConfig.mode!=0)
						{
							gpio_set(BP_PSUEN_PORT, BP_PSUEN_PIN); 
							cdcprintf("PSU: enabled\r\n");
							delayms(10);
							uint16_t v33 = voltage(BP_3V3_CHAN, 1);
							uint16_t v50 = voltage(BP_5V0_CHAN, 1);
							cdcprintf("V33=%d.%02dV, V50=%d.%02dV", v33/100, (v33%1000)/10, v50/1000, (v50%1000)/10); 
							if((voltage(BP_3V3_CHAN, 1)<3000)||(voltage(BP_5V0_CHAN, 1)<4500))
							{
								cdcprintf("\r\nShort circuit!");								
								gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);
							}
							else
								modeConfig.psu=1;
						}
						else
						{
							cdcprintf("Cannot enable PSU in HiZ!");
							modeConfig.error=1;
						}
						break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':	temp=getint();
						bits=getnumbits();		// sequence is important! TODO: make freeform
						if(bits) modeConfig.numbits=bits;
						repeat=getrepeat();
						if (modeConfig.numbits<32) temp&=((1<<modeConfig.numbits)-1);
						while(repeat--)
						{
							cdcprintf("TX: ");
							printnum(temp);
							cdcprintf(" ");
							received=protocols[modeConfig.mode].protocol_send(orderbits(temp));		// reshuffle bits if necessary
							if(modeConfig.wwr) 
							{
								cdcprintf(", RX: ");
								printnum(received);
							}
							if(repeat) cdcprintf("\r\n");
						}
						break;
				case '\"':	temp=1;
						modeConfig.error=1;
						cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
						while((((cmdtail+temp)&(CMDBUFFSIZE-1))!=cmdhead)&&(cmdbuff[((cmdtail+temp)&(CMDBUFFSIZE-1))]!='\"'))
						{
							temp++;
							if(cmdbuff[((cmdtail+temp)&(CMDBUFFSIZE-1))]=='\"') modeConfig.error=0;
						}
						if(!modeConfig.error)
						{
							temp=0;
							while(cmdbuff[cmdtail]!='\"')
							{
								cdcprintf("TX: ");
								printnum(cmdbuff[cmdtail]);
								cdcprintf(" ");
								received=protocols[modeConfig.mode].protocol_send(orderbits(cmdbuff[cmdtail])); // reshuffle bits if necessary
								if(modeConfig.wwr) 
								{
									cdcprintf(", RX: ");
									printnum(received);
								}
								cdcprintf("\r\n");
								cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
							}

						}
						else
							cdcprintf("missing terminating \"");
				case 0x00:
				case ' ':	break;
				case ',':	break;	// reuse this command??
				case '$':	jumptobootloader();
						break;
				case '#':	reset();
						break;
				case '=':	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
						temp=getint();
						temp2=modeConfig.displaymode;		// remember old displaymode
						temp3=modeConfig.numbits;		// remember old numbits
						modeConfig.numbits=32;
						for(i=0; i<4; i++)
						{
							cdcprintf("=");
							modeConfig.displaymode=i;
							printnum(temp);
						}
						modeConfig.numbits=temp3;
						break;
				case '|':	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);
						temp=getint();
						temp2=modeConfig.displaymode;		// remember old displaymode
						modeConfig.bitorder^=1;
						temp3=modeConfig.numbits;		// remember old numbits
						modeConfig.numbits=32;
						for(i=0; i<4; i++)
						{
							cdcprintf("|");
							modeConfig.displaymode=i;
							printnum(orderbits(temp));
						}
						modeConfig.bitorder^=1;
						modeConfig.numbits=temp3;
						break;
				case '~': 	selftest();
						break;
				default:	cdcprintf("Unknown command: %c", c);
						modeConfig.error=1;
						//go=0;
						//cmdtail=cmdhead-1;
						break;	
			}
			cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance to next char/command
			if((c!=' ')&&(c!=0x00)&&(c!=',')) cdcprintf("\r\n");

			if(modeConfig.error)			// something went wrong
			{
				cdcprintf("\x07");		// bell!
				go=0;
				cmdtail=cmdhead;
				modeConfig.error=0;
			}
		}

		//stop logic capture and dump data
		//TODO:destinguish between bus activity and other commands
		//only dump if bus command executed
		logicAnalyzerCaptureStop();
		
		if(modeConfig.logicanalyzerstop==0xff){
			cdcprintf("\x07Logic analyzer full before end of command!\r\n");
		}	
		
		if(modeConfig.subprotocolname)
			cdcprintf("%s-(%s)> ", protocols[modeConfig.mode].protocol_name, modeConfig.subprotocolname);
		else
			cdcprintf("%s> ", protocols[modeConfig.mode].protocol_name);
		if(go==2)
		{
			temp=0;
			while(((cmdtail+temp)&(CMDBUFFSIZE-1))!=cmdhead)
			{
				cdcputc(cmdbuff[((cmdtail+temp)&(CMDBUFFSIZE-1))]);
				temp++;
			}
		}
		go=0;
	}
}

// display teh versioninfo about the buspirate
// when not in HiZ mode it dumps info about the pins/voltags etc.
void versioninfo(void)
{
	int i;
	uint32_t *id = (uint32_t *)0x1FFFF7E8;
	uint16_t flashsize = *(uint16_t *) 0x1FFFF7E0;
	uint16_t ramsize=96;
	uint32_t pwmperiod, pwmoc;

	// sram and flash size goed hand in hand
	if(flashsize<=16) ramsize=6;
	else if(flashsize<=32) ramsize=10;
	else if(flashsize<=128) ramsize=20;
	else if(flashsize<=512) ramsize=64;
	else ramsize=96;

	cdcprintf("Bus Pirate NextGen %s\r\n", BP_PLATFORM);
	cdcprintf("Firmware %s (%s), bootloader N/A\r\n", FIRMWARE_VERSION, FWVER);
	cdcprintf("STM32 with %dK FLASH, %dK SRAM ", flashsize, ramsize);
	cdcprintf("s/n: %08X%08X%08X\r\n", id[0], id[1], id[2]);
	cdcprintf("https://dangerousprototypes.com/\r\n");

	cdcprintf("Available busprotocols:");

	for(i=0; i<MAXPROTO; i++)
	{
		cdcprintf(" %s", protocols[i].protocol_name);
	}
	cdcprintf("\r\n");

	protocols[modeConfig.mode].protocol_settings();
	cdcprintf("\r\n");

	if(modeConfig.mode!=HIZ)
	{
		cdcprintf("#bits: %d, ",modeConfig.numbits);
		cdcprintf("bitorder: %s, ", bitorders[modeConfig.bitorder]);
		cdcprintf("PU: %s, ", states[modeConfig.pullups]);
		cdcprintf("Vpu mode: %s, ", vpumodes[modeConfig.vpumode]);
		cdcprintf("Power: %s\r\n", states[modeConfig.psu]);
		cdcprintf("Displaymode: %s, ", displaymodes[modeConfig.displaymode]);
		cdcprintf("PWM: %s\r\n", states[modeConfig.pwm]);
		if(modeConfig.pwm)
		{
			pwmperiod=TIM_ARR(BP_PWM_TIMER);

// TODO is there a better way to do this?
#if(BP_PWM_CHANCHAN==1)	
			pwmoc=TIM_CCR1(BP_PWM_TIMER);
#endif
#if(BP_PWM_CHANCHAN==2)	
			pwmoc=TIM_CCR2(BP_PWM_TIMER);
#endif
#if(BP_PWM_CHANCHAN==3)	
			pwmoc=TIM_CCR3(BP_PWM_TIMER);
#endif
#if(BP_PWM_CHANCHAN==4)	
			pwmoc=TIM_CCR4(BP_PWM_TIMER);
#endif
			cdcprintf("PWM clock %d Hz, dutycycle %d.%02d\r\n", (36000000/pwmperiod), pwmoc == pwmperiod ? 1 : 0, ((pwmoc*100)/pwmperiod)%100);
		}
		showstates();
	}
	if(modeConfig.init==0) cdcprintf("Pending HWinit()\r\n");
}

const char pinstates[][4] = {
"0\0",
"1\0",
"N/A\0"
};

const char pinmodes[][5] ={
"ANA.\0",		// analogue
"I-FL\0",		// input float
"I-UD\0",		// input pullup/down
"???\0",		// illegal
"O-PP\0",		// output pushpull
"O-OD\0",		// output opendrain
"O PP\0",		// output pushpull peripheral
"O OD\0",		// output opendrain peripheral
"----\0"		// pin is not used 	
};


uint8_t getpinmode(uint32_t port, uint16_t pin)
{
	uint32_t crl, crh;
	uint8_t pinmode, crpin, i;

	crl = GPIO_CRL(port);
	crh = GPIO_CRH(port);
	crpin=0;

	for(i=0; i<16; i++)
	{
		if((pin>>i)&0x0001)
		{
			crpin=(i<8?(crl>>(i*4)):(crh>>((i-8)*4)));
			crpin&=0x000f;
		}
	}

	pinmode=crpin>>2;

	if(crpin&0x03)		// >1 is output
	{
		pinmode+=4;
	}

	return pinmode;
}



// show voltages/pinstates
void showstates(void)
{
	uint8_t auxstate, csstate, misostate, clkstate, mosistate;
	uint8_t auxmode, csmode, misomode, clkmode, mosimode;
	uint16_t v50, v33, vpu, adc;

	cdcprintf("1.GND\t2.+5v\t3.+3V3\t4.Vpu\t5.ADC\t6.AUX\t7.CS\t8.MISO\t9.CLK\t10.MOSI\r\n");
	cdcprintf("GND\t+5v\t+3V3\tVpu\tADC\tAUX\t");
	protocols[modeConfig.mode].protocol_pins();
	cdcprintf("\r\n");

	// read pindirection
	auxmode=getpinmode(BP_AUX_PORT, BP_AUX_PIN);
	if(modeConfig.csport)
		csmode=getpinmode(modeConfig.csport, modeConfig.cspin);
	else
		csmode=8;

	if(modeConfig.misoport)
		misomode=getpinmode(modeConfig.misoport, modeConfig.misopin);
	else
		misomode=8;

	if(modeConfig.clkport)
		clkmode=getpinmode(modeConfig.clkport, modeConfig.clkpin);
	else
		clkmode=8;
	if(modeConfig.mosiport)
		mosimode=getpinmode(modeConfig.mosiport, modeConfig.mosipin);
	else
		mosimode=8;

	cdcprintf("PWR\tPWR\tPWR\tPWR\tAN\t%s\t%s\t%s\t%s\t%s\r\n", pinmodes[auxmode], pinmodes[csmode], pinmodes[misomode], pinmodes[clkmode], pinmodes[mosimode]);

	// pinstates
	auxstate=(gpio_get(BP_AUX_PORT, BP_AUX_PIN)?1:0);
	if(modeConfig.csport)
		csstate=(gpio_get(modeConfig.csport, modeConfig.cspin)?1:0);
	else
		csstate=2;

	if(modeConfig.misoport)
		misostate=(gpio_get(modeConfig.misoport, modeConfig.misopin)?1:0);
	else
		misostate=2;

	if(modeConfig.clkport)
		clkstate=(gpio_get(modeConfig.clkport, modeConfig.clkpin)?1:0);
	else
		clkstate=2;
	if(modeConfig.mosiport)
		mosistate=(gpio_get(modeConfig.mosiport, modeConfig.mosipin)?1:0);
	else
		mosistate=2;

	// adcs
	v33=voltage(BP_3V3_CHAN, 1);
	v50=voltage(BP_5V0_CHAN, 1);
	vpu=voltage(BP_VPU_CHAN, 1);  // TODO make difference between vextern (pin) and vpu (voltage used)??
	adc=voltage(BP_ADC_CHAN, 1);

	// show state of pin
	cdcprintf("GND\t%d.%02dV\t%d.%02dV\t%d.%02dV\t%d.%02dV\t%s\t%s\t%s\t%s\t%s\r\n", v50/1000, (v50%1000)/10, v33/1000, (v33%1000)/10, vpu/1000, (vpu%1000)/10, adc/1000, (adc%1000)/10, pinstates[auxstate], pinstates[csstate], pinstates[misostate], pinstates[clkstate], pinstates[mosistate]);


}

// takes care of mode switchover
void changemode(void)
{
	uint32_t mode;
	int i;

	
	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// pointer is set to 'm' we should advance 1
	consumewhitechars();			// eat whitechars
	mode=getint();

	if((mode>MAXPROTO)||(mode==0))
	{
		cdcprintf("\r\nIllegal mode!\r\n");
		modeConfig.error=1;
	}

	while(modeConfig.error)			// no integer found
	{
		cdcprintf("\x07");
		modeConfig.error=0;

		for(i=0; i<MAXPROTO; i++)
			cdcprintf(" %d. %s\r\n", i+1, protocols[i].protocol_name);

		cdcprintf("Mode> ");
		cmdtail=cmdhead;	// flush all input
		getuserinput();
		consumewhitechars(); 
		mode=getint();

		if((mode>MAXPROTO)||(mode==0))
		{
			cdcprintf("\r\nIllegal mode!\r\n");
			modeConfig.error=1;
		}
	}	

	protocols[modeConfig.mode].protocol_cleanup();		// switch to HiZ
	protocols[0].protocol_setup_exc();			// disables powersuppy etc.
	modeConfig.mode=mode-1;
	protocols[modeConfig.mode].protocol_setup();		// setup the new mode
	
	if(modeConfig.mode!=0)					// postphone the setup, this allows the user to setup the powersupply first 
	{
		cdcprintf("Postphoning HWinit()\r\n");
	}

	if(modeConfig.mode==0) 	gpio_clear(BP_MODE_LED_PORT, BP_MODE_LED_PIN);
		else 	gpio_set(BP_MODE_LED_PORT, BP_MODE_LED_PIN);

}

// set display mode  (hex, bin, octa, dec) 
void changedisplaymode(void)
{
	uint32_t displaymode;
	int i;


	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// pointer is set to 'o' we should advance 1
	consumewhitechars();			// eat whitechars
	displaymode=getint();

	if((displaymode>4)||(displaymode==0))
	{
		cdcprintf("\r\nIllegal displaymode!\r\n");
		modeConfig.error=1;
	}

	while(modeConfig.error)			// no integer found
	{
		cdcprintf("\x07");
		modeConfig.error=0;

		for(i=0; i<4; i++)
			cdcprintf(" %d. %s\r\n", i+1, displaymodes[i]);

		cdcprintf("Displaymode>");
		cmdtail=cmdhead;	// flush all input
		getuserinput();
		consumewhitechars();
		displaymode=getint();

		if((displaymode>4)||(displaymode==0))
		{
			cdcprintf("\r\nIllegal displaymode!\r\n");
			modeConfig.error=1;
		}
	}	

	modeConfig.displaymode=displaymode-1;
}

// displays the help
void printhelp(void)
{
	cdcprintf(" General\t\t\t\t\tProtocol interaction\r\n");
	cdcprintf(" ---------------------------------------------------------------------------\r\n");
	cdcprintf(" \t\t\t\t\t(0)\tList current macros\r\n");
	cdcprintf(" =X/|X\tConverts X/reverse X\t\t(x)\tMacro x\r\n");
	cdcprintf(" ~\tSelftest\t\t\t[\tStart\r\n");
	cdcprintf(" #\tReset the BP   \t\t\t]\tStop\r\n");
	cdcprintf(" $\tJump to bootloader\t\t{\tStart with read\r\n");
	cdcprintf(" &/%%\tDelay 1 us/ms\t\t\t}\tStop\r\n");
	cdcprintf(" a/A/@\tAUXPIN (low/HI/READ)\t\t\"abc\"\tSend string\r\n");
	cdcprintf(" b\tSet vpumode\t\t\t123\r\n");
	cdcprintf(" c/C\tAUX assignment (aux/CS)\t\t0x123\r\n");
	cdcprintf(" d/D\tMeasure ADC (once/CONT.)\t0b110\tSend value\r\n");
	cdcprintf(" f\tMeasure frequency\t\tr\tRead\r\n");
	cdcprintf(" g/S\tGenerate PWM/Servo\t\t/\tCLK hi\r\n");
	cdcprintf(" h/H/?\tHelp (general/PROTOCOL)\t\\\tCLK lo\r\n");
	cdcprintf(" i\tVersioninfo/statusinfo\t\t^\tCLK tick\r\n");
	cdcprintf(" l/L\tBitorder (msb/LSB)\t\t-\tDAT hi\r\n");
	cdcprintf(" m\tChange mode\t\t\t_\tDAT lo\r\n");
	cdcprintf(" o\tSet output type\t\t\t.\tDAT read\r\n");
	cdcprintf(" p/P\tPullup resistors (off/ON)\t!\tBit read\r\n");
	cdcprintf(" s\tScript engine\t\t\t:\tRepeat e.g. r:10\r\n");
	cdcprintf(" v\tShow volts/states\t\t.\tBits to read/write e.g. 0x55.2\r\n");
	cdcprintf(" w/W\tPSU (off/ON)\t\t<x>/<x= >/<0>\tUsermacro x/assign x/list all\r\n");
}

// copies an previous cmd to current position int cmdbuff
int cmdhistory(int ptr)
{
	int i;
	uint32_t temp;
	
	i=1;

	for (temp=(cmdtail-2)&(CMDBUFFSIZE-1); temp!=cmdhead; temp=(temp-1)&(CMDBUFFSIZE-1))
	{
		if(!cmdbuff[temp]) ptr--;

		if((ptr==0)&&(cmdbuff[(temp+1)&(CMDBUFFSIZE-1)]))		// do we want this one?
		{
			while(cursor!=((cmdhead)&(CMDBUFFSIZE-1)))			//clear line to end
			{
				cdcputc(' ');
				cursor=(cursor+1)&(CMDBUFFSIZE-1);
			}
			while(cursor!=((cmdtail)&(CMDBUFFSIZE-1)))			//move back to start;
			{
				cdcprintf("\x1B[D \x1B[D");
				cursor=(cursor-1)&(CMDBUFFSIZE-1);
			}


			while(cmdbuff[(temp+i)&(CMDBUFFSIZE-1)])
			{
				cmdbuff[(cmdtail+i-1)&(CMDBUFFSIZE-1)]=cmdbuff[(temp+i)&(CMDBUFFSIZE-1)];
				cdcputc(cmdbuff[(temp+i)&(CMDBUFFSIZE-1)]);
				i++;
			}
			cmdhead=(cmdtail+i-1)&(CMDBUFFSIZE-1);
			cursor=cmdhead;
			cmdbuff[cmdhead]=0x00;
			break;
		}
	}

	return (!ptr);
}


// handles the userinput 
void getuserinput(void)
{
	int go, histptr;
	char c;
	uint32_t temp;

	go=0;
	cursor=cmdhead;
	histptr=0;

	while(!go)
	{
		if(cdcbyteready())
		{
			c=cdcgetc();
		
			switch(c)
			{
				case 0x08:							// backspace
						if((cmdhead!=cmdtail)&&(cursor!=cmdtail))	// not empty or at beginning?
						{
							if(cursor==cmdhead)			// at end?
							{
								cmdhead=(cmdhead-1)&(CMDBUFFSIZE-1);
								cursor=cmdhead;
								cdcputs("\x08 \x08");
								cmdbuff[cmdhead]=0x00;
							}
							else
							{
								temp=cursor;
								cdcprintf("\x1B[D");
								while(temp!=cmdhead)
								{
									cmdbuff[((temp-1)&(CMDBUFFSIZE-1))]=cmdbuff[temp];
									cdcputc(cmdbuff[temp]);
									temp=(temp+1)&(CMDBUFFSIZE-1);
								}
								cdcputc(' ');
								cmdbuff[cmdhead]=0x00;
								cmdhead=(cmdhead-1)&(CMDBUFFSIZE-1);
								cursor=(cursor-1)&(CMDBUFFSIZE-1);
								cdcprintf("\x1B[%dD", ((cmdhead-cursor+1)&(CMDBUFFSIZE-1)));
							}
						}
						else cdcputc('\x07');
						break;
				case '\r':	cmdbuff[cmdhead]=0x00;
						cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
						go=1;
						break;
				case '\x1B':	c=cdcgetc();
						if(c=='[')
						{
							c=cdcgetc();
							switch(c)
							{
								case 'D':	if(cursor!=cmdtail)	// left
										{
											cursor=(cursor-1)&(CMDBUFFSIZE-1);
											cdcprintf("\x1B[D");
										}
										else cdcputc('\x07');

										break;
								case 'C':	if(cursor!=cmdhead)	// right
										{
											cursor=(cursor+1)&(CMDBUFFSIZE-1);
											cdcprintf("\x1B[C");
										}
										else cdcputc('\x07');

										break;
								case 'A':	// up
										histptr++;
										if(!cmdhistory(histptr))	// on error restore ptr and ring a bell
										{
											histptr--;
											cdcputc('\x07');
										}
										break;
								case 'B':	// down
										histptr--;
										if((histptr<1)||(!cmdhistory(histptr)))
										{
											histptr=0;
											cdcputc('\x07');
										}
										break;
								default: 	break;
							}

						}
						break;
				default:	if((((cmdhead+1)&(CMDBUFFSIZE-1))!=cmdtail)&&(c>=0x20)&&(c<=0x7E))	// only accept printable characters if room available
						{
							if(cursor==cmdhead)		// at end
							{
								cdcputc(c);
								cmdbuff[cmdhead]=c;
								cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
								cursor=cmdhead;
							}
							else
							{
								temp=cmdhead+1;
								while(temp!=cursor)
								{
									cmdbuff[temp]=cmdbuff[(temp-1)&(CMDBUFFSIZE-1)];
									temp=(temp-1)&(CMDBUFFSIZE-1);
								}
								cmdbuff[cursor]=c;
								temp=cursor;
								while(temp!=((cmdhead+1)&(CMDBUFFSIZE-1)))
								{
									cdcputc(cmdbuff[temp]);
									temp=(temp+1)&(CMDBUFFSIZE-1);
								}
								cursor=(cursor+1)&(CMDBUFFSIZE-1);
								cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
								cdcprintf("\x1B[%dD", ((cmdhead-cursor)&(CMDBUFFSIZE-1)));
							}
						}
						else cdcputc('\x07');
						break;	
			}
		}

		if(cdcbyteready2())
		{
			SUMPlogicCommand(cdcgetc2());
		}
		//SUMPlogicService();
	}
}


// keep the user asking the menu until it falls between minval and maxval, enter returns the default value
uint32_t askint(const char *menu, uint32_t minval, uint32_t maxval, uint32_t defval)
{
	uint32_t temp;
	uint8_t	done;

	done=0;

	while(!done)
	{
		cdcprintf(menu);

		cmdtail=cmdhead;	// flush all input
		getuserinput();
		consumewhitechars();
		temp=getint();

		if(temp==0)		// assume user pressed enter
		{
			temp=defval;
			done=1;
		}
		else if((temp>=minval)&&(temp<=maxval))
			done=1;
		else
		{
			cdcprintf("\x07");
			done=0;
		}
	}

	// clear errors
	modeConfig.error=0;

	return temp;
}

// get the repeat from the commandline (if any) XX:repeat
uint32_t getrepeat(void)
{
	uint32_t tail, repeat;

	repeat=1;				// do at least one round :D
	
	tail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance one
	if(tail!=cmdhead)			// did we reach the end?
	{
		if(cmdbuff[tail]==':')		// we have a repeat \o/
		{
			cmdtail=(cmdtail+2)&(CMDBUFFSIZE-1);
			repeat=getint();
		}
	}
	return repeat;
}

// get the number of bits from the commandline (if any) XXX.numbit
uint32_t getnumbits(void)
{
	uint32_t tail, numbits;

	numbits=0;
	
	tail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance one
	if(tail!=cmdhead)			// did we reach the end?
	{
		if(cmdbuff[tail]=='.')		// we have a change in bits \o/
		{
			cmdtail=(cmdtail+2)&(CMDBUFFSIZE-1);
			numbits=getint();
		}
	}
	return numbits;
}

// represent d in the current display mode. If numbits=8 also display the ascii representation 
void printnum(uint32_t d)
{
	uint32_t mask, i;

	if (modeConfig.numbits<32) mask=((1<<modeConfig.numbits)-1);
	else mask=0xFFFFFFFF;
	d&=mask;

	switch(modeConfig.displaymode)
	{
		case 0:	if(modeConfig.numbits<=8)
				cdcprintf("%3u", d);
			else if(modeConfig.numbits<=16)
				cdcprintf("%5u", d);
			else if(modeConfig.numbits<=24)
				cdcprintf("%8u", d);
			else if(modeConfig.numbits<=32)
				cdcprintf("%10u", d);
			break;
		case 1:	if(modeConfig.numbits<=8)
				cdcprintf("0x%02X", d);
			else if(modeConfig.numbits<=16)
				cdcprintf("0x%04X", d);
			else if(modeConfig.numbits<=24)
				cdcprintf("0x%06X", d);
			else if(modeConfig.numbits<=32)
				cdcprintf("0x%08X", d);
			break;
		case 2:	if(modeConfig.numbits<=6)
				cdcprintf("0%02o", d);
			else if(modeConfig.numbits<=12)
				cdcprintf("0%04o", d);
			else if(modeConfig.numbits<=18)
				cdcprintf("0%06o", d);
			else if(modeConfig.numbits<=24)
				cdcprintf("0%08o", d);
			else if(modeConfig.numbits<=30)
				cdcprintf("0%010o", d);
			else if(modeConfig.numbits<=32)
				cdcprintf("0%012o", d);
			break;
		case 3:	cdcprintf("0b");
			for(i=0; i<modeConfig.numbits; i++)
			{
				mask=1<<(modeConfig.numbits-i-1);
				if(d&mask)
					cdcprintf("1");
				else
					cdcprintf("0");
			}
			break;
	}
	if(modeConfig.numbits!=8) cdcprintf(".%d", modeConfig.numbits);
		else cdcprintf(" (\'%c\')", ((d>=0x20)&&(d<0x7E)?d:0x20));
}

void initdelay(void)
{
	rcc_periph_clock_enable(BP_DELAYTIMER_CLOCK);

	TIM_CNT(BP_DELAYTIMER) = 0;
	TIM_PSC(BP_DELAYTIMER) = 72;
	TIM_ARR(BP_DELAYTIMER) = 65535;
	TIM_CR1(BP_DELAYTIMER) |= TIM_CR1_CEN;
}

// delays num ms
void delayms(uint32_t num)
{
	while(num)
	{
		delayus(1000);
		num--;
	}
}


// delay num us
void delayus(uint32_t num)
{
	TIM_CNT(BP_DELAYTIMER)=0;
	while(TIM_CNT(BP_DELAYTIMER)<=num);

}

// order bits according to lsb/msb setting
uint32_t orderbits(uint32_t d)
{
	uint32_t result, mask;
	int i;

	if(!modeConfig.bitorder)			// 0=MSB
		return d;
	else
	{
		mask=0x80000000;
		result=0;	

		for(i=0; i<32; i++)
		{
			if(d&mask)
			{
				result|=(1<<(i));	
			}
			mask>>=1;
		}

		return result>>(32-modeConfig.numbits);
	}
}

#define BOOT_MAGIC_VALUE	0xB007

void jumptobootloader(void)
{
	rcc_periph_clock_enable(RCC_PWR);
	rcc_periph_clock_enable(RCC_BKP);
	pwr_disable_backup_domain_write_protect();
	BKP_DR1=BOOT_MAGIC_VALUE;
	pwr_enable_backup_domain_write_protect();
	rcc_periph_clock_disable(RCC_PWR);
	rcc_periph_clock_enable(RCC_BKP);

	SCB_AIRCR=0x0004|(0x5Fa<<16);
	while(1);
}

void reset(void)
{
	SCB_AIRCR=0x0004|(0x5Fa<<16);
	while(1);
}



