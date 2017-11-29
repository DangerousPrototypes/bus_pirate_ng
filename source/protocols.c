

#include <stdint.h>
#include "cdcacm.h"
#include "buspirateNG.h"
#include "UI.h"
#include "protocols.h"

#include "HiZ.h"
#include "dummy1.h"
#include "dummy2.h"
#include "HWSPI.h" 



// nulfuncs
// these are the dummy functions when something aint used

void nullfunc1(void)
{
	cdcprintf("ERROR: command has no effect here\r\n");
}

uint32_t nullfunc2(uint32_t c)
{	
	(void) c;
	cdcprintf("ERROR: command has no effect here\r\n");
	return 0x0000;
}

uint32_t nullfunc3(void)
{	
	cdcprintf("ERROR: command has no effect here\r\n");
	return 0x0000;
}

void nullfunc4(uint32_t c)
{	
	(void) c;
	cdcprintf("ERROR: command has no effect here\r\n");
}



struct _protocol protocols[MAXPROTO]={

{
	nullfunc1,				// start
	nullfunc1,				// start with read
	nullfunc1,				// stop
	nullfunc1,				// stop with read
	nullfunc2,				// send(/read) max 32 bit
	nullfunc3,				// read max 32 bit
	nullfunc1,				// set clk high
	nullfunc1,				// set clk low
	nullfunc1,				// set dat hi
	nullfunc1,				// set dat lo
	nullfunc3,				// toggle dat (?)
	nullfunc1,				// toggle clk (?)
	nullfunc3,				// read 1 bit (?)
	nullfunc3,				// service to regular poll whether a byte ahs arrived
	nullfunc4,				// macro
	HiZsetup,				// setup UI
	HiZsetup_exc,				// real setup
	HiZcleanup,				// cleanup for HiZ
	HiZpins,				// display pin config
	HiZsettings,				// display settings 
	"HiZ",					// friendly name (promptname)
},
#ifdef BP_USE_DUMMY1
{
	dummy1_start,				// start
	dummy1_startr,				// start with read
	dummy1_stop,				// stop
	dummy1_stopr,				// stop with read
	dummy1_send,				// send(/read) max 32 bit
	dummy1_read,				// read max 32 bit
	dummy1_clkh,				// set clk high
	dummy1_clkl,				// set clk low
	dummy1_dath,				// set dat hi
	dummy1_datl,				// set dat lo
	dummy1_dats,				// toggle dat (?)
	dummy1_clk,				// toggle clk (?)
	dummy1_bitr,				// read 1 bit (?)
	dummy1_period,				// service to regular poll whether a byte ahs arrived
	dummy1_macro,				// macro
	dummy1_setup,				// setup UI
	dummy1_setup_exc,			// real setup
	dummy1_cleanup,				// cleanup for HiZ
	dummy1_pins,				// display pin config
	dummy1_settings,			// display settings 
	"DUMMY1",				// friendly name (promptname)
},
#endif
#ifdef BP_USE_DUMMY1
{
	dummy2_start,				// start
	dummy2_startr,				// start with read
	dummy2_stop,				// stop
	dummy2_stopr,				// stop with read
	dummy2_send,				// send(/read) max 32 bit
	dummy2_read,				// read max 32 bit
	dummy2_clkh,				// set clk high
	dummy2_clkl,				// set clk low
	dummy2_dath,				// set dat hi
	dummy2_datl,				// set dat lo
	dummy2_dats,				// toggle dat (?)
	dummy2_clk,				// toggle clk (?)
	dummy2_bitr,				// read 1 bit (?)
	dummy2_period,				// service to regular poll whether a byte ahs arrived
	dummy2_macro,				// macro
	dummy2_setup,				// setup UI
	dummy2_setup_exc,			// real setup
	dummy2_cleanup,				// cleanup for HiZ
	dummy2_pins,				// display pin config
	dummy2_settings,			// display settings 
	"DUMMY2",				// friendly name (promptname)
},
#endif
#ifdef BP_USE_HWSPI
{
	HWSPI_start,				// start
	HWSPI_startr,				// start with read
	HWSPI_stop,				// stop
	HWSPI_stopr,				// stop with read
	HWSPI_send,				// send(/read) max 32 bit
	HWSPI_read,				// read max 32 bit
	HWSPI_clkh,				// set clk high
	HWSPI_clkl,				// set clk low
	HWSPI_dath,				// set dat hi
	HWSPI_datl,				// set dat lo
	HWSPI_dats,				// toggle dat (?)
	HWSPI_clk,				// toggle clk (?)
	HWSPI_bitr,				// read 1 bit (?)
	HWSPI_period,				// service to regular poll whether a byte ahs arrived
	HWSPI_macro,				// macro
	HWSPI_setup,				// setup UI
	HWSPI_setup_exc,			// real setup
	HWSPI_cleanup,				// cleanup for HiZ
	HWSPI_pins,				// display pin config
	HWSPI_settings,				// display settings 
	"HW-SPI",				// friendly name (promptname)
},
#endif
};




