

#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"
#include "1WIRE.h"
#include "cdcacm.h"
#include "UI.h"


//the roster stores the first OW_DEV_ROSTER_SLOTS 1-wire addresses found during a ROM SEARCH command
//these addresses are available as MACROs for quick address entry
#define OW_DEV_ROSTER_SLOTS 10 //how many devices max to store addresses as MACROs
struct _OWID{
//      unsigned char familyID; //to lazy to do it right, for now...
        unsigned char id[8];
//      unsigned char crc;
};

struct _OWIDREG{
        unsigned char num;
        struct _OWID dev[OW_DEV_ROSTER_SLOTS];
} ;

struct _OWIDREG OWroster;


#define TRUE	1
#define FALSE	0

//because 1wire uses bit times, setting the data line high or low with (_-) has no effect
//we have to save the desired bus state, and then clock in the proper value during a clock(^)
static unsigned char DS1wireDataState=0;//data bits are low by default.

// global search state,
//these lovely globals are provided courtesy of MAXIM's code
//need to be put in a struct....
unsigned char ROM_NO[8];
unsigned char SearchChar=0xf0; //toggle between ROM and ALARM search types
unsigned char LastDiscrepancy;
unsigned char LastFamilyDiscrepancy;
unsigned char LastDeviceFlag;
unsigned char crc8;



void ONEWIRE_start(void)
{
	cdcprintf("ONEWIRE start()");
}
void ONEWIRE_startr(void)
{
	cdcprintf("ONEWIRE startr()");
}
void ONEWIRE_stop(void)
{
	cdcprintf("ONEWIRE stop()");
}
void ONEWIRE_stopr(void)
{
	cdcprintf("ONEWIRE stopr()");
}
uint32_t ONEWIRE_send(uint32_t d)
{
	OWWriteByte(d);

	return 0;
}
uint32_t ONEWIRE_read(void)
{
	return (OWReadByte());
}
void ONEWIRE_clkh(void)
{
	cdcprintf("ONEWIRE clkh()");
}
void ONEWIRE_clkl(void)
{
	cdcprintf("ONEWIRE clkl()");
}
void ONEWIRE_dath(void)
{
        DS1wireDataState=1;
	cdcprintf(" *next clock (^) will use this value\r\n");
}
void ONEWIRE_datl(void)
{
	DS1wireDataState=0;
	cdcprintf(" *next clock (^) will use this value\r\n");
}
uint32_t ONEWIRE_dats(void)
{
	return DS1wireDataState;
}
void ONEWIRE_clk(void)
{
	OWWriteBit(DS1wireDataState);
}
uint32_t ONEWIRE_bitr(void)
{
	return(OWReadBit());
}
uint32_t ONEWIRE_period(void)
{
	return 0;
}
void ONEWIRE_macro(uint32_t macro)
{
	unsigned char c,j;
        unsigned int i;
        unsigned char devID[8];

        if(macro>0 && macro<51)
	{
                macro--;							//adjust down one for roster array index
                if(macro>=OWroster.num)
		{								//no device #X on the bus, try ROM SEARCH (0xF0)
                        cdcprintf("No device, try (ALARM) SEARCH macro first\r\n");
                        return;
                }
                								//write out the address of the device in the macro
                //bpWstring(OUMSG_1W_MACRO_ADDRESS);//xxx WRITE BUS #X ID:
                //BPMSG1005;
		cdcprintf("ADDRESS MACRO %d: ", macro+1);
                //bpWdec(macro+1);
                //bpWstring(": ");
                for(j=0;j<8;j++)
		{
                        //bpWbyte(OWroster.dev[macro].id[j]); 
                        //bpSP; 
			cdcprintf("%02X ", OWroster.dev[macro].id[j]);
                        OWWriteByte(OWroster.dev[macro].id[j]);
                } 								//write address
                cdcprintf("\r\n");
		//bpBR;
                return;
        }
        switch(macro)
	{
                case 0:								//menu
                        //bpWline(OUMSG_1W_MACRO_MENU);
                        //bpWline(OUMSG_1W_MACRO_SEARCH_ROM_HEADER);
                        //BPMSG1006;
                        //BPMSG1007;
			cdcprintf(" 0.Macro menu\r\n");
			cdcprintf("Macro     1WIRE address\r\n");
                        //write out roster of devices and macros, or SEARCH ROM NOT RUN, TRY (0xf0)
                        if(OWroster.num==0)
			{
                                //bpWline(OUMSG_1W_MACRO_ADDRESS_NODEVICE);
                                //BPMSG1004;
				cdcprintf("No device, try (ALARM) SEARCH macro first\r\n");
                        }
			else
			{
                                for(c=0;c<OWroster.num; c++)
				{
                                        //bpSP;//space
                                        //bpWdec(c+1);
                                        //bpWstring(".");
					cdcprintf(" %d.");
                                        for(j=0;j<8;j++) cdcprintf("%02X ", OWroster.dev[c].id[j]);	//{bpWbyte(OWroster.dev[c].id[j]); bpSP;}
                                        //bpWstring("\x0D\x0A   *");
                                        //BPMSG1008;]
					cdcprintf("\r\n   *");
                                        DS1wireID(OWroster.dev[c].id[0]);       //print the device family identity (if known)
                                }
                        }
                        //bpWline(OUMSG_1W_MACRO_MENU_ROM);
                        //BPMSG1009;
			cdcprintf("1WIRE ROM COMMAND MACROs:\r\n");
			cdcprintf(" 51.READ ROM (0x33) *for single device bus\r\n");
			cdcprintf(" 85.MATCH ROM (0x55) *followed by 64bit address\r\n");
			cdcprintf(" 204.SKIP ROM (0xCC) *followed by command\r\n");
			cdcprintf(" 236.ALARM SEARCH (0xEC)\r\n");
			cdcprintf(" 240.SEARCH ROM (0xF0)");
                        break;
                //1WIRE ROM COMMANDS
                case 0xec://ALARM SEARCH
                case 0xf0: //SEARCH ROM
                        SearchChar=macro;
                        if(macro==0xec)
			{
                                //bpWline(OUMSG_1W_MACRO_ALARMSEARCH_ROM);
                                //BPMSG1010;
				cdcprintf("ALARM SEARCH (0xEC)\r\n");
                        }
			else
			{							//SEARCH ROM command...
                                //bpWline(OUMSG_1W_MACRO_SEARCH_ROM);
                                //BPMSG1011;
				cdcprintf("SEARCH (0xF0)\r\n");
                        }

                        //bpWline(OUMSG_1W_MACRO_SEARCH_ROM_HEADER);
                        //BPMSG1007;
			cdcprintf("Macro     1WIRE address");
                        							// find ALL devices
                        j = 0;
                        c = OWFirst();
                        OWroster.num=0;
                        while (c)
			{
                                //the roster number is the shortcut macro
                                //bpSP;
                                //bpWdec(j+1);
                                //bpWstring(".");
				cdcprintf(" %d. ", j);
                
                                // print address
                                for (i = 0; i <8; i++)
				{
                                        //bpWbyte(ROM_NO[i]);
                                        //bpSP;
					cdcprintf("%02X ", ROM_NO[i]);
                                }
                                //bpWstring("\x0D\x0A   *");
                                //BPMSG1008;
				cdcprintf("\r\n   *");
                                DS1wireID(ROM_NO[0]);   			//print the device family identity (if known)
                                
                                //keep the first X number of one wire IDs in a roster
                                //so we can refer to them by macro, rather than ID
                                if(j<OW_DEV_ROSTER_SLOTS)
				{						//only as many as we have room for
                                        for(i=0;i<8;i++) OWroster.dev[OWroster.num].id[i]=ROM_NO[i];
                                        OWroster.num++;				//increment the roster count
                                }

                                j++;    
                
                                c = OWNext();
                        }

                        //bpWline(OUMSG_1W_MACRO_SEARCH_ROM_NOTE);
                        //BPMSG1012;
			cdcprintf("Device IDs are available by MACRO, see (0).\r\n");              
                        break;
                case 0x33://READ ROM
                        DS1wireReset();
                        //bpWstring(OUMSG_1W_MACRO_READ_ROM);
                        //BPMSG1013;
			cdcprintf("READ ROM (0x33): ");
                        OWWriteByte(0x33);
                        for(i=0; i<8; i++)
			{
                                devID[i]=OWReadByte();
                                //bpWbyte(devID[i]);
                                //bpSP;   
				cdcprintf("%02X ", devID[i]);
                        }
                        //bpWBR;  
			cdcprintf("\r\n");
                        DS1wireID(devID[0]);
                        break;
                case 0x55://MATCH ROM
                        DS1wireReset();
                        //bpWline(OUMSG_1W_MACRO_MATCH_ROM);
                        //BPMSG1014;
			cdcprintf("MATCH ROM (0x55)\r\n");
                        OWWriteByte(0x55);
                        break;
                case 0xcc://SKIP ROM
                        DS1wireReset();
                        //bpWline(OUMSG_1W_MACRO_SKIP_ROM);
                        //BPMSG1015;
			cdcprintf("SKIP ROM (0xCC)\r\n");
                        OWWriteByte(0xCC);
                        break;
		case 0xff:
			DS1wireReset();
			break;
		case 0x100:
			gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
			break;
		case 0x101:
			gpio_clear(BP_1WIRE_PORT, BP_1WIRE_PIN);
			break;
                default:
                        //bpWmessage(MSG_ERROR_MACRO);
                        //BPMSG1016;
			cdcprintf("No such macro\r\n");
			modeConfig.error=1;
        }
}
void ONEWIRE_setup(void)
{
	cdcprintf("ONEWIRE setup()");
}
void ONEWIRE_setup_exc(void)
{
	modeConfig.oc=1;	//yes, always opencollector
        OWroster.num=0;		//clear any old 1-wire bus enumeration rosters

	gpio_set_mode(BP_1WIRE_SENSE_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_1WIRE_SENSE_PIN);
	gpio_set_mode(BP_1WIRE_PORT, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, BP_1WIRE_PIN);
	gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
	
}
void ONEWIRE_cleanup(void)
{
	gpio_set_mode(BP_1WIRE_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_1WIRE_PIN);
	gpio_set_mode(BP_1WIRE_SENSE_PORT, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, BP_1WIRE_SENSE_PIN);

}
void ONEWIRE_pins(void)
{
	cdcprintf("pin1\tpin2\tpin3\tpin4");
}
void ONEWIRE_settings(void)
{
	cdcprintf("1WIRE ()=()");
}



/* ***********************************************************************************
   Function: OWReset
   Args[0]: Void
   Return: Made it mimic the old source.
		0 = OK
		1 = Short
		2 = No device.
   
   Desc: OneWire reset bus procedure. See return values for details.
*********************************************************************************** */
unsigned char OWReset(void)
{
	unsigned int Presence=0,i;	

	gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
	//OW_bpResPins(); 				//found out i needed this after all.

	//SDA_TRIS=0;					//go low
	gpio_clear(BP_1WIRE_PORT, BP_1WIRE_PIN);

							//Maxim says a minimum of 480. 
	//for(i=0;i<34;i++) 
	//{						//BP Timing seems off; adjusting it looks like each FOR takes 4us. 
	//	bpDelayUS(10);				//So 10+4=14 ## I want ~490 so (((TIMEWANTED)/(DELAY_TIME)+(ADJUST))=(LOOPS_NEEDED)) OR (490/(10+4))=(35)
	//}						//-- above is old. 35x10=506. So im setting it at 34 which is about 490 (turns out to be 491 :)
	delayus(500);	


	//SDA_TRIS=1;					//release. My logic analyzer says we stayed low for exatly 488us. Thats pretty damn near perfect.
	gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
		
	//bpDelayUS(65);					// ADJUSTED. Timing looks great now with the odd numbers.
	delayus(60);
	//if(SDA)						// if lines still high, then no device
	if(gpio_get(BP_1WIRE_SENSE_PORT, BP_1WIRE_SENSE_PIN))						// if lines still high, then no device
		Presence=2;				// if no device then return 2.
		
	//for(i=0;i<35;i++)
	//{						//delay for 506us (found above) which is way after the 480 reccomended. thats ok.
	//	bpDelayUS(10);
	//}
	delayus(500);
	
	//if(SDA==0)					// if lines still low; then theres a short
	if(gpio_get(BP_1WIRE_SENSE_PORT, BP_1WIRE_SENSE_PIN))			// if lines still low; then theres a short
	{
		return 1;
	}

	return Presence;
}


/* ***********************************************************************************
   Function: OWBit
   Args[0]: uChar [Bit to send. Logic 1 or 0] - or 1 to recieve
   Return: Returns bit value on bus
   
   Desc: OWBit works as both a sending and reciving of 1 bit value on the OWBus.
		 To get a bit value send a logical 1 (OWBit 1) This will pulse the line
		 as needed then release the bus and wait a few US before sampling if the
		 OW device has sent data.
*********************************************************************************** */
unsigned char OWBit(unsigned char OWbit)
{
	gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
	//OW_bpResPins();					// I found not including this leaves the lines odd... :/

	//SDA_TRIS=0;
	gpio_clear(BP_1WIRE_PORT, BP_1WIRE_PIN);

	//bpDelayUS(4);
	delayus(10);
	if(OWbit)
	{
		//SDA_TRIS=1;
		gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
	}
	//bpDelayUS(8);
	delayus(10);

	if(OWbit)
	{						// This is where the magic happens. If a OWbit value of 1 is sent to this function
		//OWbit = SDA;				// well thats the same timing needed to get a value (not just send it) so why not
		OWbit = gpio_get(BP_1WIRE_SENSE_PORT, BP_1WIRE_SENSE_PIN);	// well thats the same timing needed to get a value (not just send it) so why not
    		//bpDelayUS(32);				// perform both? So sending one will not only send 1 bit; it will also read one bit
		delayus(30);
	}
	else
	{						// it all depends on what the iDevice is in the mood to do. If its in send mode then
		//bpDelayUS(25);				// it will sends its data, if its in recive mode. Then we will send ours.
		delayus(20);
		//SDA_TRIS=1;				//    magical, i know. :)
		gpio_set(BP_1WIRE_PORT, BP_1WIRE_PIN);
		//bpDelayUS(7);
		delayus(10);
	}

	//bpDelayUS(5); 					//Just an adjust. 70us perfect.
	//delayus(10);

	return OWbit;
}






/* ***********************************************************************************
   Function: OWByte
   Args[0]: uChar [Byte to send to bus] - or 0xFF to recieve
   Return: Returns a byte from the OWBus
   
   Desc: Like OWBit; OWByte works for both sending and getting. OWTime slots are the same
		 for sending and getting; so only one function is needed to perform both tasks.

*********************************************************************************** */
unsigned char OWByte(unsigned char OWbyte)
{
	unsigned char i, t=0;				// nothing much to say about this function. pretty standard; do this 8 times and collect results.
							// except that sends and GETS data. Sending a value of 0xFF will have the OWBit function return
	for(i=0;i<8;i++)				// bits; this will collect those returns and spit them out. Same with send. It all depends on
	{						// what the iDevice is looking for at the time this command is sent.
		t = OWBit(OWbyte&1);
		OWbyte>>=1;
		if(t) { OWbyte |= 0x80; }
	} 
	//bpDelayUS(8);
	delayus(10);
	return OWbyte;
}



void DS1wireReset(void)
{
        unsigned char c;

        c=OWReset();
        //bpWstring(OUMSG_1W_RESET);
        //BPMSG1017;      
	cdcprintf("BUS RESET ");
        if(c==0)
	{
                //bpWline(OUMSG_1W_RESET_OK);                   
                //BPMSG1018; //remove?
                //BPMSG1185;
		cdcprintf(" OK\r\n");
        }else{
                //bpWstring(OUMSG_1W_RESET_ERROR);
                //BPMSG1019;
		cdcprintf("Warning: ");
//                if(c&0b1)  BPMSG1020;           //bpWstring(OUMSG_1W_RESET_SHORT);      
//                if(c&0b10) BPMSG1021;           //bpWstring(OUMSG_1W_RESET_NODEV);
                if(c&0b1)  cdcprintf("*Short or no pull-up \r\n");           //bpWstring(OUMSG_1W_RESET_SHORT);      
                if(c&0b10) cdcprintf("*No device detected \r\n");           //bpWstring(OUMSG_1W_RESET_NODEV);
                //bpBR;
        }
}

void DS1wireID(unsigned char famID)
{
        //some devices, according to:
        //http://owfs.sourceforge.net/commands.html
        #define DS2404 0x04
        #define DS18S20 0x10
        #define DS1822 0x22
        #define DS18B20 0x28
        #define DS2431 0x2D
        switch(famID)
	{									//check for device type
                case DS18S20:
                        //bpWline("DS18S20 High Pres Dig Therm");
                        //BPMSG1022;
			cdcprintf("DS18S20 High Pres Dig Therm\r\n");
                        break;
                case DS18B20:
                        //bpWline("DS18B20 Prog Res Dig Therm");
                        //BPMSG1023;
			cdcprintf("DS18B20 Prog Res Dig Therm\r\n");
                        break;
                case DS1822:
                        //bpWline("DS1822 Econ Dig Therm");
                        //BPMSG1024;
			cdcprintf("DS1822 Econ Dig Therm\r\n");
                        break;
                case DS2404:
                        //bpWline("DS2404 Econram time Chip");
                        //BPMSG1025;
			cdcprintf("DS2404 Econram time Chip\r\n");
                        break;
                case DS2431:
                        //bpWline("DS2431 1K EEPROM");
                        //BPMSG1026;
			cdcprintf("DS2431 1K EEPROM\r\n");
                        break;
                default:
                        //bpWline("Unknown device");
                        //BPMSG1027;
			cdcprintf("Unknown device\r\n");

        }
}


//the 1-wire search algo taken from:
//http://www.maxim-ic.com/appnotes.cfm/appnote_number/187
//#define TRUE 1 //if !=0
//#define FALSE 0

//--------------------------------------------------------------------------
// Find the 'first' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : no device present
//
unsigned char OWFirst(void)
{
   // reset the search state
   LastDiscrepancy = 0;
   LastDeviceFlag = FALSE;
   LastFamilyDiscrepancy = 0;

   return OWSearch();
}

//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
unsigned char OWNext(void)
{
   // leave the search state alone
   return OWSearch();
}

//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE  : device found, ROM number in ROM_NO buffer
//        FALSE : device not found, end of search
//
unsigned char OWSearch(void)
{
   unsigned char id_bit_number;
   unsigned char last_zero, rom_byte_number, search_result;
   unsigned char id_bit, cmp_id_bit;
   unsigned char rom_byte_mask, search_direction;

   // initialize for search
   id_bit_number = 1;
   last_zero = 0;
   rom_byte_number = 0;
   rom_byte_mask = 1;
   search_result = 0;
   crc8 = 0;

   // if the last call was not the last one
   if (!LastDeviceFlag)
   {
      // 1-Wire reset
      if (OWReset())
      {
         // reset the search
         LastDiscrepancy = 0;
         LastDeviceFlag = FALSE;
         LastFamilyDiscrepancy = 0;
         return FALSE;
      }

      // issue the search command 
      OWWriteByte(SearchChar);  //!!!!!!!!!!!!!!!

      // loop to do the search
      do
      {
         // read a bit and its complement
         id_bit = OWReadBit();
         cmp_id_bit = OWReadBit();

         // check for no devices on 1-wire
         if ((id_bit == 1) && (cmp_id_bit == 1))
            break;
         else
         {
            // all devices coupled have 0 or 1
            if (id_bit != cmp_id_bit)
               search_direction = id_bit;  // bit write value for search
            else
            {
               // if this discrepancy if before the Last Discrepancy
               // on a previous next then pick the same as last time
               if (id_bit_number < LastDiscrepancy)
                  search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
               else
                  // if equal to last pick 1, if not then pick 0
                  search_direction = (id_bit_number == LastDiscrepancy);

               // if 0 was picked then record its position in LastZero
               if (search_direction == 0)
               {
                  last_zero = id_bit_number;

                  // check for Last discrepancy in family
                  if (last_zero < 9)
                     LastFamilyDiscrepancy = last_zero;
               }
            }

            // set or clear the bit in the ROM byte rom_byte_number
            // with mask rom_byte_mask
            if (search_direction == 1)
              ROM_NO[rom_byte_number] |= rom_byte_mask;
            else
              ROM_NO[rom_byte_number] &= ~rom_byte_mask;

            // serial number search direction write bit
            OWWriteBit(search_direction);

            // increment the byte counter id_bit_number
            // and shift the mask rom_byte_mask
            id_bit_number++;
            rom_byte_mask <<= 1;

            // if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
            if (rom_byte_mask == 0)
            {
                docrc8(ROM_NO[rom_byte_number]);  // accumulate the CRC
                rom_byte_number++;
                rom_byte_mask = 1;
            }
         }
      }
      while(rom_byte_number < 8);  // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((id_bit_number < 65) || (crc8 != 0)))
      {
         // search successful so set LastDiscrepancy,LastDeviceFlag,search_result
         LastDiscrepancy = last_zero;

         // check for last device
         if (LastDiscrepancy == 0)
            LastDeviceFlag = TRUE;
         
         search_result = TRUE;
      }
   }

   // if no device found then reset counters so next 'search' will be like a first
   if (!search_result || !ROM_NO[0])
   {
      LastDiscrepancy = 0;
      LastDeviceFlag = FALSE;
      LastFamilyDiscrepancy = 0;
      search_result = FALSE;
   }

   return search_result;
}

//--------------------------------------------------------------------------
// Verify the device with the ROM number in ROM_NO buffer is present.
// Return TRUE  : device verified present
//        FALSE : device not present
//
unsigned char OWVerify(void)
{
   unsigned char rom_backup[8];
   unsigned char i,rslt,ld_backup,ldf_backup,lfd_backup;

   // keep a backup copy of the current state
   for (i = 0; i < 8; i++)
      rom_backup[i] = ROM_NO[i];
   ld_backup = LastDiscrepancy;
   ldf_backup = LastDeviceFlag;
   lfd_backup = LastFamilyDiscrepancy;

   // set search to find the same device
   LastDiscrepancy = 64;
   LastDeviceFlag = FALSE;

   if (OWSearch())
   {
      // check if same device found
      rslt = TRUE;
      for (i = 0; i < 8; i++)
      {
         if (rom_backup[i] != ROM_NO[i])
         {
            rslt = FALSE;
            break;
         }
      }
   }
   else
     rslt = FALSE;

   // restore the search state 
   for (i = 0; i < 8; i++)
      ROM_NO[i] = rom_backup[i];
   LastDiscrepancy = ld_backup;
   LastDeviceFlag = ldf_backup;
   LastFamilyDiscrepancy = lfd_backup;

   // return the result of the verify
   return rslt;
}

// TEST BUILD
static unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};

//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
   // See Application Note 27
   // TEST BUILD
   crc8 = dscrc_table[crc8 ^ value];
   return crc8;
}


