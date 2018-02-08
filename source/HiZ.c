
#include <stdint.h>
#include <libopencm3/stm32/gpio.h>
#include "buspirateNG.h"
#include "HiZ.h"
#include "cdcacm.h"
#include "UI.h"
#include "AUXpin.h"	


void HiZpins(void)
{
	cdcprintf("-\t-\t-\t-");
}

void HiZsettings(void)
{
	cdcprintf("HiZ ()=()");
}


void HiZcleanup(void)
{
}

void HiZsetup(void)
{
}

// this is called duringmode changes; takes care pwm, vpu and psu is turned off, also AUX to input
void HiZsetup_exc(void)
{
	// turn everything off
	modeConfig.psu=0;
	gpio_clear(BP_PSUEN_PORT, BP_PSUEN_PIN);
	modeConfig.pullups=0;
	gpio_set(BP_VPUEN_PORT, BP_VPUEN_PIN);

	// aux and pwm
	(void)getAUX();
	setPWM(0, 0);
}
