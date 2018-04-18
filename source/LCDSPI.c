

#include <stdint.h>
#include <libopencm3/stm32/spi.h>
#include "buspirateNG.h"
#include "HWSPI.h"
#include "cdcacm.h"
#include "UI.h"
#include "LCDSPI.h"

#include "HD44780.h"
#include "ST7735R.h"

static uint32_t currentdisplay;


// subset of the protocol
typedef struct _display
{
	uint32_t (*display_send)(uint32_t);		// send max 32 bit
	uint32_t (*display_read)(void);			// read max 32 bit (is thsi actually used?)
	void (*display_macro)(uint32_t);		// macro
	void (*display_setup)(void);			// setup UI
	void (*display_cleanup)(void);			// cleanup for HiZ
	char name[10];
} display;

static struct _display displays[2]={
{
	HD44780_write,
	0,	//HD44780_read,
	HD44780_macro,
	HD44780_setup,
	HD44780_cleanup,
	"HD44780"
},
{
	ST7735R_send,
	0,	//HD44780_read,
	ST7735R_macro,
	ST7735R_setup,
	ST7735R_cleanup,
	"ST7735D/R"
}
};

uint32_t LCDSPI_send(uint32_t d)
{
	return displays[currentdisplay].display_send(d);
}

uint32_t LCDSPI_read(void)
{
	return displays[currentdisplay].display_read();
}

void LCDSPI_macro(uint32_t macro)
{
	displays[currentdisplay].display_macro(macro);
}

void LCDSPI_setup(void)
{
	
	currentdisplay=1;
}

void LCDSPI_setup_exc(void)
{
	displays[currentdisplay].display_setup();
}

void LCDSPI_cleanup(void)
{
	displays[currentdisplay].display_cleanup();
}

void LCDSPI_pins(void)
{
	cdcprintf("CS\tMISO\tCLK\tMOSI");
}

void LCDSPI_settings(void)
{
	cdcprintf("LCDSPI (display)=(%d) \"%s\"", currentdisplay, displays[currentdisplay].name);
}


