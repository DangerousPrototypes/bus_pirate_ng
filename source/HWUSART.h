
uint32_t HWUSART_send(uint32_t d);
uint32_t HWUSART_read(void);
void HWUSART_macro(uint32_t macro);
void HWUSART_setup(void);
void HWUSART_setup_exc(void);
void HWUSART_cleanup(void);
void HWUSART_pins(void);
void HWUSART_settings(void);
void HWUSART_printerror(void);


#define UARTSPEEDMENU		"\r\nUART Speed\r\n 1... baudrate\r\nbr> "
#define UARTPARITYMENU 		"\r\nParity\r\n 1. none*\r\n 2. even\r\n 3. odd\r\nparity> "
#define UARTNUMBITSMENU 	"\r\nNumber of bits\r\n 8. 8*\r\n 9. 9\r\nbits> "
#define UARTSTOPBITSMENU	"\r\nNumber of stop bits\r\n 1. 1*\r\n 2. 0.5\r\n 3. 2\r\n 4. 1.5\r\nbits> "
#define UARTBLOCKINGMENU	"\r\nUse blocking functions?\r\n 1. no\r\n 2. yes\r\nblock> "

#define USARTERRORS	(USART_SR_PE|USART_SR_FE|USART_SR_NE|USART_SR_ORE|USART_SR_LBD)

