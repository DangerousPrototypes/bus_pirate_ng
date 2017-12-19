
// global config file

// UI stuff
#define CMDBUFFSIZE	512		// must be power of 2

// USB shit :/

// we dont have a USB VID/PID yet so please supply your own
#define		USB_VID		0x 
#define		USB_PID		0x
#define		USB_VENDOR	"dangerousprototypes.com"
#define		USB_PRODUCT	"buspirateNG"
#define		USB_SERIAL	"00000000"

// enable protocols
#define 	BP_USE_DUMMY1
#define 	BP_USE_DUMMY2
#define		BP_USE_HWSPI
#define		BP_USE_HWUSART
#define		BP_USE_HWI2C

// systicks (for delays) systick is 10us
extern volatile uint32_t systicks;

// include platform
#include	"platform/testplatform.h"			// hw is not made yet



