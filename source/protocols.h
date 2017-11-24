
#include "buspirateNG.h"

enum
{
	HIZ = 0,
#ifdef BP_USE_DUMMY1
	DUMMY1,
#endif
#ifdef BP_USE_DUMMY1
	DUMMY2,
#endif
	MAXPROTO
};


typedef struct _protocol
{
	void (*protocol_start)(void);			// start
	void (*protocol_startR)(void);			// start with read
	void (*protocol_stop)(void);			// stop
	void (*protocol_stopR)(void);			// stop with read
	uint32_t (*protocol_send)(uint32_t);		// send(/read) max 32 bit
	uint32_t (*protocol_read)(void);		// read max 32 bit
	void (*protocol_clkh)(void);			// set clk high
	void (*protocol_clkl)(void);			// set clk low
	void (*protocol_dath)(void);			// set dat hi
	void (*protocol_datl)(void);			// set dat lo
	uint32_t (*protocol_dats)(void);		// toglle dat (?)
	void (*protocol_clk)(void);			// toggle clk (?)
	uint32_t (*protocol_bitr)(void);		// read 1 bit (?)
	uint32_t (*protocol_periodic)(void);		// service to regular poll whether a byte ahs arrived
	void (*protocol_macro)(uint32_t);		// macro
	void (*protocol_setup)(void);			// setup UI
	void (*protocol_setup_exc)(void);		// real setup
	void (*protocol_cleanup)(void);			// cleanup for HiZ
	void (*protocol_pins)(void);			// display pin config
	void (*protocol_settings)(void);		// display settings 
	char protocol_name[8];				// friendly name (promptname)
} protocol;

extern struct _protocol protocols[MAXPROTO];


void nullfunc1(void);
uint32_t nullfunc2(uint32_t c);
uint32_t nullfunc3(void);
void nullfunc4(uint32_t c);
void HiZpins(void);
void HiZsettings(void);

