
uint32_t LCDSPI_send(uint32_t d);
uint32_t LCDSPI_read(void);
void LCDSPI_macro(uint32_t macro);
void LCDSPI_setup(void);
void LCDSPI_setup_exc(void);
void LCDSPI_cleanup(void);
void LCDSPI_pins(void);
void LCDSPI_settings(void);

void HD44780_init(void);
void HD44760_cleanup(void);
void HD44780_initdisp(void);
void HD44780_write(uint8_t rs, uint8_t d);
void HD44780_writenibble(uint8_t rs, uint8_t d);

