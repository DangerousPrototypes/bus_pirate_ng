
#include <stdint.h>
#include "dummy2.h"
#include "cdcacm.h"


static uint32_t returnval;

void dummy2_start(void)
{
	cdcprintf("-DUMMY2- start()");
}
void dummy2_startr(void)
{
	cdcprintf("-DUMMY2- startr()");
}
void dummy2_stop(void)
{
	cdcprintf("-DUMMY2- stop()");
}
void dummy2_stopr(void)
{
	cdcprintf("-DUMMY2- stopr()");
}
uint32_t dummy2_send(uint32_t d)
{
	cdcprintf("--DUMMY2- send(%08X)=%08X", d, returnval);

	returnval=d;

	return d;
}
uint32_t dummy2_read(void)
{
	cdcprintf("-DUMMY2- read()=%08X", returnval);
	return returnval;
}
void dummy2_clkh(void)
{
	cdcprintf("-DUMMY2- clkh()");
}
void dummy2_clkl(void)
{
	cdcprintf("-DUMMY2- clkl()");
}
void dummy2_dath(void)
{
	cdcprintf("-DUMMY2- dath()");
}
void dummy2_datl(void)
{
	cdcprintf("-DUMMY2- datl()");
}
uint32_t dummy2_dats(void)
{
	cdcprintf("-DUMMY2- dats()=%08X", returnval);
	return returnval;
}
void dummy2_clk(void)
{
	cdcprintf("-DUMMY2- clk()");
}
uint32_t dummy2_bitr(void)
{
	cdcprintf("-DUMMY2- bitr()=%08X", returnval);
	return returnval;
}
uint32_t dummy2_period(void)
{
	cdcprintf("-DUMMY2- period()=%08X", returnval);
	return returnval;
}
void dummy2_macro(uint32_t macro)
{
	cdcprintf("-DUMMY2- macro(%08X)", macro);
}
void dummy2_setup(void)
{
	cdcprintf("-DUMMY2- setup()");
}
void dummy2_setup_exc(void)
{
	cdcprintf("-DUMMY2- setup_exc()");
}
void dummy2_cleanup(void)
{
	cdcprintf("-DUMMY2- cleanup()");
}
void dummy2_pins(void)
{
	cdcprintf("pin1\tpin2\tpin3\tpin4");
}
void dummy2_settings(void)
{
	cdcprintf("DUMMY (arg1 arg2)=(%d, %d)", 1, 2);
}

