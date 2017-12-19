
#include <stdint.h>
#include "buspirateNG.h"
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "HWI2C.h"
#include "cdcacm.h"
#include "UI.h"

static uint8_t	speed;


void HWI2C_start(void)
{
	cdcprintf("START");
	i2c_send_start(BPI2C);

	// wait for start (SB), switched to master (MSL) and a taken bus (BUSY)
	while (!((I2C_SR1(BPI2C)&I2C_SR1_SB)&(I2C_SR2(BPI2C)&(I2C_SR2_MSL|I2C_SR2_BUSY))));
}
void HWI2C_startr(void)
{
	cdcprintf("-HWI2C- startr()");
	i2c_send_start(BPI2C);
}
void HWI2C_stop(void)
{
	cdcprintf("STOP");
	i2c_send_stop(BPI2C);

}
void HWI2C_stopr(void)
{
	cdcprintf("-HWI2C- stopr()");
	i2c_send_stop(BPI2C);

}
uint32_t HWI2C_send(uint32_t d)
{
	uint8_t ack;
	uint32_t temp;

//	HWI2C_printI2Cflags();

	if(I2C_SR2(BPI2C)&I2C_SR2_MSL)					// we can only send if master! (please issue a start condition frist)
	{
		if((I2C_SR1(BPI2C)&I2C_SR1_SB)||(I2C_SR2(BPI2C)&I2C_SR2_TRA))	// writing is only enable after start or during transmisson
		{
			temp=(I2C_SR1(BPI2C)&I2C_SR1_SB); 		// gets destroyed by writing?

			i2c_send_data(BPI2C, d);

//			HWI2C_printI2Cflags();

			if (temp) while((!(I2C_SR1(BPI2C)&I2C_SR1_ADDR))&&(!(I2C_SR1(BPI2C)&I2C_SR1_AF))); // or BTF??
			else while((!(I2C_SR1(BPI2C)&I2C_SR1_BTF))&&(!(I2C_SR1(BPI2C)&I2C_SR1_AF)));

//			HWI2C_printI2Cflags();

//			if(I2C_SR2(BPI2C)&I2C_SR2_MSL)			// are we mastah??
			ack=!(I2C_SR1(BPI2C)&I2C_SR1_AF);		// no ack error is ack
//			else
//				ack=0;		
	
			temp=I2C_SR1(BPI2C);				// need to read both status registers??
			temp=I2C_SR2(BPI2C);
			I2C_SR1(BPI2C)=0;				// clear all errors/flags
			I2C_SR2(BPI2C)=0;				// clear all errors/flags

			cdcprintf(" %s", (ack?"ACK":"NACK"));
		}
		else
		{
			cdcprintf("Not allowed to send (wrong address)");
			modeConfig.error=1;
			ack=0;
		}
	}
	else
	{
		cdcprintf("Not allowed to send (START)");
		modeConfig.error=1;
		ack=0;
	}

	return (!ack);
}
uint32_t HWI2C_read(void)
{
	uint32_t returnval;

	if(!(I2C_SR2(BPI2C)&I2C_SR2_TRA))
	{
		i2c_enable_ack(BPI2C);				// TODO: clever way to nack last byte as per spec	

		while (!(I2C_SR1(BPI2C) & I2C_SR1_RxNE));		// wait until data available

		returnval=i2c_get_data(BPI2C);
	}
	else
	{
		cdcprintf("Not allowed to read (wrong address)");
		modeConfig.error=1;
		returnval=0;
	}

	return returnval;
}
void HWI2C_clkh(void)
{
	cdcprintf("-HWI2C- clkh()");
}
void HWI2C_clkl(void)
{
	cdcprintf("-HWI2C- clkl()");
}
void HWI2C_dath(void)
{
	cdcprintf("-HWI2C- dath()");
}
void HWI2C_datl(void)
{
	cdcprintf("-HWI2C- datl()");
}
uint32_t HWI2C_dats(void)
{
	uint32_t returnval=0;
	cdcprintf("-HWI2C- dats()=%08X", returnval);
	return returnval;
}
void HWI2C_clk(void)
{
	cdcprintf("-HWI2C- clk()");
}
uint32_t HWI2C_bitr(void)
{
	uint32_t returnval=0;
	cdcprintf("-HWI2C- bitr()=%08X", returnval);
	return returnval;
}
uint32_t HWI2C_period(void)
{
	uint32_t returnval=0;
	cdcprintf("-HWI2C- period()=%08X", returnval);
	return returnval;
}
void HWI2C_macro(uint32_t macro)
{
	cdcprintf("-HWI2C- macro(%08X)", macro);
}
void HWI2C_setup(void)
{
	cdcprintf("-HWI2C- setup()");

	speed=0;
}
void HWI2C_setup_exc(void)
{
	cdcprintf("-HWI2C- setup_exc()");

	rcc_periph_clock_enable(BPI2CCLK);

	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN, GPIO_I2C2_SCL | GPIO_I2C2_SDA);

	// i2c needs to be disabled before it can be programmed
	i2c_peripheral_disable(BPI2C);
	i2c_reset(BPI2C);

	// setup timing 
	// speed=0 100KHz
	// speed=1 400KHz
	i2c_set_speed(BPI2C, speed,I2C_CR2_FREQ_36MHZ);	// 36MHz is max

	//?
	//i2c_set_own_7bit_slave_address(I2C2, sp,0x32); // do we want this?

	// go!
	i2c_peripheral_enable(BPI2C);
}
void HWI2C_cleanup(void)
{
	cdcprintf("-HWI2C- cleanup()");

	rcc_periph_clock_disable(RCC_I2C2);
	i2c_peripheral_disable(I2C2);
}
void HWI2C_pins(void)
{
	cdcprintf("-\t-\tSCL\tSDA");
}
void HWI2C_settings(void)
{
	cdcprintf("HWI2C (arg1 arg2)=(%d, %d)", 1, 2);
}

void HWI2C_printI2Cflags(void)
{
	uint32_t temp;

	temp=I2C_SR1(BPI2C);

	if(temp&I2C_SR1_SMBALERT) cdcprintf(" SMBALERT");
	if(temp&I2C_SR1_TIMEOUT) cdcprintf(" TIMEOUT");
	if(temp&I2C_SR1_PECERR) cdcprintf(" PECERR");
	if(temp&I2C_SR1_OVR) cdcprintf(" OVR");
	if(temp&I2C_SR1_AF) cdcprintf(" AF");
	if(temp&I2C_SR1_ARLO) cdcprintf(" ARLO");
	if(temp&I2C_SR1_BERR) cdcprintf(" BERR");
	if(temp&I2C_SR1_TxE) cdcprintf(" TxE");
	if(temp&I2C_SR1_RxNE) cdcprintf(" RxNE");
	if(temp&I2C_SR1_STOPF) cdcprintf(" STOPF");
	if(temp&I2C_SR1_ADD10) cdcprintf(" ADD10");
	if(temp&I2C_SR1_BTF) cdcprintf(" BTF");
	if(temp&I2C_SR1_ADDR) cdcprintf(" ADDR");
	if(temp&I2C_SR1_SB) cdcprintf(" SB");

	cdcprintf(";");
	temp=I2C_SR2(BPI2C);

	if(temp&I2C_SR2_DUALF) cdcprintf(" DUALf");
	if(temp&I2C_SR2_SMBHOST) cdcprintf(" SMBHOST");
	if(temp&I2C_SR2_SMBDEFAULT) cdcprintf(" SMBDEFAULT");
	if(temp&I2C_SR2_GENCALL) cdcprintf(" GENCALL");
	if(temp&I2C_SR2_TRA) cdcprintf(" TRA");
	if(temp&I2C_SR2_BUSY) cdcprintf(" BUSY");
	if(temp&I2C_SR2_MSL) cdcprintf(" MSL");

	cdcprintf("!");
}

