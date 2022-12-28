// global config file

// UI stuff
#define CMDBUFFSIZE	16384		// must be power of 2

// USB shit :/

// USB VID/PID
#define		USB_VID		0x1209
#define		USB_PID		0x7331
#define		USB_VENDOR	"DangerousPrototypes.com"
#define		USB_PRODUCT	"BusPirateNG"

// enable protocols
#define		BP_USE_1WIRE
#define		BP_USE_HWUSART
#define		BP_USE_HWI2C
#define		BP_USE_HWSPI
#define		BP_USE_SW2W
#define		BP_USE_SW3W
#define 	BP_USE_DIO
#define		BP_USE_LCDSPI
//#define		BP_USE_LCDI2C
#define		BP_USE_LA
#define 	BP_USE_DUMMY1
#define 	BP_USE_DUMMY2

// enable display support
#define		DISPLAY_USE_HD44780	// is always enabled 
#define		DISPLAY_USE_ST7735

// systicks (for delays) systick is 10us
extern volatile uint32_t systicks;

// include platform
// TODO we need some makefile tricks for this
//#include	"platform/testplatform.h"			// hw is not made yet
//#include	"platform/beta1.h"				// BPNG v1.0 dec 2017
#include	"platform/beta2.h"				// BPNG v1.0 feb 2018



