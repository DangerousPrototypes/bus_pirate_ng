
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

// menus
#define SPISPEEDMENU	"\r\nSPI Clock speed\r\n 1. 18Mhz\r\n 2.  9Mhz\r\n 3. 4.5Mhz\r\n 4. 2Mhz\r\n 5. 1Mhz\r\n 6. 560khz\r\n 7. 280Khz\r\n 8. 140Khz*\r\nspeed> "
#define SPICPOLMENU	"\r\nClock polarity\r\n 1. idle low*\r\n 2. idle high\r\ncpol> "
#define SPICPHAMENU	"\r\nClock phase\r\n 1. leading edge\r\n 2. trailing edge*\r\ncpha> "
#define SPICSIDLEMENU	"\r\nCS mode\r\n 1. CS\r\n 2. !CS*\r\ncs> "

#define LA_SPI_SAMPLE_140KHZ (((100000000/140)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_280KHZ (((100000000/280)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_560KHZ (((100000000/560)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_1MHZ (((100000000/1000)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_2MHZ (((100000000/2000)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_4MHZ (((100000000/4500)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_9MHZ (((100000000/9000)/4)/(10000000/72000)/10)
#define LA_SPI_SAMPLE_18MHZ LA_SPI_SAMPLE_9MHZ //(((100000000/18000)/4)/(10000000/72000)/10) //=1, wont work! TODO:add warning!
