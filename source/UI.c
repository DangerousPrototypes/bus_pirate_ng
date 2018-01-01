#include <stdlib.h>
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include "debug.h"
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"
#include "protocols.h"
#include "AUXpin.h"
#include "ADC.h"
#include "bpflash.h"

// globals
uint32_t cmdhead, cmdtail;
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
	modeConfig.mosiport=0;
	modeConfig.mosipin=0;
	modeConfig.misoport=0;
	modeConfig.misopin=0;
	modeConfig.csport=0;
	modeConfig.cspin=0;
	modeConfig.clkport=0;
	modeConfig.clkpin=0;
}

void doUI(void)
{
	int go;
	char c;

	uint32_t temp, temp2, temp3, repeat, received, bits;
	int i;

	go=0;

	while(1)
	{
		getuserinput();

		go=1;

		cdcprintf("\r\n");
		while((go)&&(cmdtail!=cmdhead))
		{
			c=cmdbuff[cmdtail];
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
						cdcprintf("Delaying");
						while(repeat--)
						{
							cdcprintf(".");
							delayms(1000);
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
				case 'b':	temp=askint(VPUMENU, 1, 3, 1);
						modeConfig.vpumode=temp-1;
						switch(modeConfig.vpumode)
						{
							case 1:
								gpio_clear(BPVPU50ENPORT, BPVPU50ENPIN);
								gpio_set(BPVPU33ENPORT, BPVPU33ENPIN);
							case 2:
								gpio_clear(BPVPU33ENPORT, BPVPU33ENPIN);
								gpio_set(BPVPU50ENPORT, BPVPU50ENPIN);
							default:
								gpio_clear(BPVPU50ENPORT, BPVPU50ENPIN);
								gpio_clear(BPVPU33ENPORT, BPVPU33ENPIN);
						}
						break;
				case 'd':	if(modeConfig.mode!=HIZ)
						{
							cdcprintf("ADC=%0.2fV", voltage(BPADCCHAN, 1));
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
								cdcprintf("ADC=%0.2fV\r", voltage(BPADCCHAN, 1));
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
				case 'p':	gpio_clear(BPVPUENPORT, BPVPUENPIN);	// always permitted 
						cdcprintf("pullups: disabled");
						modeConfig.pullups=0;
						break;
				case 'P':	if(modeConfig.mode!=0)		// reset vpu mode to externale??
						{
							gpio_set(BPVPUENPORT, BPVPUENPIN);
							cdcprintf("pullups: enabled\r\n");
							delayms(10);
							cdcprintf("Vpu=%0.2fV (mode=%s)", voltage(BPVPUCHAN, 1), vpumodes[modeConfig.vpumode]);
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
				case 'w':	gpio_clear(BPPSUENPORT, BPPSUENPIN);	// always permitted to shut power off
						cdcprintf("PSU: disabled");
						modeConfig.psu=0;
						break;
				case 'W':	if(modeConfig.mode!=0)
						{
							gpio_set(BPPSUENPORT, BPPSUENPIN); 
							cdcprintf("PSU: enabled\r\n");
							delayms(10);
							cdcprintf("V33=%0.2fV, V50=%0.2fV", voltage(BP3V3CHAN, 0), voltage(BP5V0CHAN, 1)); 
							if((voltage(BP3V3CHAN, 0)<3.1)||(voltage(BP5V0CHAN, 1)<4.8))
							{
								cdcprintf("\r\nShort circuit!");
								modeConfig.error=1;
								gpio_clear(BPPSUENPORT, BPPSUENPIN);
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
				case ' ':	break;
				case ',':	break;	// reuse this command??
				case '$':	displayps();
						break;
				case '#':	fillps();
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
				default:	cdcprintf("Unknown command: %c", c);
						modeConfig.error=1;
						//go=0;
						//cmdtail=cmdhead-1;
						break;	
			}
			cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance to next char/command
			if(c!=' ') cdcprintf("\r\n");

			if(modeConfig.error)			// something went wrong
			{
				cdcprintf("\x07");		// bell!
				go=0;
				cmdtail=cmdhead;
				modeConfig.error=0;
			}
		}
		go=0;
		cdcprintf("%s> ", protocols[modeConfig.mode].protocol_name);
	}
}


void versioninfo(void)
{
	int i;
	uint32_t *id = (uint32_t *)0x1FFFF7E8;
	uint16_t flashsize = *(uint16_t *) 0x1FFFF7E0;
	uint16_t ramsize=96;
	uint32_t pwmperiod, pwmoc;

	if(flashsize<=16) ramsize=6;
	else if(flashsize<=32) ramsize=10;
	else if(flashsize<=128) ramsize=20;
	else if(flashsize<=512) ramsize=64;
	else ramsize=96;

	cdcprintf("Buspirate NextGen (ARM)\r\n");
	cdcprintf("Firmware %s, bootloader N/A\r\n", FWVER);
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
			pwmperiod=TIM_ARR(BPPWMTIMER);
#if(1)	// (BPPWMCHAN==TIM_OC1)	
			pwmoc=TIM_CCR1(BPPWMTIMER);
#endif
#if(0)	// (BPPWMCHAN==TIM_OC2)	
			pwmoc=TIM_CCR2(BPPWMTIMER);
#endif
#if(0)	// (BPPWMCHAN==TIM_OC3)	
			pwmoc=TIM_CCR3(BPPWMTIMER);
#endif
#if(0)	// (BPPWMCHAN==TIM_OC4)	
			pwmoc=TIM_CCR4(BPPWMTIMER);
#endif
			cdcprintf("PWM clock %d Hz, dutycycle %0.2f\r\n", (36000000/pwmperiod), (float)((pwmoc*1.0)/pwmperiod));
		}
		showstates();
	}
	
}

void showstates(void)
{
	uint8_t auxstate, csstate, misostate, clkstate, mosistate;
	float v50, v33, vpu, adc;

	cdcprintf("1.GND\t2.+5v\t3.+3V3\t4.Vpu\t5.ADC\t6.AUX\t7.CS\t8.MISO\t9.CLK\t10.MOSI\r\n");
	cdcprintf("GND\t+5v\t+3V3\tVpu\tADC\tAUX\t");
	protocols[modeConfig.mode].protocol_pins();
	cdcprintf("\r\n");
	cdcprintf("PWR\tPWR\tPWR\tPWR\t2.5V\t1\t1\t1\t0\t1\r\n");

	// pinstates
	auxstate=(gpio_get(BPAUXPORT, BPAUXPIN)?1:0);
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
	v33=voltage(BP3V3CHAN, 0);
	v50=voltage(BP5V0CHAN, 1);
	vpu=voltage(BPVPUCHAN, 1);
	adc=voltage(BPADCCHAN, 1);

	//TODO adc/pinstate shit
	cdcprintf("GND\t%0.2fV\t%0.2fV\t%0.2fV\t%0.2fV\t%d\t%d\t%d\t%d\t%d\r\n", v50, v33, vpu, adc, auxstate, csstate, misostate, clkstate, mosistate);


}

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
	protocols[modeConfig.mode].protocol_setup_exc();
}

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


void printhelp(void)
{
	cdcprintf(" General\t\t\t\t\tProtocol interaction\r\n");
	cdcprintf(" ---------------------------------------------------------------------------\r\n");
	cdcprintf(" ?\tThis help\t\t\t(0)\tList current macros\r\n");
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
	cdcprintf(" h\tCommandhistory\t\t\t\\\tCLK lo\r\n");
	cdcprintf(" i\tVersioninfo/statusinfo\t\t^\tCLK tick\r\n");
	cdcprintf(" l/L\tBitorder (msb/LSB)\t\t-\tDAT hi\r\n");
	cdcprintf(" m\tChange mode\t\t\t_\tDAT lo\r\n");
	cdcprintf(" o\tSet output type\t\t\t.\tDAT read\r\n");
	cdcprintf(" p/P\tPullup resistors (off/ON)\t!\tBit read\r\n");
	cdcprintf(" s\tScript engine\t\t\t:\tRepeat e.g. r:10\r\n");
	cdcprintf(" v\tShow volts/states\t\t.\tBits to read/write e.g. 0x55.2\r\n");
	cdcprintf(" w/W\tPSU (off/ON)\t\t<x>/<x= >/<0>\tUsermacro x/assign x/list all\r\n");
}

void getuserinput(void)
{
	int go;
	char c;

	go=0;
	while(!go)
	{
		c=cdcgetc();
		
		switch(c)
		{
			case 0x08:			// delete
					if(cmdhead!=cmdtail)
					{
						cmdhead=(cmdhead-1)&(CMDBUFFSIZE-1);
						cdcputs("\x08 \x08");
					}
					break;
			case '\r':	//cmdbuff[cmdhead]=0x00;
					//cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
					go=1;
					break;
			default:	
					if((c>=0x20)&&(c<=0x7E))	// only accept printable characters
					{
						cdcputc(c);
						cmdbuff[cmdhead]=c;
						cmdhead=(cmdhead+1)&(CMDBUFFSIZE-1);
					}
					break;	
		}
	}
}

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
		else cdcprintf(" (\'%c\')", d);
}

void delayms(uint32_t num)
{
	num*=1000; 		// convert to us
	num/=10;

	num+=systicks;		// should wrap ol

	while(systicks!=num);	
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







