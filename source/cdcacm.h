

// declarations
uint8_t cdcbyteready(void);
uint8_t cdcgetc(void);
void cdcputc(char c);
void cdcputs(char *s);
void cdcprintf(const char *fmt, ...);
void cdcinit(void);
void cdcpoll(void);



// defines
#define RXBUFFSIZE	128		// needs to be power of 2
#define TXBUFFSIZE	128		// needs to be power of 2
#define PRINTBUFLEN	256		// buffer for cdcprintf

#define IRQ_PRI_USB	(2 << 4)	// not used prio of usb interrupt

