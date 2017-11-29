
#include <stdint.h>
#include "HiZ.h"
#include "cdcacm.h"


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
	// turn everything off
}
