

// declarations
uint8_t cdcbyteready(void);
uint8_t cdcgetc(void);
void cdcputc(char c);
void cdcputs(char *s);
void cdcprintf(const char *fmt, ...);
void cdcinit(void);
void cdcflush(void);

uint8_t cdcbyteready2(void);
uint8_t cdcgetc2(void);
void cdcputc2(char c);
void cdcputs2(char *s);
void cdcprintf2(const char *fmt, ...);




// defines
// !!pointer for the ringbuffer are 8bit!!
#define RXBUFFSIZE	256		// needs to be power of 2 cmd 
#define TXBUFFSIZE	256		// needs to be power of 2
#define RXLABUFFSIZE	256		// needs to be power of 2 la
#define TXLABUFFSIZE	256		// needs to be power of 2
#define PRINTBUFLEN	256		// buffer for cdcprintf

#define IRQ_PRI_USB	(2 << 4)	// not used prio of usb interrupt

// ANSI text colours 
#define BLACK		"\1B[30m"	
#define RED		"\1B[31m"	
#define GREEN		"\1B[32m"	
#define YELLOW		"\1B[33m"	
#define BLUE		"\1B[34m"	
#define MAGENTA		"\1B[35m"	
#define CYAN		"\1B[36m"	
#define WHITE		"\1B[37m"	// normal color
#define BRIGHTBLACK	"\1B[90m"	
#define BRIGHTRED	"\1B[91m"	
#define BRIGHTGREEN	"\1B[92m"	
#define BRIGHTYELLOW	"\1B[93m"	
#define BRIGHTBLUE	"\1B[94m"	
#define BRIGHTMAGENTA	"\1B[95m"	
#define BRIGHTCYAN	"\1B[96m"	
#define BRIGHTWHITE	"\1B[97m"

// ANSI background colours 
#define BGBLACK		"\1B[40m"	//normal BG
#define BGRED		"\1B[41m"	
#define BGGREEN		"\1B[42m"	
#define BGYELLOW	"\1B[43m"	
#define BGBLUE		"\1B[44m"	
#define BGMAGENTA	"\1B[45m"	
#define BGCYAN		"\1B[46m"	
#define BGWHITE		"\1B[47m"
#define BGBRIGHTBLACK	"\1B[100m"	
#define BGBRIGHTRED	"\1B[101m"	
#define BGBRIGHTGREEN	"\1B[102m"	
#define BGBRIGHTYELLOW	"\1B[103m"	
#define BGBRIGHTBLUE	"\1B[104m"	
#define BGBRIGHTMAGENTA	"\1B[105m"	
#define BGBRIGHTCYAN	"\1B[106m"	
#define BGBRIGHTWHITE	"\1B[107m"

// ANSI textstyles
#define BOLD		"\1B[1m"
#define BOLDOFF		"\1B[21m"
#define ITALIC		"\1B[3m"
#define ITALICOFF	"\1B[23m"
#define BLINK		"\1B[5m"
#define BLINKOFF	"\1B[25m"
#define INVERSE		"\1B[7m"
#define INVERSEOFF	"\1B[27m"
#define ANSIRESET	"\1B[0m"

