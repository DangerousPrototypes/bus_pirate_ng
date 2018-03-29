
void HWI2C_start(void);
void HWI2C_stop(void);
uint32_t HWI2C_send(uint32_t d);
uint32_t HWI2C_read(void);
void HWI2C_macro(uint32_t macro);
void HWI2C_setup(void);
void HWI2C_setup_exc(void);
void HWI2C_cleanup(void);
void HWI2C_pins(void);
void HWI2C_settings(void);
void HWI2C_printI2Cflags(void);
void HWI2C_help(void);

#define HWI2CSPEEDMENU	"\r\nSpeed\r\n 1. 100KHz\r\n 2. 400Khz\r\nspeed> "

#define LA_I2C_PERIOD_100KHZ (((100000000/100)/4)/(10000000/72000))/10
#define LA_I2C_PERIOD_400KHZ (((100000000/400)/4)/(10000000/72000))/10