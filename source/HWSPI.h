
void HWSPI_start(void);
void HWSPI_startr(void);
void HWSPI_stop(void);
void HWSPI_stopr(void);
uint32_t HWSPI_send(uint32_t d);
uint32_t HWSPI_read(void);
void HWSPI_macro(uint32_t macro);
void HWSPI_setup(void);
void HWSPI_setup_exc(void);
void HWSPI_cleanup(void);
void HWSPI_pins(void);
void HWSPI_settings(void);
void HWSPI_printSPIflags(void);
void HWSPI_help(void);

// special for binmode and lcd
void HWSPI_setcpol(uint32_t val);
void HWSPI_setcpha(uint32_t val);
void HWSPI_setbr(uint32_t val);
void HWSPI_setdff(uint32_t val);
void HWSPI_setlsbfirst(uint32_t val);
void HWSPI_setcsidle(uint32_t val);
void HWSPI_setcs(uint8_t cs);
uint16_t HWSPI_xfer(uint16_t d);









// menu's

#define SPISPEEDMENU	"\r\nSPI Clock speed\r\n 1. 18.00Mhz\r\n 2.  9.00Mhz\r\n 3. 4.500Mhz\r\n 4. 2.250Mhz\r\n 5. 1.125Mhz\r\n 6. 562.5khz\r\n 7. 281.3Khz\r\nspeed> "
#define SPICPOLMENU	"\r\nClock polarity\r\n 1. idle low\r\n 2. idle high*\r\ncpol> "
#define SPICPHAMENU	"\r\nClock phase\r\n 1. leading edge\r\n 2. trailing edge*\r\ncpha> "
#define SPICSIDLEMENU	"\r\nCS mode\r\n 1. CS\r\n 2. !CS*\r\ncs> "


