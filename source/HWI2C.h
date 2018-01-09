
void HWI2C_start(void);
void HWI2C_startr(void);
void HWI2C_stop(void);
void HWI2C_stopr(void);
uint32_t HWI2C_send(uint32_t d);
uint32_t HWI2C_read(void);
void HWI2C_clkh(void);
void HWI2C_clkl(void);
void HWI2C_dath(void);
void HWI2C_datl(void);
uint32_t HWI2C_dats(void);
void HWI2C_clk(void);
uint32_t HWI2C_bitr(void);
uint32_t HWI2C_period(void);
void HWI2C_macro(uint32_t macro);
void HWI2C_setup(void);
void HWI2C_setup_exc(void);
void HWI2C_cleanup(void);
void HWI2C_pins(void);
void HWI2C_settings(void);
void HWI2C_printI2Cflags(void);


#define HWI2CSPEEDMENU	"\r\nSpeed\r\n 1. 100KHz\r\n 2. 400Khz\r\nspeed> "
