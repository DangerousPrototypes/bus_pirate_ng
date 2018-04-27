
typedef struct _modeConfig
{
	uint8_t numbits;				// number of used bits
	uint8_t wwr;					// write with read
	uint8_t oc;					// are we in opencollector?
	uint8_t mode;					// which mode we are in?
	uint8_t pullups;				// pullup enabled? (0=off, 1=on)
	uint8_t vpumode;				// vpu mode (0=ext, 1=5v, 2=3v3)
	uint8_t bitorder;				// bitorder (0=msb, 1=lsb)
	uint8_t psu;					// psu (0=off, 1=on)
	uint8_t error;					// error occurred
	uint8_t displaymode;				// display mode (dec, hex, oct, bin)
	uint8_t pwm;					// pwm active?
	uint8_t init;					// for postphoning the init
	char *subprotocolname;				// can be set if there is a sub protocol
	
	uint8_t	logicanalyzertriggersactive;
	uint8_t	logicanalyzertriggersdirection; 
	uint16_t logicanalyzerperiod; 			// period of the logic analyzer clock
	uint32_t logicanalyzersamplecount; 		//number of samples in most recent capture
	uint8_t logicanalyzerstop;			//why did the logic analyzer stop?
	
	uint32_t csport;				// cs is located on this port/gpio
	uint32_t cspin;
	uint32_t misoport;				// cs is located on this port/gpio
	uint32_t misopin;
	uint32_t clkport;				// cs is located on this port/gpio
	uint32_t clkpin;
	uint32_t mosiport;				// cs is located on this port/gpio
	uint32_t mosipin;
} _modeConfig;


// define our globals
extern uint32_t cmdhead, cmdvhead, cmdtail;
extern char cmdbuff[CMDBUFFSIZE];
extern struct _modeConfig modeConfig;
extern const char vpumodes[][4];
extern const char bitorders[][4];
extern const char states[][4];


//declarations
void consumewhitechars(void);
uint32_t getint(void);
void versioninfo(void);
void doUI(void);
void initUI(void);
void showstates(void);
void changemode(void);
void printhelp(void);
int cmdhistory(int ptr);
void getuserinput(void);
void changedisplaymode(void);
uint32_t askint(const char *menu, uint32_t minval, uint32_t maxval, uint32_t defval);
uint32_t getrepeat(void);
uint32_t getnumbits(void);
void printnum(uint32_t d);
void initdelay(void);
void delayms(uint32_t num);
void delayus(uint32_t num);
uint32_t orderbits(uint32_t d);
uint8_t getpinmode(uint32_t port, uint16_t pin);
void jumptobootloader(void);
void reset(void);
int isbuscmd(char c);

// macro
#define SKIPCURCHAR	if(cmdtail!=cmdhead) cmdtail=(cmdtail+1)&(CMDBUFFSIZE-1);

// 
#define VPUMENU		"\r\nVPU mode:\r\n 1. External\r\n 2. 3V3\r\n 3. 5V0\r\nvpumode> "
#define PWMMENUPERIOD	"\r\nPeriod in ticks (0..0xFFFFFFFF)> "
#define PWMMENUOC	"\r\nOverflow in ticks (0..0xFFFFFFFF)> "



