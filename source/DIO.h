
uint32_t DIO_send(uint32_t d);
uint32_t DIO_read(void);
void DIO_macro(uint32_t macro);
void DIO_setup(void);
void DIO_setup_exc(void);
void DIO_cleanup(void);
void DIO_pins(void);
void DIO_settings(void);
void DIO_help(void);


#define DIOODMENU "\r\nOutput mode\r\n 1. Normal*\r\n 2. Opendrain\r\nopendrain> "
#define DIOOMMENU "\r\noutputmask> "

