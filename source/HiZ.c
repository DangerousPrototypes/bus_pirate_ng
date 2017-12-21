
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

void HiZsetup_exc(void)
{
	cdcprintf("turning to HiZ");

	// turn everything off
	modeConfig.psu=0;
	gpio_clear(BPPSUENPORT, BPPSUENPIN);
	modeConfig.pullups=0;
	gpio_clear(BPVPUENPORT, BPVPUENPIN);

	// aux and pwm
	(void)getAUX();
	setPWM(0, 0);
}
