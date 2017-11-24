#include <stdlib.h>
#include <stdint.h>
#if(0)
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#endif
#include "debug.h"
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"
#include "protocols.h"

// globals
int cmdhead, cmdtail;
char cmdbuff[CMDBUFFSIZE];
struct _modeConfig modeConfig;

// global constants
const char vpumodes[][4] = {
"EXT\0",
"5V0\0",
"3V3\0"
};

const char bitorders[][4] ={
"msb\0",
"lsb\0"
};

const char states[][4] ={
"ON\0\0",
"OFF\0"
};



// eats up the spaces and comma's from the cmdline
void consumewhitechars(void)
{
	while((cmdbuff[cmdtail]==' ')||(cmdbuff[cmdtail]==','))
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
		else									// ?! no good
		{

		}
	}
	else
	{
		// no good
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
	modeConfig.hiz=1;
	modeConfig.mode=0;
	modeConfig.pullups=0;
	modeConfig.vpumode=0;
	modeConfig.psu=0;
	modeConfig.bitorder=0;
}

void doUI(void)
{
	int go;
	char c;

	uint32_t temp;

	go=0;

	while(1)
	{
		if(cdcbyteready())
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
				case '\r':			// enter
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

		if(go)
		{
			cdcprintf("\r\n");
			while((go)&&(cmdtail!=cmdhead))
			{
				c=cmdbuff[cmdtail];
				switch (c)
				{
					case '[':	protocols[modeConfig.mode].protocol_start();
							break;
					case ']':	protocols[modeConfig.mode].protocol_stop();
							break;
					case '{':	protocols[modeConfig.mode].protocol_startR();
							break;
					case '}':	protocols[modeConfig.mode].protocol_stopR();
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
					case 'h':
					case '?':	printhelp();
							break;
					case 'i':	versioninfo();
							break;
					case 'm':	changemode();
							break;
					case 'r':	temp=protocols[modeConfig.mode].protocol_read();
							cdcprintf("RX: 0x%08X", temp);
							break;
					case 'v':	showstates();
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
							cdcprintf("TX: 0x%08X", temp);
							protocols[modeConfig.mode].protocol_send(temp);
							break;

					case ' ':
					case ',':	break;
					default:	cdcprintf("Unknown command: %c", c);
							go=0;
							cmdtail=cmdhead-1;
							break;	
				}
				cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// advance to next char/command
				if(c!=' ') cdcprintf("\r\n");

// TODO input error
			}
			go=0;
			cdcprintf("%s> ", protocols[modeConfig.mode].protocol_name);
		}



	}
}


void versioninfo(void)
{
	int i;
	uint32_t *id = (uint32_t *)0x1FFFF7E8;
	uint16_t flashsize = *(uint16_t *) 0x1FFFF7E0;
	uint16_t ramsize=96;

	if(flashsize<=16) ramsize=6;
	else if(flashsize<=32) ramsize=10;
	else if(flashsize<=128) ramsize=20;
	else if(flashsize<=512) ramsize=64;
	else ramsize=96;

	cdcprintf("Buspirate NextGen (ARM)\r\n");
	cdcprintf("Firmware %s, bootloader %s\r\n", FWVER, BLVER);
	cdcprintf("STM32 with %dK FLASH, %dK SRAM ", flashsize, ramsize);
	cdcprintf("s/n: %08X%08X%08X\r\n", id[0], id[1], id[2]);
	cdcprintf("https://dangerousprototypes.com/\r\n");

	cdcprintf("Available busprotocols:");

	for(i=0; i<MAXPROTO; i++)
	{
		cdcprintf(" %s", protocols[i].protocol_name);
	}
	cdcprintf("\r\n");

	if(1)	//!modeConfig.hiz)
	{
		protocols[modeConfig.mode].protocol_settings();
		cdcprintf("\r\n");
		cdcprintf("#bits: %d, ",modeConfig.numbits);
		cdcprintf("bitorder: %s, ", bitorders[modeConfig.bitorder]);
		cdcprintf("PU: %s, ", states[modeConfig.pullups]);
		cdcprintf("Vpu mode: %s, ", vpumodes[modeConfig.vpumode]);
		cdcprintf("Power: %s\r\n", states[modeConfig.psu]);
		showstates();
	}
	
}

void showstates(void)
{
	cdcprintf("1.GND\t2.+5v\t3.+3V3\t4.Vpu\t5.ADC\t6.AUX\t7.CS\t8.MISO\t9.CLK\t10.MOSI\r\n");
	cdcprintf("GND\t+5v\t+3V3\tVpu\tADC\tAUX\t");
	protocols[modeConfig.mode].protocol_pins();
	cdcprintf("\r\n");
	cdcprintf("PWR\tPWR\tPWR\tPWR\t2.5V\t1\t1\t1\t0\t1\r\n");

	//TODO adc/pin shit
	cdcprintf("GND\t5.0V\t3.3V\t3.3V\t2.5V\t1\t1\t1\t0\t1\r\n");


}

void changemode(void)
{
	uint32_t mode;
	int i;

	
	cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);	// pointer is to 'm' we should advance
	consumewhitechars();
	mode=getint();

	if(mode>MAXPROTO)
	{
		cdcprintf("\r\nIllegal mode!");
	}
	else if(mode==0)
	{
		for(i=0; i<MAXPROTO; i++)
			cdcprintf(" %d. %s\r\n", i+1, protocols[i].protocol_name);
	}
	else
	{
		protocols[modeConfig.mode].protocol_cleanup();		// switch to HiZ
		modeConfig.mode=mode-1;

		protocols[modeConfig.mode].protocol_setup();		// setup the new mode
		protocols[modeConfig.mode].protocol_setup_exc();
	}


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
	cdcprintf(" &/%\tDelay 1 us/ms\t\t\t}\tStop\r\n");
	cdcprintf(" a/A/@\tAUXPIN (low/HI/READ)\t\t\"abc\"\tSend string\r\n");
	cdcprintf(" b\tSet baudrate\t\t\t123\r\n");
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

	
