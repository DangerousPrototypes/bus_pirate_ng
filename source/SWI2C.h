void SWI2C_start(void);
void SWI2C_stop(void);
uint32_t SWI2C_write(uint32_t d);
uint32_t SWI2C_read(void);
void SWI2C_macro(uint32_t macro);
void SWI2C_setup(void);
void SWI2C_setup_exc(void);
void SWI2C_cleanup(void);
void SWI2C_pins(void);
void SWI2C_settings(void);
void SWI2C_printI2Cflags(void);
void SWI2C_help(void);

void SWI2C_setDATAmode(uint8_t input);
#define SWI2C_INPUT	1
#define SWI2C_OUTPUT 0

#define SWI2CSPEEDMENU	"\r\nSpeed\r\n 1. 100KHz\r\n 2. 400Khz\r\nspeed> "

#define LA_SWI2C_PERIOD_100KHZ (((100000000/100)/4)/(10000000/72000))/10
#define LA_SWI2C_PERIOD_400KHZ (((100000000/400)/4)/(10000000/72000))/10

#define SWI2C_CLOCK_HIGH() gpio_set(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN)
#define SWI2C_CLOCK_LOW() gpio_clear(BP_SW2W_CLK_PORT, BP_SW2W_CLK_PIN)

#define SWI2C_DATA_HIGH() gpio_set(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN)
#define SWI2C_DATA_LOW() gpio_clear(BP_SW2W_SDA_PORT, BP_SW2W_SDA_PIN)