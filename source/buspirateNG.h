
// global config file

// UI stuff
#define CMDBUFFSIZE	512		// must be power of 2

// USB shit :/

// we dont have a USB VID/PID yet so please supply your own
#define		USB_VID		0x1209
#define		USB_PID		0x7331
#define		USB_VENDOR	"dangerousprototypes.com"
#define		USB_PRODUCT	"buspirateNG"

// enable protocols
#define 	BP_USE_DUMMY1
#define 	BP_USE_DUMMY2
#define		BP_USE_HWSPI
#define		BP_USE_HWUSART
#define		BP_USE_HWI2C
#define		BP_USE_LA
#define		BP_USE_SW2W
#define		BP_USE_SW3W


// systicks (for delays) systick is 10us
extern volatile uint32_t systicks;

// include platform
// TODO we need some makefile tricks for this
//#include	"platform/testplatform.h"			// hw is not made yet
//#include	"platform/beta1.h"				// BPNG v1.0 dec 2017
#include	"platform/beta2.h"				// BPNG v1.0 feb 2018



