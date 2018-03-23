#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "buspirateNG.h"
#include "cdcacm.h"
#include "UI.h"
#include "sump.h"
#include "LA.h"

//
// SUMP LA
//


//commandset
//http://www.sump.org/projects/analyzer/protocol/
#define SUMP_RESET 	0x00
#define SUMP_RUN	0x01
#define SUMP_ID		0x02
#define SUMP_DESC	0x04
#define SUMP_XON	0x11
#define SUMP_XOFF 	0x13
#define SUMP_DIV 	0x80
#define SUMP_CNT	0x81
#define SUMP_FLAGS	0x82
#define SUMP_TRIG	0xc0
#define SUMP_TRIG_VALS 0xc1

static enum _LAstate {
	LA_IDLE = 0,
	LA_ARMED,
} LAstate = LA_IDLE;

#define LA_SAMPLE_SIZE 4096
static unsigned char sumpPadBytes;
static unsigned int sumpSamples;


void SUMPreset(void){
	//default speed and samples
	//setup PR register
	sumpSamples=LA_SAMPLE_SIZE;
	sumpPadBytes=0;
	LAstate=LA_IDLE;
}

void SUMPlogicCommand(unsigned char inByte){
	static unsigned long l;

	static enum _SUMP {
		C_IDLE = 0,
		C_PARAMETERS,
		C_PROCESS,
	} sumpRXstate = C_IDLE;

	static struct _sumpRX {
		unsigned char command[5];
		unsigned char parameters;
		unsigned char parCnt;
	} sumpRX;

	switch(sumpRXstate){ //this is a state machine that grabs the incoming commands one byte at a time

		case C_IDLE:

			switch(inByte){//switch on the current byte
				case SUMP_RESET://reset
					SUMPreset();
					break;
				case SUMP_ID://SLA0 or 1 backwards: 1ALS
					cdcprintf2("1ALS");
					break;
				case SUMP_RUN://arm the triger
					LAstate=LA_ARMED;
					logicAnalyzerDumpSamples(sumpSamples);
					SUMPreset();
					break;
				case SUMP_DESC:
					// device name string
					cdcputc2(0x01);
					cdcprintf2("BPv3");
					cdcputc2(0x00);
					//sample memory (4096)
					cdcputc2(0x21);
					cdcputc2(0x00);
					cdcputc2(0x00);
					cdcputc2(0x10);
					cdcputc2(0x00);
					//sample rate (1MHz)
					cdcputc2(0x23);
					cdcputc2(0x00);
					cdcputc2(0x0F);
					cdcputc2(0x42);
					cdcputc2(0x40);
					//number of probes (5)
					cdcputc2(0x40);
					cdcputc2(0x07);
					//protocol version (2)
					cdcputc2(0x41);
					cdcputc2(0x02);
					cdcputc2(0x00);
					break;
				case SUMP_XON://resume send data
				case SUMP_XOFF://pause send data
					break;
				default://long command
					sumpRX.command[0]=inByte;//store first command byte
					sumpRX.parameters=4; //all long commands are 5 bytes, get 4 parameters
					sumpRX.parCnt=0;//reset the parameter counter
					sumpRXstate=C_PARAMETERS;
					break;
			}
			break;
		case C_PARAMETERS: 
			sumpRX.parCnt++;
			sumpRX.command[sumpRX.parCnt]=inByte;//store each parameter
			if(sumpRX.parCnt<sumpRX.parameters) break; //if not all parameters, quit
		case C_PROCESS: //ignore all long commands for now
			switch(sumpRX.command[0]){

				case SUMP_TRIG: //set CN on these pins
					//if(sumpRX.command[1] & 0b10000)	CNEN2|=0b1; //AUX
					//if(sumpRX.command[1] & 0b1000)  CNEN2|=0b100000;
					//if(sumpRX.command[1] & 0b100)   CNEN2|=0b1000000;
					//if(sumpRX.command[1] & 0b10)  	CNEN2|=0b10000000;
					//if(sumpRX.command[1] & 0b1) 	CNEN2|=0b100000000;
/*
				case SUMP_FLAGS:
					sumpPadBytes=0;//if user forgot to uncheck chan groups 2,3,4, we can send padding bytes
					if(sumpRX.command[1] & 0b100000) sumpPadBytes++;
					if(sumpRX.command[1] & 0b10000) sumpPadBytes++;
					if(sumpRX.command[1] & 0b1000) sumpPadBytes++;
					break;
*/
				case SUMP_CNT:
					sumpSamples=sumpRX.command[2];
					sumpSamples<<=8;
					sumpSamples|=sumpRX.command[1];
					sumpSamples=(sumpSamples+1)*4;
					//prevent buffer overruns
					if(sumpSamples>LA_SAMPLE_SIZE) sumpSamples=LA_SAMPLE_SIZE;
					break;
				case SUMP_DIV:
					l=sumpRX.command[3];
					l<<=8;
					l|=sumpRX.command[2];
					l<<=8;
					l|=sumpRX.command[1];

					//convert from SUMP 100MHz clock to our 16MIPs
					//l=((l+1)*16)/100;
					l=((l+1)*4)/25; 

					//adjust downwards a bit
					if(l>0x10)
						l-=0x10;
					else //fast as possible
						l=1;

					//setup PR register
					//PR5=(l>>16);//most significant word
					//PR4=l;//least significant word
					break;
			}

			sumpRXstate=C_IDLE;
			break;					
		}

}

//
//
//	To avoid rewriting interrupt vectors with the bootloader,
//  this firmware currently uses polling to read the trigger and timer
// 	A final version should use interrupts after lots of testing.
//
//
void SUMPlogicService(void){
	//static unsigned int i;
//	static unsigned char j;

	switch(LAstate){//dump data
		case LA_ARMED: //check interrupt flags
			//if(IFS1bits.CNIF==0){//no flags
			//	if(CNEN2) //if no trigger just continue
					break;
			//}
	
			//else sample
			//T4CONbits.TON=1;//start timer4
			//IFS1bits.T5IF=0;//clear interrupt flag//setup timer and wait

			/*for(i=0;i<sumpSamples;i++){ //take SAMPLE_SIZE samples
				//bpConfig.terminalInput[i]=(PORTB>>6); //change to pointer for faster use...
				//while(IFS1bits.T5IF==0); //wait for timer4 (timer 5 interrupt)
				//IFS1bits.T5IF=0;//clear interrupt flag
				
			}
			
			//CNEN2=0;//change notice off
			//T4CON=0; //stop count

			for(i=sumpSamples; i>0; i--){ //send back to SUMP, backwards
				//UART1TX(bpConfig.terminalInput[(i-1)]);
				//for(j=0; j<sumpPadBytes; j++) UART1TX(0); //add padding if needed
			}*/
			logicAnalyzerDumpSamples(sumpSamples);

			SUMPreset();
			break;
		case LA_IDLE:
			break;
	}
	

}



